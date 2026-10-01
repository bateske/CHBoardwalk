// The play screen's presentation: the camera, the board with its houses and
// tokens, the dice, cards, coins in flight, the HUD and the plate.
//
// The game reports events; the stage turns them into motion and says busy()
// until it has shown them (CHBlackjack's Presenter, CHChess's Stage).
#pragma once
#include <stdint.h>

namespace stage {

extern const uint8_t SEAT_COLOUR[4];    // each seat's colour (its token's)

void begin();
void update();                       // drain game events, advance motion (once per tick)
bool busy();
// Draws the play scene; false if nothing changed since the last frame (the
// framebuffer still holds it). ui: a signature of what the caller draws on top.
bool render(uint32_t frame, uint32_t ui);
void invalidate();                   // redraw next frame
void setZoom(uint8_t tileH);         // iso zoom now, 5..10
void lookAt(uint8_t tile);           // the camera's aim
void setFast(bool on);               // QUICK pace

// The board, its buildings and tokens only (the title's backdrop).
void renderScene(uint32_t frame);

}  // namespace stage
