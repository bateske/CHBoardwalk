#pragma GCC optimize("Os")   // cold code: size over speed
#include "Cpu.h"
#include "Game.h"

namespace cpu {

using namespace game;
using namespace board;

// Cash kept back when buying, bids as a percentage of what a deed is worth
// to it, cash kept back when building.
struct Level { int16_t reserve; uint8_t bidPct; int16_t buildReserve; };
static const Level LEVEL[LEVELS] = {
    {400, 70, 500},     // careful: buys little, bids low, builds late
    {200, 100, 300},
    {50, 120, 150},     // a shark
};
static const Level &lv(uint8_t p) { return LEVEL[st.pl[p].kind - CPU]; }

// t's group without t: how many p holds, the group's size, whether another
// player holds all the rest (t would complete their set) and whether any
// other player is in it at all.
static void stake(uint8_t p, uint8_t t, uint8_t &mine, uint8_t &n, bool &blocks, bool &shared) {
    uint8_t tiles[4], other[SEATS] = {0, 0, 0, 0};
    n = groupTiles(group(t), tiles);
    mine = 0;
    blocks = shared = false;
    for (uint8_t i = 0; i < n; i++) {
        uint8_t o = owner(tiles[i]);
        if (tiles[i] == t || o == BANK) continue;
        if (o == p) { mine++; continue; }
        shared = true;
        if (++other[o] == n - 1) blocks = true;
    }
}

int valuation(uint8_t p, uint8_t t) {
    uint8_t mine, n;
    bool blocks, shared;
    stake(p, t, mine, n, blocks, shared);
    bool completes = mine + 1 == n;
    int pct;
    if (type(t) == STREET) pct = completes ? 160 : blocks ? 130 : mine ? 120 : 100;
    else pct = 100 + 15 * mine;                         // railroads and utilities: each is worth more than the last
    int v = price(t) * pct / 100 * lv(p).bidPct / 100;
    int cash = (int)st.pl[p].cash - (completes ? 0 : lv(p).reserve / 2);
    if (v > cash) v = cash;
    return v < 0 ? 0 : v;
}

bool wantsBuy(uint8_t p, uint8_t t) {
    uint8_t mine, n;
    bool blocks, shared;
    stake(p, t, mine, n, blocks, shared);
    return mine + 1 == n || blocks || st.pl[p].cash - price(t) >= lv(p).reserve;
}

// The house that adds the most rent for its cost.
int pickBuild(uint8_t p) {
    int best = -1, bestScore = 0;
    for (uint8_t t = 0; t < TILES; t++) {
        if (!canBuild(t) || st.pl[p].cash - houseCost(t) < lv(p).buildReserve) continue;
        const uint16_t *r = RENT[TILE[t].aux];
        int now = level(t) ? r[level(t)] : 2 * r[0];
        int score = (r[level(t) + 1] - now) * 100 / houseCost(t);
        if (score > bestScore) { best = t; bestScore = score; }
    }
    return best;
}

// With a full set it cannot afford to build on, a deed it will never make a
// set of (someone else is in that group) goes to auction: the cheapest. Not
// with a single bidder across the table, who would have it for nothing.
int pickOffer(uint8_t p) {
    bool need = false;
    if (st.players < 3) return -1;
    for (uint8_t t = 0; t < TILES && !need; t++)
        need = type(t) == STREET && owner(t) == p && level(t) < 5 && ownsGroup(p, group(t)) &&
               st.pl[p].cash < houseCost(t) + lv(p).buildReserve;
    if (!need) return -1;
    int best = -1;
    for (uint8_t t = 0; t < TILES; t++) {
        uint8_t mine, n;
        bool blocks, shared;
        if (!canOffer(t)) continue;
        stake(p, t, mine, n, blocks, shared);
        if (shared && (best < 0 || price(t) < price((uint8_t)best))) best = t;
    }
    return best;
}

// The card if it has one; early on, with deeds still to be had, it pays to
// get out and about; later, jail is the safest square on the board.
uint8_t jailChoice(uint8_t p) {
    if (st.pl[p].flags & (F_CARD | F_CARD << 1)) return 2;
    uint8_t unowned = 0;
    for (uint8_t t = 0; t < TILES; t++) unowned += isDeed(t) && owner(t) == BANK;
    return unowned > 6 && st.pl[p].cash >= 50 + lv(p).reserve ? 1 : 0;
}

}  // namespace cpu
