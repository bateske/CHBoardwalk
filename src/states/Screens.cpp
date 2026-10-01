#pragma GCC optimize("Os")   // cold code: size over speed (hot pixel loops live in Draw/Mask/Iso)
#include <Arduino.h>
#include <string.h>
#include <CHGfx.h>
#include "../../config.h"
#include "Screens.h"
#include "../CHGame.h"
#include "../gfx/Palette.h"
#include "../gfx/Draw.h"
#include "../gfx/Mask.h"
#include "../gfx/Fmt.h"
#include "../fx/Fx.h"
#include "../audio/Audio.h"
#include "../game/Game.h"
#include "../save/Save.h"
#include "../debug/Debug.h"

// P0 skeleton: an all-CPU game ticking under a title, to prove the build and
// measure the shared modules and the rules. The real screens replace this.
namespace screens {

static Options opt;
static Stats stats;
static bool hasGame;

void begin() {
    opt.sound = 1;
    save::load(opt, stats, hasGame);
    audio::begin(opt.sound != 0);
    game::Setup s = {{game::CPU, game::CPU + 1, game::CPU + 2, game::OFF}, 30, 1};
    game::start(s);
    fx::banner("BOARDWALK", fx::B_GOLD, 30, 200);
}

void update() {
    game::Event e;
    while (game::popEvent(e)) {
        if (e.type == game::EV_PAY) fx::floatText("$", 64, 100, GOLD);
        if (e.type == game::EV_SOLD) { audio::sfx(Sfx::Gavel); fx::fountain(fx::COIN, 64, 90, 8); }
    }
    game::update(false);
    if (arduboy.justPressed(A_BUTTON)) {
        gfx_wait();
        save::store(opt, stats, game::active());
        fx::shake(6, 2);
    }
    fx::update();
}

void render(uint32_t frame) {
    gfx_clear(FELT);
    char buf[24];
    for (uint8_t p = 0; p < game::st.players; p++) {
        fmtMoney(buf, game::st.pl[p].cash);
        text35x2(20, 50 + p * 14, buf, p == game::st.cur ? FX_B : WHITE);
    }
    fmtInt(fmtStr(buf, "ROUND "), game::st.round);
    text35(40, 110, buf, GOLD);
    fx::drawParticles(2);
    fx::drawFloats();
    fx::drawBanner();
    fx::applyShake(0, 127);
    (void)frame;
}

}  // namespace screens
