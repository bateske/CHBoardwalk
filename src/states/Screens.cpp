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
#include "../iso/Iso.h"
#include "../stage/Stage.h"
#include "../save/Save.h"
#include "../debug/Debug.h"

// P1 probe: the board with a made-up game on it, for the look-dev sheet.
namespace screens {

static Options opt;
static Stats stats;
static bool hasGame;

#if CHBW_DEBUG
//   Z <tileH>            zoom
//   C <tile>             look at a tile
//   O <light> <dark> <solid>   tile tones (palette indices) and solid pink/orange
//   F                    a made-up mid-game: deeds, houses, tokens
static bool debugHook(char cmd, const char *args) {
    using namespace game;
    switch (cmd) {
        case 'Z': stage::setZoom((uint8_t)dbg::parseNum(args, 10)); return true;
        case 'C': stage::lookAt((uint8_t)dbg::parseNum(args, 10)); return true;
        case 'O':
            iso::lightTile = (uint8_t)dbg::parseNum(args, 10);
            iso::darkTile = (uint8_t)dbg::parseNum(args, 10);
            iso::solidGroups = dbg::parseNum(args, 10) != 0;
            return true;
        case 'F': {
            static const uint8_t OWN[][2] = {
                {1, 0}, {3, 0}, {6, 1}, {8, 1}, {9, 1}, {11, 2}, {13, 2}, {14, 2}, {16, 3}, {18, 3}, {19, 3},
                {21, 0}, {23, 0}, {24, 0}, {26, 1}, {27, 1}, {29, 1}, {31, 2}, {32, 2}, {34, 2}, {37, 3}, {39, 3},
                {5, 0}, {15, 1}, {25, 2}, {35, 3}, {12, 0}, {28, 2},
            };
            for (auto &o : OWN) st.deed[o[0]] = o[1];
            static const uint8_t LV[][2] = {{1, 2}, {3, 1}, {6, 4}, {8, 4}, {9, 5}, {11, 3}, {13, 3}, {14, 3},
                                            {16, 1}, {21, 5}, {23, 4}, {24, 4}, {26, 2}, {31, 5}, {32, 5}, {34, 5},
                                            {37, 3}, {39, 4}};
            for (auto &l : LV) st.deed[l[0]] |= (uint8_t)(l[1] << 3);
            uint8_t at = (uint8_t)dbg::parseNum(args, 10);
            st.pl[0].pos = at; st.pl[1].pos = at; st.pl[2].pos = (uint8_t)((at + 1) % 40); st.pl[3].pos = (uint8_t)((at + 39) % 40);
            return true;
        }
    }
    return false;
}
#endif

void begin() {
    opt.sound = 1;
    save::load(opt, stats, hasGame);
    audio::begin(opt.sound != 0);
    game::Setup s = {{game::HUMAN, game::HUMAN, game::HUMAN, game::HUMAN}, 30, 1};
    game::start(s);
    stage::lookAt(0);
#if CHBW_DEBUG
    dbg::hook = debugHook;
#endif
}

void update() {
    stage::update();
    fx::update();
}

void render(uint32_t frame) {
    stage::render(frame, 0);
}

}  // namespace screens
