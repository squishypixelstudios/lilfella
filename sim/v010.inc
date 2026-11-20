// v0.10: tests and pictures for POP (the bubble shooter), the scrub that keeps its progress, RUN's card and
// power-ups, the icon touch fix, and the wiki's pictures.
namespace lf {
struct V010 {
  // a grown fella on the home screen, awake, nothing going on (age in seconds; looks as Game::packLooks)
  static void grown(Game& g, SimPlat& p, float age, uint8_t looks) {
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); g.simulate(70, false); run(g, p, 3);
    PetData& d = g.pet.d; d.pad2 = 0xE0FF; d.nextSick = 1e9f; d.sick[3] |= SF_BUG_DONE; memset(d.sick, 0, 3); d.pad0[0] = looks; d.age = age;
    d.hunger = d.fun = 80; d.health = 100; d.digestTimer = 1e6f; d.droppings = 0; d.floorMess = 0;
    g.scr = S_MAIN; g.pet.sleeping = false; g.pet.nap = 0; g.lastForm_ = g.form(); g.endNewborn(); run(g, p, 1);
  }
  // every 4th frame of the Lil Blob's meal: ./sim babyeat <dir>
  static int babyEat(const char* dir) {
    outDir = dir;
    for (int side = 0; side < 2; side++) {
      SimPlat p; p.ms = 99; Game g(&p); g.begin(); grown(g, p, 300, 0);
      g.blobX_ = g.blobTx_ = side ? 26.0f : 6.0f; g.pet.d.hunger = 20;
      g.debugAct(0);
      for (int i = 0; g.scr == S_ANIM && i < 30 * 12; i++) { step(g, p); if (i % 4 == 0) { char nm[24]; snprintf(nm, sizeof nm, "eat%d_%03d", side, i); shot(g, nm); } }
    }
    return 0;
  }

  // ---- POP ----
  static void popOpen(Game& g, SimPlat& p, bool empty) {   // a fresh game, the card already gone (empty: no bubbles on the field)
    g.debugAct(-1); run(g, p, 0.2f); g.pet.sleeping = false;
    g.debugAct(13); step(g, p); g.pop_.card = 0; step(g, p);
    if (empty) memset(g.pop_.g, 0, sizeof g.pop_.g);
  }
  static bool popAimAt(const Game& g, int r, int c, float& ang) {   // an aim that lands in (r, c), the straightest there is
    bool found = false;
    for (int i = -200; i <= 200; i++) {
      const float a = i * (POP_MAX_ANG / 200.0f);
      int rr, cc;
      if (g.popTrace(a, rr, cc) && rr == r && cc == c && (!found || fabsf(a) < fabsf(ang))) { ang = a; found = true; }
    }
    return found;
  }
  // ./sim pop
  static int pop() {
    int fails = 0;
    auto check = [&](bool ok, const char* what) { printf("  %-66s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
    SimPlat p; p.ms = 777; Game g(&p); g.begin(); grown(g, p, TEEN_END + 60, 37);
    Game::Pop& P = g.pop_;
    {   // from the Play menu, the card, the starting field
      tapIcon(g, p, IC_PLAY); run(g, p, 0.2f); tapDot(g, p, 30, 20); run(g, p, 0.2f);
      check(g.scr == S_POP && P.card > 0, "Play > POP opens on the how-to card");
      const int n = g.popCount();
      check(n >= 23 && n <= 23 + POP_START_EXTRA && n == P.start, "the field: three full rows (8 + 7 + 8) and a few hanging under them");
      bool valid = true, curOn = false, nextOn = false;
      for (int r = 0; r < POP_ROWS; r++) for (int c = 0; c < POP_COLS; c++) {
        const int k = P.g[r][c];
        if (k >= PK_COUNT || (c >= Game::popCols(r) && k) || (r > POP_START_ROWS && k)) valid = false;
        if (k == P.cur) curOn = true;
        if (k == P.next) nextOn = true;
      }
      check(valid, "every bubble is one of the five kinds, in its place");
      check(curOn && nextOn, "the launcher's bubble and the next one are kinds that are on the field");
      step(g, p, true, 92, MY + 60); step(g, p, true, 92, MY + 60); step(g, p); step(g, p);
      check(P.card <= 0 && P.phase == 1 && P.shots == 0, "a tap puts the card away (and doesn't fire)");
      g.debugAct(-1); run(g, p, 0.2f); g.debugAct(13); run(g, p, POP_CARD_SECS + 0.2f);
      check(g.scr == S_POP && P.card <= 0 && P.phase == 1, "left alone, the card goes by itself");
    }
    {   // the five kinds look different from each other
      popOpen(g, p, true);
      for (int k = 1; k < PK_COUNT; k++) P.g[0][k - 1] = (uint8_t)k;
      step(g, p);
      bool distinct = true; int lit[PK_COUNT] = {0};
      for (int a = 1; a < PK_COUNT; a++) for (int b = a + 1; b < PK_COUNT; b++) {
        int diff = 0;
        for (int y = 0; y < 5; y++) for (int x = 0; x < 5; x++) if (g.dots()[y * MW + (a - 1) * 5 + x] != g.dots()[y * MW + (b - 1) * 5 + x]) diff++;
        if (diff < 8) distinct = false;
      }
      for (int k = 1; k < PK_COUNT; k++) for (int y = 0; y < 5; y++) for (int x = 0; x < 5; x++) if (g.dots()[y * MW + (k - 1) * 5 + x]) lit[k]++;
      check(distinct, "X, triangle, diamond, empty and black: any two differ in 8 dots or more");
      check(lit[PK_BLACK] == 21 && lit[PK_EMPTY] == 12, "black is filled right in; empty is only a ring");
    }
    {   // a shot straight up, into an empty field: it sticks to the ceiling
      popOpen(g, p, true); P.cur = PK_DIAMOND;
      popShoot(g, p, 0.0f);
      check(g.popCount() == 1 && P.g[0][3] == PK_DIAMOND && P.phase == 1 && P.shots == 1, "a shot straight up sticks to the ceiling above the launcher");
    }
    {   // three of a kind pop; the field is clear: a win, 4 hearts
      popOpen(g, p, true); P.g[0][3] = P.g[0][4] = PK_X; P.cur = PK_X; P.start = 2;
      const float fun0 = (g.pet.d.fun = 20); const uint32_t games0 = g.sv.lifeGames;
      popShoot(g, p, 0.0f);
      check(g.popCount() == 0 && P.popped == 3, "a third X beside two X pops all three");
      check(P.phase == 3 && P.won && P.hearts == 4, "the field is clear: won, 4 hearts (the ceiling never moved)");
      check(g.pet.d.fun > fun0 + 20 && g.sv.lifeGames == games0 + 1, "a win makes them happy, and counts as one game");
      run(g, p, 2.6f); check(g.scr == S_POP && P.phase == 4, "then the result card");
      tapDot(g, p, 20, 14); run(g, p, 0.2f); check(g.scr == S_MAIN, "a tap puts the result away");
    }
    {   // a different kind doesn't pop
      popOpen(g, p, true); P.g[0][3] = P.g[0][4] = PK_X; P.cur = PK_BLACK;
      popShoot(g, p, 0.0f);
      check(g.popCount() == 3 && P.g[1][3] == PK_BLACK && P.popped == 0 && P.phase == 1, "a black one under two X just sticks there (the row below, half a bubble along)");
    }
    {   // two of a kind aren't enough
      popOpen(g, p, true); P.g[0][3] = PK_X; P.g[0][0] = PK_EMPTY; P.cur = PK_X;
      popShoot(g, p, 0.0f);
      check(g.popCount() == 3 && P.popped == 0, "two of a kind stay");
    }
    {   // what hung from the popped ones falls
      popOpen(g, p, true);
      P.g[0][0] = PK_EMPTY; P.g[0][3] = P.g[0][4] = PK_X; P.g[1][3] = PK_BLACK; P.g[2][3] = P.g[2][4] = PK_DIAMOND; P.cur = PK_X;
      float a = 0;
      const bool can = popAimAt(g, 1, 2, a);
      check(can, "there's an aim that lands beside the X pair");
      if (can) popShoot(g, p, a);
      check(g.popCount() == 1 && P.g[0][0] == PK_EMPTY && P.popped == 6, "the X trio pops, and the three hanging under it drop off");
      check(P.phase == 1 && !P.won, "...and the game goes on (one bubble is left)");
      check(P.cur == PK_EMPTY, "the launcher's next bubble is a kind still on the field");
    }
    {   // off the walls
      popOpen(g, p, true); P.cur = PK_TRIANGLE;
      int r = 0, c = 0, b = 0;
      const bool ok = g.popTrace(1.2f, r, c, &b);
      check(ok && b >= 1 && r == 0, "a low shot bounces off the side wall on its way to the ceiling");
      popShoot(g, p, Game::popAngleAt(popAimX(1.2f)));
      int rr = 0, cc = 0; (void)rr; (void)cc;
      check(g.popCount() == 1, "...and lands on the field");
      bool inside = true;
      popOpen(g, p, false);
      for (int i = -30; i <= 30; i++) { int r2, c2; if (!g.popTrace(i * POP_MAX_ANG / 30.0f, r2, c2) || r2 < 0 || r2 >= POP_ROWS || c2 < 0 || c2 >= Game::popCols(r2) || P.g[r2][c2]) inside = false; }
      check(inside, "every aim lands in a free spot on the field");
      // the shot goes where the guide said: the cell traced for the aim the finger gives is the cell that fills
      bool same = true;
      for (int i = -6; i <= 6; i++) {
        popOpen(g, p, false);
        const float a = Game::popAngleAt(popAimX(i * 0.2f));
        int r2, c2; g.popTrace(a, r2, c2);
        const uint8_t cur = P.cur; const int n0 = g.popCount(), pop0 = P.popped;
        popShoot(g, p, a);
        if (P.popped == pop0 && (g.popCount() != n0 + 1 || P.g[r2][c2] != cur)) same = false;
      }
      check(same, "a shot lands in the spot the dotted guide pointed at");
    }
    {   // the ceiling
      popOpen(g, p, true); P.g[0][0] = PK_EMPTY;
      static const uint8_t K[4] = {PK_X, PK_TRIANGLE, PK_DIAMOND, PK_BLACK};
      bool early = false;
      for (int i = 0; i < POP_DROP_EVERY; i++) { if (P.drops) early = true; P.cur = K[i % 4]; P.phase = 2; if (i < 7) g.popLand(0, 1 + i); else g.popLand(1, i - 7); }
      check(!early && P.drops == 1 && P.sinceDrop == 0 && g.popCellY(0) == 5, "after POP_DROP_EVERY shots the ceiling comes down one row");
      check(P.phase == 1, "...and play goes on");
      step(g, p);
      int wall = 0; for (int x = 0; x < MW; x++) if (g.dots()[4 * MW + x] == SH_INK) wall++;
      check(wall == MW, "the ceiling is drawn where it has come down to");
    }
    {   // below the line
      popOpen(g, p, true); P.g[0][3] = PK_X; P.start = 1; P.drops = POP_SAFE_ROWS - 1; P.cur = PK_BLACK; P.phase = 2;
      const float fun0 = (g.pet.d.fun = 20);
      g.popLand(1, 3);
      check(P.phase == 3 && !P.won && P.hearts == 0, "a bubble below the line ends the game: lost, no hearts");
      check(g.pet.d.fun > fun0 && g.pet.d.fun < fun0 + 6, "a loss is still a little fun");
      run(g, p, 2.6f); check(P.phase == 4, "then the result card");
      tapDot(g, p, 20, 14); run(g, p, 0.2f); check(g.scr == S_MAIN, "a tap puts it away");
    }
    {   // hearts
      popOpen(g, p, true); P.g[0][3] = P.g[0][4] = PK_X; P.cur = PK_X; P.drops = 1; P.phase = 2; g.popLand(1, 3);
      check(P.won && P.hearts == 3, "cleared after the ceiling came down once: 3 hearts");
      popOpen(g, p, true); P.g[0][3] = P.g[0][4] = PK_X; P.cur = PK_X; P.drops = 2; P.phase = 2; g.popLand(1, 3);
      check(P.won && P.hearts == 2, "cleared after two drops: 2 hearts");
    }
    {   // leaving
      popOpen(g, p, false);
      { bool icons = false; step(g, p); for (int y = 0; y < POP_MY - 6; y++) for (int x = 0; x < W / 2; x++) if (fb.px[y * W + x] != SH_BG) icons = true;   // (top left: where Feed and the Light were)
        for (int y = POP_MY + POP_MH * DOT + 4; y < H; y++) for (int x = 0; x < W; x++) if (fb.px[y * W + x] != SH_BG) icons = true;
        check(!icons, "POP has the whole screen: no icons above or below its LCD"); }
      { const int n0 = P.shots; tap(g, p, 40, 200); run(g, p, 1.5f); check(g.scr == S_POP && P.shots == n0 + 1, "a touch down where the bottom icons were only aims and fires"); }
      popOpen(g, p, false); tap(g, p, 145, 15); run(g, p, 0.2f);
      check(g.scr == S_MAIN, "the BACK button (top right) leaves POP");
      popOpen(g, p, false); run(g, p, GAME_IDLE_SECS + 1.0f);
      check(g.scr == S_MAIN, "left untouched for 30 s, it's put away");
    }
    // whole games: a sure hand, a shaky one, and someone firing anywhere
    static const struct { const char* who; float wobble; } BOT[3] = {{"sure hand", 0.0f}, {"shaky hand (+-7 deg)", 0.12f}, {"firing anywhere", 3.0f}};
    for (int b = 0; b < 3; b++) {
      int won = 0, hearts[5] = {0}, shots = 0, games = 60; bool sane = true, kindOk = true;
      for (int s = 0; s < games; s++) {
        g.pet.rng = 99991u * (s + 1) + b; popOpen(g, p, false);
        uint32_t rs = s * 7919u + b;
        for (int n = 0; g.scr == S_POP && P.phase == 1 && n < 400; n++) {
          bool on = false;
          for (int r = 0; r < POP_ROWS; r++) for (int c = 0; c < Game::popCols(r); c++) if (P.g[r][c] == P.cur) on = true;
          if (!on) kindOk = false;
          popShoot(g, p, popBotAngle(g, BOT[b].wobble, rs));
          if (P.phase == 2 || g.popCount() > POP_ROWS * POP_COLS || P.drops > POP_SAFE_ROWS) sane = false;
        }
        if (P.phase != 3) sane = false;
        if (P.won) won++;
        if (P.hearts >= 0 && P.hearts <= 4) hearts[P.hearts]++; else sane = false;
        shots += P.shots;
      }
      printf("  %-22s won %2d of %d, %4.1f shots a game, hearts 0-4: %d %d %d %d %d\n", BOT[b].who, won, games, shots / (float)games, hearts[0], hearts[1], hearts[2], hearts[3], hearts[4]);
      char what[96];
      snprintf(what, sizeof what, "%s: every game ends, nothing out of bounds", BOT[b].who); check(sane, what);
      snprintf(what, sizeof what, "%s: the launcher only holds kinds that are on the field", BOT[b].who); check(kindOk, what);
      if (b == 0) check(won >= games * 9 / 10, "a sure hand nearly always clears the field");
      if (b == 2) check(won < games / 2, "firing anywhere mostly loses");
    }
    {   // mashing the screen
      bool ok = true;
      for (int s = 0; s < 30; s++) {
        g.pet.rng = 31337u * (s + 1); g.debugAct(-1); run(g, p, 0.2f); g.debugAct(13);
        uint32_t rs = s * 2654435761u + 1;
        for (int i = 0; i < 30 * 40 && g.scr == S_POP; i++) {
          rs = rs * 1664525u + 1013904223u;
          const bool down = (rs >> 20) % 3 != 0;
          step(g, p, down, (int)((rs >> 8) % W), MY + (int)((rs >> 3) % 112));
          if (g.popCount() > POP_ROWS * POP_COLS || P.phase < 1 || P.phase > 4) ok = false;
        }
      }
      check(ok, "mashing: nothing breaks");
    }
    return fails ? (printf("pop: %d FAILED\n", fails), 1) : (printf("pop: all ok\n"), 0);
  }
  // ./sim popshots <dir>
  static int popShots(const char* dir) {
    outDir = dir;
    SimPlat p; p.ms = 4321; Game g(&p); g.begin(); grown(g, p, TEEN_END + 60, 37);
    Game::Pop& P = g.pop_;
    g.pet.rng = 2024; g.debugAct(13); run(g, p, 0.5f); shot(g, "pop_card");
    P.card = 0; step(g, p); shot(g, "pop_start");
    step(g, p, true, 60, MY + 60); step(g, p, true, 60, MY + 60); shot(g, "pop_aim_left");
    step(g, p, true, 160, MY + 60); step(g, p, true, 160, MY + 60); shot(g, "pop_aim_bank");
    uint32_t rs = 5;
    const int x = popAimX(popBotAngle(g, 0, rs));
    step(g, p, true, x, MY + 60); step(g, p, true, x, MY + 60); shot(g, "pop_aim_best"); step(g, p);
    for (int i = 0; i < 6; i++) step(g, p);
    shot(g, "pop_flight");
    for (int i = 0; i < 40 && P.phase == 2; i++) step(g, p);
    step(g, p); step(g, p); shot(g, "pop_popping"); run(g, p, 0.2f); shot(g, "pop_falling"); run(g, p, 0.6f); shot(g, "pop_after");
    P.sinceDrop = POP_DROP_EVERY - 1; P.cur = PK_BLACK; popShoot(g, p, -0.9f); step(g, p); shot(g, "pop_ceiling");
    popPlay(g, p); run(g, p, 0.5f); shot(g, "pop_over"); run(g, p, 2.0f); shot(g, "pop_result");
    g.debugAct(-1); run(g, p, 0.3f);
    g.debugAct(11); run(g, p, 0.5f); shot(g, "run_card");
    return 0;
  }

  // ---- the rest of v0.10: ./sim v010 ----
  static int lit(const Game& g, int x0, int y0, int w, int h) { int n = 0; for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) if (g.dots()[y * MW + x]) n++; return n; }
  static int checks() {
    int fails = 0;
    auto check = [&](bool ok, const char* what) { printf("  %-74s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; };
    SimPlat p; p.ms = 4242; Game g(&p); g.begin(); grown(g, p, TEEN_END + 60, 37);
    PetData& d = g.pet.d;
    const int medsX = iconX(IC_MEDS) + 8, meterX = iconX(IC_METER) + 8, by = iconY(IC_METER) + 8;
    {   // icons: judged by where the finger rests, not where it first touched
      auto press = [&](int firstX, int restX, int frames) { step(g, p, true, firstX, by); for (int i = 0; i < frames; i++) step(g, p, true, restX, by); step(g, p); step(g, p); };
      const int s = meterX > medsX ? 1 : -1;   // (which side of Medicine the Meter is on: they have swapped places since)
      press(medsX + 14 * s, meterX - 4 * s, 4);   // lands on Medicine's side of the line, rolls onto Meter
      check(g.scr == S_METER, "a finger that lands a little toward Medicine and settles on Meter opens the Meter");
      g.debugAct(-1); run(g, p, 0.3f);
      press(meterX, meterX, 2);
      check(g.scr == S_METER, "a plain tap on Meter opens the Meter");
      g.debugAct(-1); run(g, p, 0.3f);
      press(medsX, medsX, 2);
      check(g.scr == S_ANIM && g.anim() == AN_REFUSE, "a plain tap on Medicine is still Medicine (refused: not sick)");
      run(g, p, 2.0f); g.debugAct(-1); run(g, p, 0.3f);
      press(meterX - 14 * s, medsX + 4 * s, 4);
      check(g.scr == S_ANIM && g.anim() == AN_REFUSE, "...and one that settles on Medicine is Medicine");
      run(g, p, 2.0f); g.debugAct(-1); run(g, p, 0.3f);
    }
    {   // the Meter's first page: battery as a number only
      tapIcon(g, p, IC_METER); run(g, p, 0.2f);
      int longest = 0;
      for (int y = 0; y < MH; y++) { int runLen = 0; for (int x = 0; x < MW; x++) { runLen = g.dots()[y * MW + x] ? runLen + 1 : 0; if (runLen > longest) longest = runLen; } }
      check(g.scr == S_METER && longest < 8, "Meter page 1: no battery bar (no long line of dots anywhere)");
      check(lit(g, 0, 18, MW, 5) > 10, "...the battery percentage is still there");
      g.debugAct(-1); run(g, p, 0.3f);
    }
    {   // Settings > Sound steps down
      g.debugAct(7); run(g, p, 0.3f);
      int seen[8], n = 0;
      seen[n++] = g.sv.volume;
      for (int i = 0; i < 6; i++) { tap(g, p, 92, SETTINGS_Y0 + 12); seen[n++] = g.sv.volume; }
      check(seen[0] == 2, "a new save's volume is 2");
      check(seen[1] == 1 && seen[2] == 0 && seen[3] == 5 && seen[4] == 4 && seen[5] == 3 && seen[6] == 2, "each tap on Sound is one step quieter: 2, 1, 0, then round to 5, 4, 3, 2");
      g.debugAct(-1); run(g, p, 0.3f);
    }
    {   // the scrub keeps its progress
      d.droppings = 3; d.floorMess = 1; g.pet.rng = 777;
      tapIcon(g, p, IC_LITTER); run(g, p, 0.3f);
      check(g.scr == S_SCRUB && g.sc_.np == 4, "the scrub opens: 3 in the box, 1 on the floor");
      for (int i = 0; i < 50; i++) { const int k = i % 16, x = MX + 20 + (k < 8 ? k : 16 - k) * 16; step(g, p, true, x, MY + 60); }
      step(g, p);
      const Game::Scrub before = g.sc_;
      float worn = 0; for (int i = 0; i < before.np; i++) worn += before.poo[i].hp0 - before.poo[i].hp;
      check(worn > 50 && !before.done, "some scrubbing done, not finished");
      tapIcon(g, p, IC_LITTER); run(g, p, 0.3f);
      check(g.scr == S_MAIN && d.droppings == 3 && d.floorMess == 1, "leaving early: nothing's cleaned yet");
      run(g, p, 5.0f);
      tapIcon(g, p, IC_LITTER); run(g, p, 0.1f);
      bool same = g.scr == S_SCRUB && g.sc_.np == before.np;
      for (int i = 0; same && i < before.np; i++) { const auto& a = before.poo[i]; const auto& b = g.sc_.poo[i]; if (a.x != b.x || a.y != b.y || a.shape != b.shape || a.tough != b.tough || fabsf(a.hp - b.hp) > 0.01f) same = false; }
      check(same, "coming back: the same poops in the same places, worn down exactly as far");
      // left alone until the game puts it away, then another poop arrives
      run(g, p, GAME_IDLE_SECS + 1.0f);
      check(g.scr == S_MAIN, "left untouched, the scrub is put away");
      if (!g.displayOn()) tapDot(g, p, 20, 14);   // (with the motion sensor off the screen has gone dark by now: the first touch only wakes it)
      d.droppings = 4;
      tapIcon(g, p, IC_LITTER); run(g, p, 0.1f);
      bool kept = g.scr == S_SCRUB && g.sc_.np == 5;
      for (int i = 0; kept && i < 3; i++) { const auto& a = before.poo[i]; const auto& b = g.sc_.poo[i]; if (a.shape != b.shape || a.tough != b.tough || fabsf(a.hp / a.hp0 - b.hp / b.hp0) > 0.01f) kept = false; }
      check(kept, "a poop done in between joins in; the others keep their wear");
      check(g.sc_.poo[3].hp == g.sc_.poo[3].hp0, "...and the new one is untouched");
      scrubSolve(g, p); run(g, p, 2.5f);
      check(d.droppings == 0 && d.floorMess == 0 && g.scr == S_MAIN, "scrubbed to the end: all clean");
      d.droppings = 2; tapIcon(g, p, IC_LITTER); run(g, p, 0.1f);
      bool fresh = g.scr == S_SCRUB && g.sc_.np == 2; for (int i = 0; fresh && i < 2; i++) if (g.sc_.poo[i].hp != g.sc_.poo[i].hp0) fresh = false;
      check(fresh, "the next mess starts fresh");
      g.debugAct(-1); run(g, p, 0.3f); d.droppings = 0;
    }
    {   // YEET: squashed or stretched, the whole outline is drawn
      bool ok = true; int worst = 0;
      static const struct { float age; uint8_t looks; } WHO[4] = {{600, 0}, {BABY_END + 60, 2}, {CHILD_END + 60, Game::packLooks(1, 1, 0)}, {TEEN_END + 60, 37}};
      for (int w = 0; w < 4; w++) {
        d.age = WHO[w].age; d.pad0[0] = WHO[w].looks; g.lastForm_ = g.form();
        const Sprite& sp = SPRITES[g.gameSprite()];
        int bottom = 0; for (int c = 0; c < sp.w; c++) if (sp.px[(sp.h - 1) * sp.w + c]) bottom++;
        for (int k = 0; k <= 10; k++) {
          const float sy = 0.5f + 0.05f * k, sx = 1.0f / sy > 1.5f ? 1.5f : 1.0f / sy;
          g.m_.clear(); g.drawYeetFella(20.0f, 14.0f, 0, sx, sy, false, 0);
          int last = -1; for (int y = MH - 1; y >= 0 && last < 0; y--) for (int x = 0; x < MW; x++) if (g.dots()[y * MW + x]) { last = y; break; }
          int n = 0; for (int x = 0; x < MW && last >= 0; x++) if (g.dots()[last * MW + x]) n++;
          if (n < bottom) { ok = false; if (bottom - n > worst) worst = bottom - n; }
        }
      }
      check(ok, "YEET: drawn shorter than they are (stretched in flight, squashed on a bounce), the bottom row is all there");
      // and in a real flight: a Lil Blob, every frame, bottom row never above the ground while it rests on it
      d.age = 600; d.pad0[0] = 0; g.lastForm_ = g.form(); 
      g.debugAct(14); run(g, p, 0.4f); tapDot(g, p, 20, 14); run(g, p, 0.3f);
      step(g, p, true, MX + 100, MY + 50); for (int i = 1; i <= 15; i++) step(g, p, true, MX + 100 - i * 9, MY + 50 + i * 7); step(g, p);
      bool flew = g.yt_.phase == 2, cut = false;
      for (int i = 0; i < 30 * 30 && g.yt_.phase == 2; i++) { step(g, p); if (g.yt_.gyScr >= 0 && g.yt_.gyScr < MH && g.yt_.drawBot >= g.yt_.gyScr) cut = true; if (g.yt_.gyScr >= MH && g.yt_.drawBot > MH - 1) cut = true; }
      check(flew && !cut, "YEET: a Lil Blob's whole flight, never drawn into the ground or off the bottom");
      g.debugAct(-1); run(g, p, 0.3f);
      d.age = TEEN_END + 60; d.pad0[0] = 37; g.lastForm_ = g.form();
    }
    {   // RUN: the card, and power-ups that move about
      g.debugAct(11); run(g, p, 0.3f);
      check(g.scr == S_RUN && g.rn_.card > 0 && g.rn_.dist == 0, "RUN opens on its how-to card (nothing moves yet)");
      check(lit(g, 0, 0, MW, MH) > 120, "...with the words on it (tap = jump, tap in air = double jump)");
      tapDot(g, p, 20, 14);
      check(g.rn_.card <= 0 && g.rn_.phase == 1 && g.rn_.y <= 0.01f, "a tap puts the card away (and isn't a jump)");
      run(g, p, 0.5f); check(g.rn_.dist > 0, "then the run starts");
      g.debugAct(-1); run(g, p, 0.3f); g.debugAct(11); run(g, p, RUN_CARD_SECS + 0.3f);
      check(g.rn_.card <= 0 && g.rn_.dist > 0, "left alone, the card goes by itself");
      g.debugAct(-1); run(g, p, 0.3f);
      // 40 runs of 40 s each (bumping into things doesn't stop these): where and when the power-ups turn up
      int heights[32] = {0}, n = 0, firstMin = 9999, firstMax = 0, xs[16] = {0}; bool reach = true;
      const int h = spriteH(g.gameSprite());
      for (int s = 0; s < 40; s++) {
        g.pet.rng = 1234567u * (s + 1) + 1; 
        Game::Run& r = g.rn_; memset(&r, 0, sizeof r); r.next = 40; r.phase = 1; r.wallDone = 1;   // (nobody jumps here: without this every later thing would be the wall the 4th heart needs)
        bool was[6] = {false}; int first = -1;
        for (int f = 0; f < 30 * 40; f++) {
          g.runStep(1.0f / 30, false, false);
          if (r.phase != 1) { r.phase = 1; r.y = 0; r.vy = 0; r.t = (f + 1) / 30.0f; }
          r.boost = 0;
          for (int k = 0; k < 6; k++) {
            if (r.ob[k].on && !was[k] && r.ob[k].kind == 6) {
              if (first < 0) first = f;
              if (r.ob[k].by < 32) heights[r.ob[k].by]++;
              const int off = (int)r.ob[k].x - MW; if (off >= 0 && off < 16) xs[off]++;
              if (r.ob[k].by > h + 7 || r.ob[k].by + 5 > MH - 1) reach = false;
              n++;
            }
            was[k] = r.ob[k].on;
          }
        }
        if (first >= 0) { if (first < firstMin) firstMin = first; if (first > firstMax) firstMax = first; }
      }
      int kinds = 0, low = 0, mid = 0, high = 0, xk = 0;
      for (int i = 0; i < 32; i++) if (heights[i]) { kinds++; if (i <= 1) low += heights[i]; else if (i <= h + 3) mid += heights[i]; else high += heights[i]; }
      for (int i = 0; i < 16; i++) if (xs[i]) xk++;
      printf("  power-ups in 40 runs: %d (on the ground %d, a jump up %d, high up %d), %d different heights, %d different distances out; the first came between %.1f and %.1f s\n", n, low, mid, high, kinds, xk, firstMin / 30.0, firstMax / 30.0);
      check(n >= 80 && low > 0 && mid > 0 && high > 0 && kinds >= 5, "RUN: power-ups come at every height: on the ground, a jump up, high up");
      check(xk >= 8, "RUN: ...and at different distances out");
      check(firstMax - firstMin > 30 * 8, "RUN: the first one comes early in some runs, late in others");
      check(reach, "RUN: every one is on the screen and within a jump's reach");
      g.debugAct(-1); run(g, p, 0.3f);
    }
    {   // the look picker: no row of dots under the looks
      d.age = BABY_END + 60; d.pad0[0] = 0; g.lastForm_ = g.form(); g.scr = S_MAIN; g.pet.sleeping = false;
      for (int i = 0; i < 30 * 6 && g.scr != S_EVOLVE; i++) step(g, p);
      run(g, p, 0.6f);
      check(g.scr == S_EVOLVE, "the look picker opens");
      check(lit(g, 0, MH - 1, MW, 1) == 0, "nothing on the bottom row under the looks (the dots are gone)");
      const int sel0 = g.evoSel_; tapDot(g, p, 37, 14);
      check(g.evoSel_ != sel0, "the arrows still flip through the looks");
      tapDot(g, p, 20, 14); run(g, p, 3.0f);
      check(g.scr == S_MAIN && stageOfForm(g.form()) == 1, "tapping the look still picks it");
    }
    {   // Auto off is a plain timer (v1.0.1: no motion sensor at all), and the ending needs no shakes
      SimPlat p3; p3.ms = 2222; Game h(&p3); h.begin(); grown(h, p3, TEEN_END + 60, 37);
      p3.autoOff = true;
      check(h.sv.eq[2] == 3, "a new save: auto off is 30 Sec");
      h.sv.eq[2] = 0;   // auto off: 5 Sec
      h.wake(); run(h, p3, 4.0f);
      check(h.displayOn(), "auto off: the screen stays on for the first seconds untouched");
      run(h, p3, 2.0f);
      check(!h.displayOn(), "auto off: untouched for 5 s, the screen goes off (time alone decides)");
      tap(h, p3, 92, 100); run(h, p3, 0.2f);
      check(h.displayOn() && h.scr == S_MAIN, "auto off: a tap turns it back on");
      h.pet.d.hunger = 30; tapIcon(h, p3, IC_FEED); run(h, p3, 0.3f);
      bool dark = false; int n = 0;
      for (; h.scr == S_ANIM && n < 30 * 20; n++) { step(h, p3); if (!h.displayOn()) dark = true; }
      check(n > 30 * 5 && !dark && h.scr == S_MAIN, "auto off: a meal longer than 5 s plays to the end with the screen on");
      h.sv.eq[2] = 1; h.wake(); run(h, p3, 20.0f);
      check(h.displayOn(), "auto off: Never still keeps it on");
      {   // an old save on the 5 Sec default is moved to 30 Sec, once; choosing 5 Sec afterwards sticks
        SimPlat q; Game a(&q); a.begin(); grown(a, q, 600, 0);
        a.sv.eq[2] = 0; a.sv.fixes &= (uint8_t)~FIX_AUTOOFF; a.saveNow();
        Game b(&q); b.begin();
        check(b.sv.eq[2] == 3, "an old save on 5 Sec comes up as 30 Sec");
        b.sv.eq[2] = 0; b.saveNow();
        Game c(&q); c.begin();
        check(c.sv.eq[2] == 0, "5 Sec picked afterwards is kept");
      }
      // the ending: three squeezes, the dropper rises and the name page opens by itself
      h.sv.eq[2] = 1; h.pet.d.health = 0.5f; h.pet.d.sick[SICK_BUG] = 3; h.simulate(3 * 3600, false); run(h, p3, 0.4f);
      check(h.scr == S_ENDING, "an ending starts");
      run(h, p3, ENDING_WAIT_AT + 0.3f);
      for (int i = 0; i < ENDING_PULLS; i++) { for (int k = 0; k < 4; k++) step(h, p3, true, MX + 80, MY + 50); step(h, p3); run(h, p3, 0.8f); }
      run(h, p3, ENDING_UP_AT + 0.3f);
      check(h.scr == S_SETUP_NAME, "after the three squeezes the name page opens, with no shakes or extra tap");
    }
    {   // the Lil Blob's meal: one unbroken shot
      SimPlat p2; p2.ms = 99; Game h(&p2); h.begin(); grown(h, p2, 300, 0);
      for (int side = 0; side < 2; side++) {
        h.blobX_ = h.blobTx_ = side ? 26.0f : 6.0f; h.pet.d.hunger = 20;
        h.debugAct(0);
        int prev = -1, maxJump = 0, frames = 0, minLit = 9999; bool edge = false; float prevCx = -1, maxSlide = 0;
        for (int i = 0; h.scr == S_ANIM && i < 30 * 12; i++) {
          h.idleSec = 0;   // (the screen stays on: with the motion sensor off it would go dark after 15 s untouched, and the meal with it)
          step(h, p2);
          // where the picture's weight is: it never leaps across, not even on the frame that hands over to the home screen
          { long sx = 0; int n = 0; for (int y = 0; y < MH; y++) for (int x = 0; x < MW; x++) if (h.dots()[y * MW + x]) { sx += x; n++; }
            const float cx = n ? (float)sx / n : prevCx; if (prevCx >= 0 && fabsf(cx - prevCx) > maxSlide) maxSlide = fabsf(cx - prevCx); prevCx = cx; }
          if (h.scr != S_ANIM) break;
          const int n = lit(h, 0, 0, MW, MH);
          if (prev >= 0 && abs(n - prev) > maxJump) maxJump = abs(n - prev);
          if (n < minLit) minLit = n;
          for (int y = 0; y < MH; y++) if (h.dots()[y * MW] && h.dots()[y * MW + MW - 1]) edge = true;   // (the old zoom drew a frame right across)
          prev = n; frames++;
        }
        char what[160];
        snprintf(what, sizeof what, "Lil Blob eating (food on the %s): no cut (never more than 20 dots change at once)", side ? "left" : "right");
        check(frames > 100 && frames < 30 * 8 && maxJump <= 20 && !edge && minLit > 15, what);
        snprintf(what, sizeof what, "Lil Blob eating (food on the %s): nothing jumps to the other side, not for one frame (most it shifts: %.1f dots)", side ? "left" : "right", maxSlide);
        check(maxSlide < 4.0f, what);
        check((side ? h.blobX_ < 12 : h.blobX_ > 20) && h.scr == S_MAIN, "and they are left standing where they ate, on the food's side");
        run(h, p2, 0.5f);
      }
    }
    return fails ? (printf("v010: %d FAILED\n", fails), 1) : (printf("v010: all ok\n"), 0);
  }

  // ---- how soon can a Lil Blob become a Lil Blub? ./sim blub ----
  // A carer who is there the whole time and answers at once: every icon tried as soon as the blob lands, the bug
  // cured the second it shows, the light off the second they nod off (and, in the second run, the nap cut short by
  // shaking them awake). Prints when the look picker opens, in real minutes and in their own age.
  static int blub() {
    for (int shake = 0; shake < 2; shake++) for (int hour = 12; hour <= 22; hour += 10) {
      SimPlat p; p.ms = 1000 + shake; p.setClock(hour, 0); Game g(&p); g.begin();
      tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14);
      const uint32_t t0 = p.ms;
      PetData& d = g.pet.d;
      int picker = -1; float ageAt = 0; bool learned = false; int learnedAt = -1;
      for (int s = 0; s < 3600 * 14 && picker < 0; s++) {
        g.wake();   // (they keep the screen on: growing up waits for a lit screen)
        run(g, p, 1.0f);
        if (g.scr == S_EVOLVE) { picker = (int)((p.ms - t0) / 1000); ageAt = d.age; break; }
        if (!learned && (d.pad2 & 0x4000)) { learned = true; learnedAt = (int)((p.ms - t0) / 1000); }
        if (g.scr != S_MAIN || g.form() == F_DROPPER) continue;
        if (g.pet.sleeping) {
          if (!g.lightOff()) { tapIcon(g, p, IC_LIGHT); run(g, p, 0.2f); }
          else if (shake && (g.pet.nap > 0) && learned) { g.pet.disturb(); g.handleEvents(); g.setLight(false); }   // (only with the motion sensor on)   // shaken awake, light back on
          continue;
        }
        if (g.pet.sick()) { tapIcon(g, p, IC_MEDS); run(g, p, MEDS_SECS + 0.5f); continue; }
        if (d.droppings || d.floorMess) { tapIcon(g, p, IC_LITTER); run(g, p, 0.3f); if (g.scr == S_SCRUB) { scrubSolve(g, p); run(g, p, 2.5f); } continue; }
        if (d.hunger < 40) { tapIcon(g, p, IC_FEED); run(g, p, g.eatSecs() + 0.5f); continue; }
        if (!(d.pad2 & (1u << IC_CALL))) { tapIcon(g, p, IC_CALL); run(g, p, 0.3f); continue; }
        if (!(d.pad2 & (1u << IC_METER))) { tapIcon(g, p, IC_METER); run(g, p, 0.3f); g.debugAct(-1); continue; }
        if (!(d.pad2 & (1u << IC_PLAY))) { tapIcon(g, p, IC_PLAY); run(g, p, 0.3f); g.debugAct(-1); continue; }
      }
      printf("  started at %02d:00, %s: all 7 lessons learned at %4.1f min; the look picker opened at %4.1f min (their age then: %4.1f min)\n",
             hour, shake ? "nap cut short by a shake" : "nap slept out with the light off", learnedAt / 60.0, picker / 60.0, (double)ageAt / 60.0);
    }
    return 0;
  }

  // ---- the wiki's pictures: every status effect, set up for real and photographed. ./sim wikishots <dir> ----
  static void blinkOn(Game& g, SimPlat& p) { for (int i = 0; i < 40 && ((int)(g.anim_ * 2) & 1); i++) step(g, p); for (int i = 0; i < 40 && !((int)(g.anim_ * 2) & 1); i++) step(g, p); step(g, p); step(g, p); }   // (the icons beside the head blink: wait until they show)
  static int wikiShots(const char* dir) {
    outDir = dir;
    SimPlat p; p.ms = 9000; Game g(&p); g.begin(); grown(g, p, TEEN_END + 3600, 37);
    PetData& d = g.pet.d;
    auto reset = [&]() {
      g.debugAct(-1); p.setClock(14, 0); g.setLocked(false); g.setLight(false);
      memset(d.sick, 0, 3); d.hunger = d.fun = 80; d.health = 100; d.resent = 0; d.droppings = 0; d.floorMess = 0;
      d.giftOnFloor = 0; d.exploring = 0; d.digestTimer = 1e6f; d.exploreTimer = 1e6f; d.evolving = 0; d.breed = SP_PLAIN; d.sadness = 0;
      g.pet.sleeping = false; g.pet.nap = 0; g.pet.grump = 0; g.msgT_ = 0; g.clockShowT_ = 0; g.pottyPending_ = 0;
      g.blobX_ = g.blobTx_ = 13; g.faceLeft_ = false; g.idleSec = 0;
      run(g, p, 1.2f); g.blobX_ = g.blobTx_ = 13; g.faceLeft_ = false; g.msgT_ = 0; step(g, p);
    };
    auto still = [&]() { g.blobX_ = g.blobTx_ = 13; g.faceLeft_ = false; g.stepT_ = 5; g.blinkT_ = 3; step(g, p); };
    reset(); still(); shot(g, "well");
    reset(); d.sick[SICK_BUG] = 1; d.health = 75; still(); blinkOn(g, p); still(); blinkOn(g, p); shot(g, "sick_bug");
    reset(); d.sick[SICK_HUNGER] = 1; d.health = 75; still(); blinkOn(g, p); shot(g, "sick_hunger");
    reset(); d.sick[SICK_LIGHT] = 1; d.health = 75; still(); blinkOn(g, p); shot(g, "sick_light");
    reset(); d.sick[SICK_BUG] = d.sick[SICK_HUNGER] = d.sick[SICK_LIGHT] = 1; d.health = 25; still(); blinkOn(g, p); shot(g, "sick_three");
    tapIcon(g, p, IC_CALL); run(g, p, 0.2f); still(); blinkOn(g, p); shot(g, "sick_call");
    reset(); d.sick[SICK_BUG] = 1; tapIcon(g, p, IC_MEDS); run(g, p, 0.9f); shot(g, "medicine_1"); run(g, p, 0.75f); shot(g, "medicine_2"); run(g, p, 1.0f); shot(g, "medicine_better");
    reset(); tapIcon(g, p, IC_MEDS); run(g, p, 0.4f); shot(g, "refuse");
    reset(); d.health = 12; still(); blinkOn(g, p); shot(g, "weak");
    reset(); d.hunger = 6; run(g, p, 1.2f); tapIcon(g, p, IC_CALL); run(g, p, 0.2f); still(); shot(g, "hungry");
    reset(); d.fun = 6; run(g, p, 1.2f); tapIcon(g, p, IC_CALL); run(g, p, 0.2f); still(); shot(g, "bored");
    reset(); d.droppings = 4; d.floorMess = 2; d.messAge = 10; run(g, p, 1.2f); tapIcon(g, p, IC_CALL); run(g, p, 0.2f); g.blobX_ = g.blobTx_ = 4; step(g, p); shot(g, "messy");
    reset(); d.resent = 70; run(g, p, 1.2f); tapIcon(g, p, IC_CALL); run(g, p, 0.2f); still(); shot(g, "upset");
    reset(); tapIcon(g, p, IC_CALL); run(g, p, 0.2f); still(); shot(g, "happy_nothing");
    // asleep at night, the light still on: they call (SLEEPY); then the light off
    reset(); p.setClock(23, 0); run(g, p, 2.5f); d.nightSleep = 0; d.nightLight = 0;
    tapIcon(g, p, IC_CALL); run(g, p, 0.2f); g.blobX_ = g.blobTx_ = 13; step(g, p); shot(g, "asleep_light_on");
    tapIcon(g, p, IC_LIGHT); run(g, p, 0.6f); shot(g, "asleep_dark");
    tapDot(g, p, 20, 14); run(g, p, 0.2f); shot(g, "asleep_dark_clock");
    g.setLight(false); d.nightSleep = 0; d.nightLight = 0;
    // worn out in the day: a nap
    reset(); g.pet.nap = TEST_NAP_SECS; run(g, p, 2.5f); g.blobX_ = g.blobTx_ = 13; step(g, p); shot(g, "nap");
    reset(); p.setClock(23, 0); run(g, p, 2.5f); g.setLight(true); d.hunger = 80; tapIcon(g, p, IC_PLAY); run(g, p, 0.2f); g.setLight(false); step(g, p); shot(g, "zzz_refusal");
    reset(); tapIcon(g, p, IC_LOCK); run(g, p, 0.3f); shot(g, "lock_ask"); tapDot(g, p, 10, 23); run(g, p, 1.0f); shot(g, "lock_shut"); run(g, p, 1.5f); still(); shot(g, "locked");
    g.setLocked(false);
    reset(); d.exploring = 1; d.exploreLeft = 200; g.say("EXPLORING", 3); run(g, p, 0.3f); shot(g, "exploring");
    reset(); d.giftOnFloor = GIFT_GEM; g.say("GIFT!"); g.blobX_ = g.blobTx_ = 18; step(g, p); shot(g, "gift");
    reset(); d.breed = SP_VOID; still(); shot(g, "void");
    reset(); d.breed = SP_VOID; d.sick[SICK_BUG] = 1; still(); blinkOn(g, p); shot(g, "void_sick");
    // the Meter
    reset(); p.batt = 87; tapIcon(g, p, IC_METER); run(g, p, 0.2f); shot(g, "meter_1");
    d.hunger = 60; tapIcon(g, p, IC_METER); run(g, p, 0.2f); shot(g, "meter_hungry");
    d.fun = 0; tapIcon(g, p, IC_METER); run(g, p, 0.2f); shot(g, "meter_happy_zero");
    d.resent = 55; tapIcon(g, p, IC_METER); run(g, p, 0.2f); shot(g, "meter_trust");
    g.sv.sickCount = 2; d.giftCount = 3; tapIcon(g, p, IC_METER); run(g, p, 0.2f); shot(g, "meter_last");
    // a Lil Blob's lessons
    {
      SimPlat p2; p2.ms = 500; Game h(&p2); h.begin();
      tap(h, p2, 92, 195); tap(h, p2, 92, 100); tap(h, p2, 92, 182); run(h, p2, 0.5f); shot(h, "dropper");
      tapDot(h, p2, 20, 14); run(h, p2, SQUEEZE_SECS + 2.5f);
      tapIcon(h, p2, IC_CALL); run(h, p2, 0.2f); shot(h, "lesson_hello");
      h.pet.d.sick[SICK_BUG] = 1; h.pet.d.sick[3] |= SF_BUG_DONE; run(h, p2, 3.0f); blinkOn(h, p2); shot(h, "lesson_bug");
    }
    // the ending
    reset(); p.batt = -1; d.health = 0.5f; d.sick[SICK_BUG] = 3; g.simulate(3 * 3600, false); run(g, p, 0.4f); shot(g, "ending_caption"); run(g, p, 1.6f); shot(g, "ending_melt"); run(g, p, 3.2f); shot(g, "ending_dropper");
    return 0;
  }
};
}  // namespace lf
