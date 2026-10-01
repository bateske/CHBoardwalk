#pragma GCC optimize("Os")   // cold code: size over speed (hot pixel loops live in Draw/Iso)
#include <string.h>
#include <CHGfx.h>
#include <Arduino.h>
#include "../../config.h"
#include "Stage.h"
#include "../game/Game.h"
#include "../iso/Iso.h"
#include "../gfx/Draw.h"
#include "../gfx/Palette.h"
#include "../gfx/Fmt.h"
#include "../fx/Fx.h"
#include "../audio/Audio.h"
#include "../assets/Assets.h"

namespace stage {

using namespace iso;
using namespace board;

const uint8_t SEAT_COLOUR[4] = {RED, BLUE, GOLD, FELT_LT};

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------
static bool fast;

// Camera: eased (world, Q4) towards its aim.
static int32_t cx16, cy16;
static int16_t aimX, aimY;
static uint8_t aimShift = 2;

// ---------------------------------------------------------------------------
// Geometry and the camera
// ---------------------------------------------------------------------------

// Frame a world point: as little off it as keeps it well on screen, and
// never past the board's edges (no empty carpet for nothing). The view is
// 128 x 118 below the HUD.
static void aimAt(int fx, int fy, uint8_t shift) {
    int x = fx, y = fy;
    // Content bounds: the board and its slab, and the tallest token on the
    // far tiles.
    int x0 = -N * hw() - 6, x1 = N * hw() + 6;
    int y0 = -zoomed(16), y1 = 2 * N * hh() + slab() + 10;
    if (x < x0 + 64) x = x0 + 64;
    if (x > x1 - 64) x = x1 - 64;
    if (y < y0 + 59) y = y0 + 59;
    if (y > y1 - 59) y = y1 - 59;
    aimX = (int16_t)x; aimY = (int16_t)y; aimShift = shift;
}

static void aimTile(uint8_t t, uint8_t shift) {
    int x, y;
    worldOf(t, x, y);
    aimAt(x, y - zoomed(4), shift);
}

static void snapCamera() {
    cx16 = aimX << 4; cy16 = aimY << 4;
    cam.x = aimX; cam.y = aimY;
}

static void stepCamera() {
    int32_t dx = (aimX << 4) - cx16, dy = (aimY << 4) - cy16;
    cx16 += dx >> aimShift; cy16 += dy >> aimShift;
    if (dx > -16 && dx < 16) cx16 = aimX << 4;
    if (dy > -16 && dy < 16) cy16 = aimY << 4;
    cam.x = (int)(cx16 >> 4);
    cam.y = (int)(cy16 >> 4);
}

// Everything held in world space keeps its place on the board.
void setZoom(uint8_t h) {
    uint8_t o = tileH;
    if (h == o) return;
    tileH = h;
    cx16 = cx16 * h / o; cy16 = cy16 * h / o;
    aimX = (int16_t)(aimX * h / o); aimY = (int16_t)(aimY * h / o);
}

void setFast(bool on) { fast = on; }
void lookAt(uint8_t t) { aimTile(t, 2); snapCamera(); }
bool busy() { return false; }
void begin() {}

void update() {
    stepCamera();
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------

// Where a seat's token stands on a tile: a slot each, two abreast and two
// deep, so nothing shuffles about when a tile fills up.
static void slot(uint8_t t, uint8_t seat, int &x, int &y) {
    if (corner(t)) place(t, seat & 1 ? 11 : 5, seat & 2 ? 12 : 6, x, y);
    else place(t, seat & 1 ? 6 : 2, seat & 2 ? 14 : 9, x, y);
}

static void drawToken(uint8_t seat, int x, int y, int lift) {
    const uint8_t *a = TOKEN[seat];
    int s = zscale(), sx = toScreenX(x), sy = toScreenY(y);
    if (sx < -24 || sx > 152 || sy < -8 || sy > 170) return;
    gfx_fillEllipse(sx, sy, zoomed(4), (tileH + 2) / 5, INK);   // its shadow stays on the board
    sprite4(a, sx - ((a[0] * s) >> 9), sy - ((a[1] * s) >> 8) - lift, RM_ID, s);
}

static void drawBuilding(uint8_t t, const uint8_t *a, int along) {
    int x, y, s = zscale();
    place(t, along, 2, x, y);
    sprite4(a, toScreenX(x) - ((a[0] * s) >> 9), toScreenY(y) + zoomed(1) - ((a[1] * s) >> 8), RM_ID, s);
}

// What stands on a tile, back to front: on the near sides the houses are
// behind the tokens, on the far sides in front of them.
static void drawOn(uint8_t t) {
    using namespace game;
    bool near = side(t) == 0 || side(t) == 3;
    uint8_t lv = level(t);
    for (uint8_t pass = 0; pass < 2; pass++) {
        if ((pass == 0) == near) {
            // Houses along the band, the far end first.
            if (lv == 5) drawBuilding(t, HOTEL, 4);
            else for (uint8_t i = 0; i < lv; i++) {
                uint8_t k = side(t) < 2 ? (uint8_t)(lv - 1 - i) : i;
                drawBuilding(t, HOUSE, 1 + k * 2);
            }
        } else {
            for (uint8_t i = 0; i < 4; i++) {
                uint8_t seat = near ? i : (uint8_t)(3 - i);
                if (seat >= st.players || st.pl[seat].pos != t) continue;
                int x, y;
                slot(t, seat, x, y);
                drawToken(seat, x, y, 0);
            }
        }
    }
}

static void drawStanding(uint32_t) {
    // By lattice depth: two tiles of the ring at each (one at the far and
    // near corners).
    for (int d = 0; d <= 2 * (N - 1); d++) {
        uint8_t a = (uint8_t)((60 - d) % 40), b = (uint8_t)((20 + d) % 40);
        drawOn(a);
        if (b != a) drawOn(b);
    }
}

static void drawOwners() {
    for (uint8_t t = 0; t < TILES; t++)
        if (isDeed(t) && game::owner(t) != game::BANK) rim(t, SEAT_COLOUR[game::owner(t)]);
}

static void drawHud(uint32_t) {
    using namespace game;
    gfx_fillRect(0, 0, 128, 9, INK);
    gfx_hline(0, 9, 128, GOLD);
    for (uint8_t p = 0; p < st.players; p++) {
        char buf[12];
        int x = p * 32;
        fmtMoney(buf, st.pl[p].cash);
        gfx_fillRect(x + 2, 3, 3, 3, SEAT_COLOUR[p]);
        text35(x + 7, 2, buf, p == st.cur ? FX_B : WHITE);
    }
}

void renderScene(uint32_t frame) {
    drawTable();
    drawBoard();
    drawOwners();
    drawStanding(frame);
}

void invalidate() {}

bool render(uint32_t frame, uint32_t) {
    renderScene(frame);
    drawHud(frame);
    fx::drawParticles((uint8_t)((2 * tileH + 2) / 5));
    fx::drawFloats();
    fx::drawBanner();
    fx::applyShake(10, 127);
    return true;
}

}  // namespace stage
