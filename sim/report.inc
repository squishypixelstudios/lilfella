// ---------------------------------------------------------------------------------------------
// Balance report: a scripted player taps the real icons and menus, minute-by-minute stats are recorded.
//   ./sim games <outdir>      every mini-game, many times, at several skill levels
//   ./sim sessions <outdir>   60 two-hour sessions for each player habit (every 5 min / every 20 min / never)
// ---------------------------------------------------------------------------------------------
namespace lf {
struct Reporter {
  static constexpr int MIN = 120;
  struct Bin {
    double w = 0, hunger = 0, fun = 0, energy = 0, health = 0, chonk = 0;
    double asleep = 0, nap = 0, lightOff = 0, sick = 0, calling = 0, awake = 0;
    double poops = 0, floorPoops = 0, mischief = 0, mischiefLesson = 0, good = 0, goodCaught = 0, wand = 0;
    double sleepStarts = 0, bubbles = 0, mistakes = 0, picker = 0, evolved = 0, lessonAsk = 0, lessonsDone = 0;
    double feeds = 0, snacks = 0, games = 0, litter = 0, meds = 0, pets = 0, checks = 0, refused = 0, tired = 0;
    double coinsEnd = 0, ageEnd = 0, droppingsEnd = 0, n = 0;
    double ask[8] = {0};
  };
  Bin bins[MIN];
  // per-run summaries
  struct RunSum { int lessonsMin = -1, pickerMin = -1, evolveMin = -1, pick = -1, mistakes = 0, poops = 0, coins = 0, mischief = 0, good = 0, caught = 0, wand = 0; float age = 0, chonk = 0, minHealth = 100; bool ended = false; };
  std::vector<RunSum> sums;

  SimPlat* P = nullptr; Game* G = nullptr; uint32_t t0 = 0; uint32_t r = 1;
  // previous-frame state
  int pTotal = 0, pFloor = 0, pHb = 0, pTut = -1, pMist = 0; float pMis = 0, pGood = 0; bool pSleep = false, pDone = false; Screen pScr = S_MAIN; Form pForm = F_DROPPER;
  RunSum cur;
  int rnd(int n) { r ^= r << 13; r ^= r >> 17; r ^= r << 5; return (int)(r % (uint32_t)n); }
  int minute() const { return (int)((P->ms - t0) / 60000); }
  Bin* bin() { int m = minute(); return (m >= 0 && m < MIN) ? &bins[m] : nullptr; }

  void sample(uint32_t ms) {
    Game& g = *G; PetData& d = g.pet.d; Bin* b = bin();
    const double w = ms / 1000.0;
    const int total = d.droppings + d.floorMess;
    if (b) {
      b->w += w; b->hunger += d.hunger * w; b->fun += d.fun * w; b->energy += d.energy * w; b->health += d.health * w; b->chonk += d.chonk * w;
      if (g.pet.sleeping) { b->asleep += w; if (g.pet.napping) b->nap += w; } else b->awake += w;
      if (g.lightOff()) b->lightOff += w;
      if (d.health < 35) b->sick += w;
      if (g.callReason() != CR_NONE) b->calling += w;
      if (total > pTotal) { b->poops += total - pTotal; cur.poops += total - pTotal; }
      if (d.floorMess > pFloor) b->floorPoops += d.floorMess - pFloor;
      if (g.pet.sleeping && !pSleep) b->sleepStarts++;
      if (d.bubblesOnFloor > pHb) b->bubbles += d.bubblesOnFloor - pHb;
      if (d.pad1[0] > pMist) b->mistakes += d.pad1[0] - pMist;
      if (g.tutWant_ >= 0 && pTut < 0) { b->lessonAsk++; b->ask[g.tutWant_]++; }
      const bool done = d.pad2 & 0x4000;
      if (done && !pDone) { b->lessonsDone++; cur.lessonsMin = minute(); }
      if (g.scr == S_EVOLVE && pScr != S_EVOLVE && cur.pickerMin < 0) { b->picker++; cur.pickerMin = minute(); }
      const Form f = g.form();
      if (pForm == F_BABY && f != F_BABY && f != F_DROPPER) { b->evolved++; cur.evolveMin = minute(); }
      pForm = f;
      if (d.health < cur.minHealth) cur.minHealth = d.health;
    }
    pTotal = total; pFloor = d.floorMess; pHb = d.bubblesOnFloor; pTut = g.tutWant_; pMist = d.pad1[0];
    pSleep = g.pet.sleeping; pDone = d.pad2 & 0x4000; pScr = g.scr;
  }
  // frames: 33 ms while touching/watching closely, 100 ms otherwise (the game clamps dt to 0.1 s, so this stays exact)
  void fr(bool down = false, int x = 0, int y = 0, uint32_t ms = 33) { P->ms += ms; G->frame(fb, down, x * 2, y * 2); sample(ms); }
  void wait(float s, uint32_t ms = 33) { int n = (int)(s * 1000 / ms); for (int i = 0; i < n; i++) fr(false, 0, 0, ms); }
  void tapXY(int x, int y) { fr(true, x, y); fr(true, x, y); fr(); fr(); }
  void tapD(int dx, int dy) { tapXY(MX + dx * DOT + 1, MY + dy * DOT + 1); }
  void tapI(int ic) { tapXY(iconX(ic) + 8, iconY(ic) + 8); }
  void settle() { for (int i = 0; i < 30 * 8 && G->scr != S_MAIN && G->scr != S_EVOLVE; i++) fr(); }
  void count(double Bin::*f) { if (Bin* b = bin()) b->*f += 1; }

  // ---- mini-game players ----

  // skill 0 expert, 1 average, 2 beginner: reaction time, how far ahead they look, aim wobble
  void playDodge(int skill) {
    const int gs = G->gameSprite();
    const int w = spriteW(gs), cy = MH - 1 - spriteH(gs);
    const float delay = skill == 0 ? 0.0f : skill == 1 ? 0.30f : 0.50f, look = skill == 0 ? 2.0f : skill == 1 ? 1.0f : 0.7f;
    int target = (int)G->dg_.x; float next = 0;
    for (int f = 0; f < 30 * 90 && G->scr == S_DODGE && G->dg_.phase <= 1; f++) {
      Game::Dodge& d = G->dg_;
      next -= 1.0f / 30;
      if (d.phase == 1 && next <= 0) {
        float best = -1e9;
        for (int x = 0; x <= MW - w; x++) {
          float safe = look;
          for (float tt = 0; tt < look; tt += 1.0f / 30) {
            float cxp = d.x + (x > d.x ? 1 : -1) * 34.0f * tt;
            if ((x >= d.x && cxp > x) || (x <= d.x && cxp < x)) cxp = (float)x;
            const int cxi = (int)cxp; bool hit = false;
            for (auto& it : d.it) if (it.on) { const float y = it.y + it.v * tt; if (y + it.h - 1 >= cy + 2 && y < MH - 1 && it.x + it.w > cxi + 1 && it.x < cxi + w - 1) hit = true; }
            if (hit) { safe = tt; break; }
          }
          const float sc = safe - fabsf(x - (MW - w) / 2.0f) * 0.002f;
          if (sc > best) { best = sc; target = x; }
        }
        if (skill == 2) target += rnd(3) - 1;
        next = delay * (0.8f + 0.4f * rnd(100) / 100.0f);
      }
      fr(true, MX + (target + w / 2) * DOT + 1, MY + 60);
    }
    fr(); wait(1.0f);
    for (int f = 0; f < 30 * 6 && G->scr == S_DODGE; f++) { if (G->dg_.t > 1.0f) tapD(20, 14); else fr(); }
  }
  // a person digging: guesses a random section each time, never one already dug
  void playScrub() {   // rub back and forth until it's clean
    for (int i = 0; i < 30 * 15 && G->scr == S_SCRUB && !G->sc_.done; i++) { const int k = i % 16; P->ms += 33; G->frame(fb, true, (MX + 20 + (k < 8 ? k : 16 - k) * 16) * 2, (MY + 60) * 2); sample(33); }
    for (int f = 0; f < 30 * 3 && G->scr == S_SCRUB; f++) fr();
  }
  void playLitter() {
    if (G->scr == S_SCRUB) { playScrub(); return; }
    if (G->scr != S_LITTER) return;
    wait(0.6f);
    for (int guard = 0; guard < 30 * 60 && G->scr == S_LITTER && !G->lg_.done; guard++) {
      bool tapped = false;
      for (int i = 0; i < LITTER_BUBBLES && !tapped; i++) { int px, py; bool pp; if (!G->lgBubble(i, px, py, pp) || !pp) continue; for (int j = 0; j < LITTER_BUBBLES && !tapped; j++) { int cx, cy; bool cp; if (G->lgBubble(j, cx, cy, cp) && !cp && abs(cx - px) < 9 && cy < py && py - cy < 12 && cy >= 2) { tapD(cx, cy); tapped = true; } } }
      if (!tapped) fr();
    }
    for (int f = 0; f < 30 * 3 && G->scr == S_LITTER; f++) fr();
    if (G->scr == S_LITTER) tapI(IC_LITTER);
  }

  // ---- one care action; returns false when there was nothing to do ----
  int sessionGame = 0, dodgeSkill = 1, pickWant = 0;
  bool careStep() {
    Game& g = *G; PetData& d = g.pet.d;
    settle();
    if (g.scr == S_EVOLVE) {
      for (int k = 0; k < pickWant; k++) { tapD(MW - 2, 14); wait(0.3f); }
      tapD(MW / 2, 20); cur.pick = pickWant; wait(2.5f); settle(); return true;
    }
    if (g.scr != S_MAIN) { tapI(IC_METER); settle(); if (g.scr != S_MAIN) { g.debugAct(-1); fr(); } return true; }
    if (d.bubblesOnFloor) { tapD(0, MH - 2); wait(0.3f); return true; }
    if (d.giftOnFloor) { tapD(d.bubblesOnFloor * 3 + 3, MH - 3); wait(0.3f); return true; }
    const CallReason why = g.callReason();
    if (why != CR_NONE && why != CR_HELLO && why != CR_CHECK) { tapI(IC_CALL); wait(0.8f); count(&Bin::checks); }   // look at what they want
    if (why == CR_SICK || d.health < 35) { tapI(IC_MEDS); count(&Bin::meds); wait(2.6f); if (trace) printf("   meds -> health %.0f scr %d an %d\n", d.health, (int)g.scr, (int)g.an_); return true; }
    if ((why == CR_HUNGRY || d.hunger < 50) && !(g.pet.sleeping && d.hunger >= 40)) {   // asleep and not very hungry: let them sleep
      tapI(IC_FEED); count(&Bin::feeds); wait(0.4f);
      if (g.an_ == AN_REFUSE) count(&Bin::refused);
      wait(2.6f); return true;
    }
    if (why == CR_MESS || d.droppings || d.floorMess) { tapI(IC_LITTER); count(&Bin::litter); playLitter(); settle(); return true; }
    if ((why == CR_BORED || d.fun < 50) && !g.pet.sleeping) {
      if (d.energy < 10) return false;
      tapI(IC_PLAY); wait(0.3f);
      if (g.scr != S_MENU) { count(&Bin::tired); settle(); return false; }
      sessionGame++;
      tapD(10, 4); count(&Bin::games); wait(0.2f);   // DODGE
      playDodge(dodgeSkill);
      settle(); return true;
    }
    if (why == CR_LIGHT && !g.lightOff()) { tapI(IC_LIGHT); wait(0.3f); return true; }
    if (why == CR_UPSET || why == CR_HELLO) { if (why == CR_HELLO) { tapI(IC_CALL); wait(2.6f); return true; } }
    if (why == CR_CHECK) { tapI(IC_METER); for (int i = 0; i < 8 && g.scr == S_METER; i++) { wait(0.4f); tapD(20, 14); } settle(); return true; }
    return false;
  }
  bool trace = false;
  void checkIn(float watch) {
    count(&Bin::checks);
    if (trace) printf("min %3d check: why %d health %.0f hunger %.0f fun %.0f energy %.0f sleep %d nap %d light %d scr %d tut %d\n", minute(), (int)G->callReason(), G->pet.d.health, G->pet.d.hunger, G->pet.d.fun, G->pet.d.energy, G->pet.sleeping, G->pet.napping, G->lightOff(), (int)G->scr, G->tutWant_);
    if (!G->displayOn()) tapD(20, 1);   // the first touch only wakes the screen
    wait(0.5f);
    // a quick pet, the way people say hello
    if (!G->pet.sleeping && G->scr == S_MAIN && G->form() != F_DROPPER) {
      int idle, alt, sl; G->blobSprite(G->form(), idle, alt, sl);
      tapD((int)G->blobX_ + spriteW(idle) / 2, MH - 3); count(&Bin::pets); wait(1.6f);
    }
    const uint32_t start = P->ms;
    for (int guard = 0; guard < 40; guard++) {
      if (careStep()) continue;
      if ((P->ms - start) / 1000.0f >= watch) break;
      wait(1.0f);
    }
  }

  int formWidth() const { return spriteW(G->gameSprite()); }
  float dodgeScore() const { return G->dg_.score; }
  int litterGot() const { return G->lg_.got; }
  int litterBubble() const { return 0; }

  // ---- one 2-hour session ----
  void session(int seed, int every, float watch, int pick) {
    SimPlat p; p.ms = 2000 + (uint32_t)seed * 7919u; P = &p;
    Game g(&p); G = &g; g.begin();
    r = 0x9E3779B9u ^ (uint32_t)(seed * 2654435761u); if (!r) r = 1;
    tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f);   // clock + name
    p.setClock(12, 0);
    t0 = p.ms; cur = RunSum(); pickWant = pick; sessionGame = seed; dodgeSkill = 1;
    pTotal = pFloor = pHb = pMist = 0; pTut = -1; pMis = pGood = 0; pSleep = false; pDone = false; pScr = g.scr; pForm = g.form();
    int nextCheck = 0;
    while (minute() < MIN && !g.pet.ended && g.sv.hasBlob) {
      if (every > 0 && (int)((p.ms - t0) / 1000) >= nextCheck) {
        checkIn(watch);
        nextCheck = (int)((p.ms - t0) / 1000) + every * 60 + (rnd(2 * 60 + 1) - 60) * every / 5;
      }
      fr(false, 0, 0, 100);
      if (minute() < MIN) { Bin& b = bins[minute()]; (void)b; }
      if ((p.ms - t0) % 60000 < 100 && minute() > 0 && minute() <= MIN) {   // end of each minute
        Bin& b = bins[minute() - 1];
        b.coinsEnd += g.sv.sickCount; b.ageEnd += g.pet.d.age / 60.0; b.droppingsEnd += g.pet.d.droppings; b.n++;
      }
    }
    cur.mistakes = g.pet.d.pad1[0]; cur.coins = (int)g.sv.sickCount; cur.age = g.pet.d.age / 60.0f; cur.chonk = g.pet.d.chonk; cur.ended = g.pet.ended;
    sums.push_back(cur);
  }
};
}  // namespace lf

static void writeSessions(lf::Reporter& R, int runs, const char* path, const char* name) {
  using lf::Reporter;
  FILE* f = fopen(path, "w");
  fprintf(f, "minute,hunger,happy,energy,health,chonk,asleep_pct,nap_pct,light_off_pct,sick_pct,calling_pct,poops,floor_poops,mischief,mischief_lesson,good_moments,good_caught,wand,sleep_starts,bubbles,care_mistakes,lesson_asks,ask_call,ask_feed,ask_light,ask_play,ask_meds,ask_litter,ask_meter,ask_train,lessons_done,picker_shown,evolved,feeds,snacks,games,litter_games,meds,pets,checks,refused,tired,coins,age_min,droppings\n");
  for (int m = 0; m < Reporter::MIN; m++) {
    const Reporter::Bin& b = R.bins[m];
    const double W = b.w > 0 ? b.w : 1, N = runs;
    fprintf(f, "%d,%.1f,%.1f,%.1f,%.1f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
            m + 1, b.hunger / W, b.fun / W, b.energy / W, b.health / W, b.chonk / W, 100 * b.asleep / W, 100 * b.nap / W, 100 * b.lightOff / W, 100 * b.sick / W, 100 * b.calling / W,
            b.poops / N, b.floorPoops / N, b.mischief / N, b.mischiefLesson / N, b.good / N, b.goodCaught / N, b.wand / N, b.sleepStarts / N, b.bubbles / N, b.mistakes / N, b.lessonAsk / N);
    for (int i : {lf::IC_CALL, lf::IC_FEED, lf::IC_LIGHT, lf::IC_PLAY, lf::IC_MEDS, lf::IC_LITTER, lf::IC_METER, lf::IC_LOCK}) fprintf(f, ",%.3f", b.ask[i] / N);
    fprintf(f, ",%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f,%.1f,%.2f\n",
            b.lessonsDone / N, b.picker / N, b.evolved / N, b.feeds / N, b.snacks / N, b.games / N, b.litter / N, b.meds / N, b.pets / N, b.checks / N, b.refused / N, b.tired / N,
            b.n ? b.coinsEnd / b.n : 0, b.n ? b.ageEnd / b.n : 0, b.n ? b.droppingsEnd / b.n : 0);
  }
  fclose(f);
  char sp[600]; snprintf(sp, sizeof sp, "%s.runs.csv", path);
  f = fopen(sp, "w");
  fprintf(f, "profile,run,lessons_min,picker_min,evolve_min,pick,mistakes,poops,coins,mischief,good,caught,wand,age_min,chonk,min_health,ended\n");
  int i = 0;
  for (auto& s : R.sums) fprintf(f, "%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.1f,%.1f,%.1f,%d\n", name, i++, s.lessonsMin, s.pickerMin, s.evolveMin, s.pick, s.mistakes, s.poops, s.coins, s.mischief, s.good, s.caught, s.wand, s.age, s.chonk, s.minHealth, s.ended);
  fclose(f);
}

static int cmdSessions(const char* dir, int runs) {
  struct Prof { const char* name; int every; float watch; } profs[] = {{"every5", 5, 45}, {"every20", 20, 30}, {"handsoff", 0, 0}};
  for (auto& pr : profs) {
    auto* R = new lf::Reporter();
    for (int s = 0; s < runs; s++) R->session(s, pr.every, pr.watch, s % 3);   // picks spread evenly: 20 / 20 / 20
    char path[512]; snprintf(path, sizeof path, "%s/session_%s.csv", dir, pr.name);
    writeSessions(*R, runs, path, pr.name);
    printf("%s: %d sessions\n", pr.name, runs);
    delete R;
  }
  return 0;
}

// every mini-game, many times: what you get out of each
static int cmdGames(const char* dir, int n) {
  using namespace lf;
  char path[512]; snprintf(path, sizeof path, "%s/games.csv", dir);
  FILE* f = fopen(path, "w");
  fprintf(f, "game,skill,run,result,secs,fun_gain,energy_cost,chonk_change,coins,clumps,floor,bubble_found\n");
  for (int kind = 1; kind < 3; kind++) {   // (kind 0 was Guess: gone)
    const int skills = kind == 1 ? 3 : 1;
    for (int sk = 0; sk < skills; sk++)
      for (int i = 0; i < n; i++) {
        auto* R = new Reporter();
        SimPlat p; p.ms = 3000 + (uint32_t)(i * 104729 + sk * 7 + kind * 13); R->P = &p;
        Game g(&p); R->G = &g; g.begin(); R->r = 1234567u + i * 2654435761u + sk * 97 + kind * 31;
        tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); p.setClock(12, 0); R->t0 = p.ms;
        g.simulate(70, false); R->wait(2);
        g.pet.d.age = BABY_END + 3600; g.pet.d.pad0[0] = 1; g.pet.d.pad2 = 0x2000; g.pet.d.nextSick = 1e9f; g.pet.rng = 99991u * (i + 1) + sk;
        g.pet.d.hunger = 70; g.pet.d.fun = 30; g.pet.d.energy = 80; g.pet.d.health = 90; g.pet.sleeping = false;
        R->wait(3.0f); if (g.scr == S_EVOLVE) { R->tapD(MW / 2, 20); R->wait(3); }
        g.pet.d.fun = 30; g.pet.d.energy = 80;
        const float fun0 = g.pet.d.fun, en0 = g.pet.d.energy, ch0 = g.pet.d.chonk; const uint32_t c0 = g.sv.sickCount;
        const uint32_t ms0 = p.ms;
        int result = 0, clumps = 0, flo = 0, hb = 0;
        if (kind == 1) {
          R->tapI(IC_PLAY); R->wait(0.3f); R->tapD(10, 4); R->wait(0.2f);
          R->playDodge(sk); result = (int)(R->dodgeScore() * 10);
        } else {
          g.pet.d.droppings = 1 + i % 6; g.pet.d.floorMess = i % 3; clumps = g.pet.d.droppings; flo = g.pet.d.floorMess;
          R->tapI(IC_LITTER); R->playLitter(); hb = R->litterBubble();
          result = R->litterGot();
        }
        R->settle();
        const char* nm = kind == 1 ? "dodge" : "scrub";
        fprintf(f, "%s,%d,%d,%d,%.1f,%.1f,%.1f,%.2f,%d,%d,%d,%d\n", nm, sk, i, result, (p.ms - ms0) / 1000.0, g.pet.d.fun - fun0, en0 - g.pet.d.energy,
                g.pet.d.chonk - ch0, (int)(g.sv.sickCount - c0), clumps, flo, hb);
        delete R;
      }
  }
  // Dodge for every look: bigger fellas are bigger targets
  fprintf(f, "\n");
  FILE* f2; snprintf(path, sizeof path, "%s/dodge_forms.csv", dir); f2 = fopen(path, "w");
  fprintf(f2, "form,width,skill,avg,median,best\n");
  struct FL { const char* nm; float age; uint8_t pad; } fl[] = {
    {"Lil Blob", 600, 0}, {"Blobbert", BABY_END + 60, 1}, {"Mochi", BABY_END + 60, 2}, {"Cheeks", BABY_END + 60, 3},
    {"Bean", CHILD_END + 60, 3 | 1 << 2}, {"Bun", CHILD_END + 60, 1 | 2 << 2}, {"Curly", CHILD_END + 60, 1 | 3 << 2},
    {"Puddle", TEEN_END + 60, 41}, {"Jelly", TEEN_END + 60, 37}, {"Dapper", TEEN_END + 60, 39}, {"Sprout", CHILD_END + 60, 5},
    {"Dango", TEEN_END + 60, 69}, {"Puff", TEEN_END + 60, 105}, {"Loaf", TEEN_END + 60, 42}};
  for (auto& L : fl) for (int sk = 0; sk < 2; sk++) {
    std::vector<float> sc; int w = 0;
    for (int i = 0; i < 30; i++) {
      auto* R = new Reporter(); SimPlat p; p.ms = 5000 + i * 7717 + sk; R->P = &p; Game g(&p); R->G = &g; g.begin(); R->r = 777u + i * 31337u;
      tap(g, p, 92, 195); tap(g, p, 92, 100); tap(g, p, 92, 182); tapDot(g, p, 20, 14); run(g, p, SQUEEZE_SECS + 0.2f); p.setClock(12, 0); R->t0 = p.ms; g.simulate(70, false); R->wait(1);
      g.pet.d.age = L.age; g.pet.d.pad0[0] = L.pad; g.pet.d.pad2 = 0x2000; g.pet.d.nextSick = 1e9f; g.pet.d.energy = 90; g.pet.d.hunger = 80; g.pet.d.fun = 40; g.pet.sleeping = false; g.pet.rng = 4242u + i * 99991u;
      R->wait(1.0f); if (g.scr != S_MAIN) { g.debugAct(-1); R->wait(0.2f); }
      w = R->formWidth();
      R->tapI(IC_PLAY); R->wait(0.3f); R->tapD(30, 4); R->wait(0.2f); R->playDodge(sk); sc.push_back(R->dodgeScore());
      delete R;
    }
    std::vector<float> s2 = sc; std::sort(s2.begin(), s2.end()); float avg = 0; for (float v : sc) avg += v; avg /= sc.size();
    fprintf(f2, "%s,%d,%d,%.1f,%.1f,%.1f\n", L.nm, w, sk, avg, s2[s2.size() / 2], s2.back());
  }
  fclose(f2);
  fclose(f);
  printf("games: %d of each written to %s\n", n, dir);
  return 0;
}
static int cmdTrace(int seed, int every) {
  using namespace lf;
  auto* R = new Reporter(); R->trace = true;
  R->session(seed, every, 30, seed % 3);
  return 0;
}

// ---------------------------------------------------------------------------------------------
// Touch map: where every touch lands, pixel by pixel, on every screen; and whether things on the home
// screen sit on top of each other.   ./sim touchmap
// ---------------------------------------------------------------------------------------------
namespace lf {
struct TouchMap {
  static constexpr int FLOOR = MH - 1;
  Reporter R; SimPlat p; Game* g = nullptr; int fails = 0;
  void check(bool ok, const char* what) { printf("  %-62s %s\n", what, ok ? "ok" : "FAIL"); if (!ok) fails++; }
  void fresh() {
    delete g; p = SimPlat(); p.ms = 5000; g = new Game(&p); R.P = &p; R.G = g; g->begin();
    tap(*g, p, 92, 195); tap(*g, p, 92, 100); tap(*g, p, 92, 182); tapDot(*g, p, 20, 14); p.setClock(12, 0); R.t0 = p.ms; R.wait(SQUEEZE_SECS + 0.5f);
    g->pet.d.pad2 = 0xE0FF; g->pet.d.nextSick = 1e9f; g->pet.d.hunger = g->pet.d.fun = 80; g->pet.d.energy = 90;
  }
  int iconAt(int x, int y) {   // what the game's own hit test says (logical px)
    g->in.tap = true; g->in.px = x; g->in.py = y; int ic = -1;
    if (!g->iconTapped(ic)) ic = -1;
    g->in.tap = false; return ic;
  }
  void icons() {
    printf("icons\n");
    int wrong = 0, lcdHits = 0, deadInBox = 0;
    for (int ic = 0; ic < IC_COUNT; ic++) {
      const int x0 = iconX(ic), y0 = iconY(ic);
      for (int y = y0; y < y0 + 16; y++) for (int x = x0; x < x0 + 16; x++) { const int a = iconAt(x, y); if (a < 0) deadInBox++; else if (a != ic) wrong++; }
    }
    for (int y = MY - 4; y <= MY1 + 4; y++) for (int x = 0; x < W; x++) if (iconAt(x, y) >= 0) lcdHits++;
    char b[96];
    snprintf(b, sizeof b, "every pixel of all 8 icons hits that icon (%d wrong, %d dead)", wrong, deadInBox); check(!wrong && !deadInBox, b);
    snprintf(b, sizeof b, "no touch on the LCD or its edge reaches an icon (%d px)", lcdHits); check(!lcdHits, b);
    int gaps = 0;   // between the icon rows and the LCD: every row of pixels goes somewhere
    for (int y = 0; y < H; y++) { int dx, dy; bool any = iconAt(92, y) >= 0 || g->inMatrix(92, y, dx, dy); if (!any) gaps++; }
    snprintf(b, sizeof b, "no dead strip between icons and LCD (%d dead rows)", gaps); check(!gaps, b);
    // the icon columns split halfway between neighbouring icons
    bool mid = true;
    for (int c = 0; c < 3; c++) { const int l = ICON_X[c] + 16, r = ICON_X[c + 1], m = (l + r) / 2; if (iconAt(m - 2, 10) != ICON_AT[c] || iconAt(m + 2, 10) != ICON_AT[c + 1]) mid = false; }
    check(mid, "icon columns split halfway between neighbouring icons");
    // rounded screen corners: keep icons at least ~20 logical px (40 physical) in from each corner curve
    bool corner = true; const float R0 = 24;
    for (int sl : {0, 3, 4, 7}) {
      const int x0 = ICON_X[sl % 4], y0 = sl < 4 ? ICON_TOP_Y : ICON_BOT_Y;
      for (int cy : {y0, y0 + 15}) for (int cx : {x0, x0 + 15}) {
        const float ccx = cx < W / 2 ? R0 : W - 1 - R0, ccy = cy < H / 2 ? R0 : H - 1 - R0;
        const bool inCorner = (cx < R0 || cx > W - 1 - R0) && (cy < R0 || cy > H - 1 - R0);
        if (inCorner && hypotf(cx - ccx, cy - ccy) > R0) corner = false;
      }
    }
    check(corner, "icons clear of the rounded screen corners (24 px radius)");
  }
  // sweep a page top to bottom at one x; report which thing each pixel row triggers
  template <class Snap, class Diff, class Restore>
  void sweep(const char* name, int x, Screen s, Snap snap, Diff diff, Restore restore, int expectN, int minGap) {
    int last = -2, runs = 0, overl = 0; int count[16] = {0}; int prevEnd = -100, gapMin = 999;
    int startY[16], endY[16]; for (int i = 0; i < 16; i++) startY[i] = 999, endY[i] = -1;
    for (int y = 0; y < H; y++) {
      g->setScreen(s); R.fr(); auto st = snap();
      R.tapXY(x, y);
      const int what = diff(st);
      restore(st);
      if (what >= 0 && what < 16) { count[what]++; if (y < startY[what]) startY[what] = y; endY[what] = y; }
      if (what != last && what >= 0) { runs++; if (prevEnd >= 0 && last == -1) { if (y - prevEnd - 1 < gapMin) gapMin = y - prevEnd - 1; } }
      if (what >= 0) prevEnd = y;
      last = what;
    }
    int distinct = 0; for (int i = 0; i < 16; i++) if (count[i]) { distinct++; if (endY[i] - startY[i] + 1 != count[i]) overl++; }
    char b[120];
    snprintf(b, sizeof b, "%s: %d targets found, each one solid band (%d split)", name, distinct, overl); check(distinct == expectN && !overl, b);
    snprintf(b, sizeof b, "%s: smallest gap between targets %d px", name, gapMin == 999 ? 0 : gapMin); check(gapMin >= minGap, b);
    for (int i = 0; i < 16; i++) if (count[i]) printf("      target %d: y %d-%d\n", i, startY[i], endY[i]);
  }
  void pages() {
    printf("pages\n");
    struct SS { uint8_t vol, br, e0, e2; bool st; int ts; };
    for (int page = 0; page < 3; page++) {
      sweep(page == 2 ? "settings page 3" : page ? "settings page 2" : "settings page 1", 92, S_SETTINGS,
        [&] { g->setPage_ = page; return SS{g->sv.volume, g->sv.brightness, g->sv.eq[0], g->sv.eq[2], g->showTouch, g->timeScale}; },
        [&](const SS& a) {
          return g->scr == S_SETUP_CLOCK ? 5 : g->scr == S_CONFIRM ? (g->confirm_ == CF_ERASE ? 11 : 8) : g->scr == S_MAIN ? 9 : g->scr == S_FRIENDS ? 10 : g->scr == S_JOURNAL ? 12
               : g->setPage_ > page ? 4 : g->setPage_ < page ? 9
               : g->sv.volume != a.vol ? 0 : g->sv.brightness != a.br ? 1 : g->sv.eq[0] != a.e0 ? 2
               : g->sv.eq[2] != a.e2 ? 3 : g->showTouch != a.st ? 6 : g->timeScale != a.ts ? 7 : -1; },
        [&](const SS& a) { g->sv.volume = a.vol; g->sv.brightness = a.br; g->sv.eq[0] = a.e0; g->sv.eq[2] = a.e2; g->showTouch = a.st; g->timeScale = a.ts; g->setPage_ = page; g->setScreen(S_MAIN); },
        page == 2 ? 4 : 6, 2);
    }
    // old friends: 8 entries = 2 pages; Back in the middle, page arrows either side
    for (int i = 0; i < HALL_MAX; i++) { HallEntry& e = g->sv.hall[i]; memset(&e, 0, sizeof e); snprintf(e.name, sizeof e.name, "Friend%d", i); e.ending = (uint8_t)(1 + i % 6); e.age = (uint16_t)(5 + i * 40); e.look = (uint8_t)(2 + i % 13); e.flags = HE_HOURS; }
    g->sv.hallCount = HALL_MAX;
    sweep("old friends: Back", 92, S_FRIENDS, [&] { return 0; },
      [&](int) { return g->scr == S_SETTINGS ? 0 : g->scr != S_FRIENDS ? 1 : g->friendsPage_ ? 2 : -1; },
      [&](int) { g->friendsPage_ = 0; g->setScreen(S_MAIN); }, 1, 0);
    sweep("old friends: next page", W - 36, S_FRIENDS, [&] { return 0; },
      [&](int) { return g->scr == S_SETTINGS ? 0 : g->scr != S_FRIENDS ? 1 : g->friendsPage_ ? 2 : -1; },
      [&](int) { g->friendsPage_ = 0; g->setScreen(S_MAIN); }, 1, 0);
    sweep("old friends: < on page 1 does nothing", 36, S_FRIENDS, [&] { return 0; },
      [&](int) { return g->scr != S_FRIENDS ? 0 : g->friendsPage_ ? 1 : -1; },
      [&](int) { g->friendsPage_ = 0; g->setScreen(S_MAIN); }, 0, 0);
    g->sv.hallCount = 0;
    sweep("new blob? (confirm)", 60, S_CONFIRM, [&] { g->confirm_ = CF_NEWBLOB; return 0; },
      [&](int) { return g->scr == S_SETTINGS ? 0 : g->scr == S_SETUP_NAME ? 1 : -1; },
      [&](int) { g->setScreen(S_MAIN); }, 1, 0);
    sweep("new blob? YES side", 130, S_CONFIRM, [&] { g->confirm_ = CF_NEWBLOB; return 0; },
      [&](int) { return g->scr == S_SETTINGS ? 0 : g->scr == S_SETUP_NAME ? 1 : -1; },
      [&](int) { if (g->scr == S_SETUP_NAME) { fresh(); } g->setScreen(S_MAIN); }, 1, 0);
    char nm[12] = "zz"; memcpy(g->nameBuf_, nm, 12);  // a name the random picker never gives, so every NEW NAME tap shows
    {   // name page: keys (y 38-120), Random name (130-158), Let's goo! (168-196); nothing else starts a pet
      int randomOk = 0, startOk = 0, keysOk = 0, strayStart = 0;
      for (int y = 0; y < H; y++) {
        g->setScreen(S_SETUP_NAME); R.fr(); memcpy(g->nameBuf_, nm, 12); g->nameFresh_ = true;
        R.tapXY(92, y);
        const bool started = g->scr == S_MAIN, changed = strcmp(g->nameBuf_, nm) != 0;
        if (started) { fresh(); if (y >= 168 && y < 196) startOk++; else strayStart++; }
        else if (changed) { if (y >= 130 && y < 158) randomOk++; else if (y >= 38 && y < 122) keysOk++; }
        g->setScreen(S_MAIN);
      }
      char b2[120];
      snprintf(b2, sizeof b2, "name page: Random name %d rows, keys %d rows, Let's goo! %d rows, stray starts %d", randomOk, keysOk, startOk, strayStart);
      check(randomOk >= 26 && keysOk >= 60 && startOk >= 26 && !strayStart, b2);
    }
    // clock page: a 2-D sweep; a touch may change at most one field, and every field must be reachable both ways
    g->clockNext_ = S_SETTINGS;
    int both = 0, reach[5][2] = {{0}}; 
    for (int y = 30; y < 180; y += 2) for (int x = 0; x < W; x += 2) {
      g->setScreen(S_SETUP_CLOCK); g->clockNext_ = S_SETTINGS; int c0[5]; memcpy(c0, g->clockSel_, sizeof c0); R.fr();
      R.tapXY(x, y);
      int changed = 0;
      for (int i = 0; i < 5; i++) if (g->clockSel_[i] != c0[i]) { changed++; reach[i][(g->clockSel_[i] > c0[i]) ^ (i == 0 && false)]++; }
      if (changed > 1) both++;
      memcpy(g->clockSel_, c0, sizeof c0);
    }
    bool all = true; for (int i = 0; i < 5; i++) if (!reach[i][0] || !reach[i][1]) all = false;
    check(!both, "clock page: no touch changes two numbers at once");
    check(all, "clock page: every number can be raised and lowered");
    g->setScreen(S_MAIN);
  }
  void menus() {
    printf("menus\n");
    for (int mn = 1; mn < 2; mn++) {   // (the feed menu is gone: Feed serves a meal)
      int hits[5] = {0}, bands = 0, last = -1;
      for (int cell = 0; cell < MH * (mn == 1 ? 2 : 1); cell++) {
        const int dy = cell % MH, dxm = mn == 1 ? (cell < MH ? 10 : 30) : 20;
        g->setScreen(S_MAIN); g->pet.d.energy = 100; R.fr(); R.tapI(mn == 0 ? IC_FEED : IC_PLAY); R.fr();
        if (g->scr != S_MENU) { g->pet.sleeping = false; continue; }
        g->pet.d.hunger = 50;
        R.tapD(dxm, dy);
        int what = -1;
        if (mn == 0 && g->scr == S_ANIM) what = g->an_ == AN_EAT_MEAL ? 0 : g->an_ == AN_EAT_SNACK ? 1 : g->an_ == AN_EAT_DIET ? 2 : -1;
        if (mn == 1) what = g->scr == S_DODGE ? 0 : g->scr == S_RUN ? 1 : g->scr == S_YEET ? 2 : g->scr == S_LITTER ? 3 : -1;
        if (what >= 0) hits[what]++;
        if (what != last) bands++;
        last = what;
        g->debugAct(-1); R.fr();
      }
      char b[96];
      if (mn == 0) snprintf(b, sizeof b, "feed menu: MEAL/SNACK/DIET rows %d/%d/%d dots, no row split", hits[0], hits[1], hits[2]);
      else snprintf(b, sizeof b, "play menu: MOVE/RUN/YEET/POP cells %d/%d/%d/%d dots, no split", hits[0], hits[1], hits[2], hits[3]);
      check(hits[0] > 0 && hits[1] > 0 && hits[2] > 0 && (mn == 0 ? bands == 3 : (hits[3] > 0 && bands == 4)), b);   // (play: two columns swept, top to bottom)
    }
  }
  // home screen: things never drawn on top of each other (where there's room), and a tap on each lands on it
  void home() {
    printf("home screen\n");
    int scenes = 0, overlaps = 0, forced = 0, tapsOk = 0, taps = 0;
    const struct { float age; uint8_t pad; } looks[] = {{600, 0}, {BABY_END + 60, 1}, {BABY_END + 60, 2}, {BABY_END + 60, 3},
      {CHILD_END + 60, 3 | 1 << 2}, {CHILD_END + 60, 1 | 2 << 2}, {CHILD_END + 60, 1 | 3 << 2},
      {TEEN_END + 60, 41}, {TEEN_END + 60, 37}, {TEEN_END + 60, 39}, {CHILD_END + 60, 5},
      {TEEN_END + 60, 69}, {TEEN_END + 60, 105}, {TEEN_END + 60, 42}};
    uint32_t rr = 777;
    auto rnd = [&](int n) { rr ^= rr << 13; rr ^= rr >> 17; rr ^= rr << 5; return (int)(rr % (uint32_t)n); };
    for (auto& L : looks) for (int k = 0; k < 120; k++) {
      PetData& d = g->pet.d;
      const uint8_t hb = (uint8_t)(rnd(4) * 0), gift = (uint8_t)(rnd(3) == 0 ? 1 + rnd(4) : 0), mess = (uint8_t)rnd(3), drop = (uint8_t)rnd(3);
      const float startX = (float)rnd(30);
      float settledX = 0;
      // the same scene, set up afresh before each tap
      auto scene = [&]() {
        d.age = L.age; d.pad0[0] = L.pad; d.exploring = 0; g->pet.sleeping = false; g->pet.napping = false; d.pad1[1] = 0; d.energy = 90;
        d.bubblesOnFloor = hb; d.giftOnFloor = gift; d.floorMess = mess; d.droppings = drop; d.giftAge = d.messAge = 0;
        g->lastForm_ = g->form(); g->pottyPending_ = 0; d.digestTimer = 9999; g->msgT_ = 0; g->clockShowT_ = 0; g->petCd_ = 0;
        g->setScreen(S_MAIN); g->blobX_ = g->blobTx_ = startX;
        g->stepT_ = 99; g->updateBlobWalk();   // the walking limits only (no random steps), so every tap sees the same scene
        settledX = g->blobX_; g->blobTx_ = g->blobX_;
      };
      scene(); scenes++;
      int idle, alt, sl; g->blobSprite(g->form(), idle, alt, sl);
      const int cw = spriteW(idle), cx = (int)settledX;
      const int itemsEnd = hb * 4 + (gift ? 8 : 0);
      const int messStart = BOX_X - 4 - (mess ? (mess - 1) * 4 : 0);
      const bool overItems = itemsEnd && cx < itemsEnd;
      const bool overMess = mess && cx + cw > messStart;
      const bool overBox = false;                                        // (no box on screen any more)
      const int room = (mess ? messStart : MW) - itemsEnd - (itemsEnd ? 1 : 0);
      if (overItems || overMess || overBox) { if (room < cw) forced++; else overlaps++; }
      // taps on the middle of each thing
      if (gift && !overItems) { scene(); taps++; R.tapD(3, FLOOR - 2); R.fr(); if (!d.giftOnFloor) tapsOk++; else printf("      miss gift: look %d x %d w %d\n", (int)g->form(), cx, cw); }
      if (!overBox && !overMess) { scene(); taps++; R.tapD(cx + cw / 2, FLOOR - spriteH(idle) / 2); R.fr(); if (g->scr == S_ANIM && g->an_ == AN_PET) tapsOk++; else printf("      miss fella: look %d x %d w %d scr %d an %d hb %d->%d gift %d->%d clock %.1f catX %.1f sl %d\n", (int)g->form(), cx, cw, (int)g->scr, (int)g->an_, hb, d.bubblesOnFloor, gift, d.giftOnFloor, g->clockShowT_, g->blobX_, (int)g->pet.sleeping); }
      if (!drop || cx + cw <= MW - 10) { scene(); taps++; R.tapD(MW - 5, FLOOR - 6); R.fr();   // the smell at the right edge opens the litter game when there is poop
        if (drop ? g->scr == S_SCRUB : g->scr != S_SCRUB) tapsOk++; else printf("      miss smell: look %d x %d w %d scr %d drop %d\n", (int)g->form(), cx, cw, (int)g->scr, drop); }
    }
    char b[120];
    snprintf(b, sizeof b, "%d scenes: fella never stands on items/box when there's room (%d did)", scenes, overlaps); check(!overlaps, b);
    printf("      (%d scenes had no room: a wide fella with a full floor)\n", forced);
    snprintf(b, sizeof b, "taps on the gift, the fella and the box land (%d of %d)", tapsOk, taps); check(tapsOk == taps, b);
  }
  void evolve() {
    printf("look picker\n");
    PetData& d = g->pet.d; d.age = BABY_END + 60; d.pad0[0] = 0; g->pet.sleeping = false;
    R.wait(1.0f + LEVELUP_SECS);   // (the level-up animation plays first)
    if (g->scr != S_EVOLVE) { check(false, "picker opens"); return; }
    R.wait(0.6f);
    int prev = g->evoSel_; R.tapD(2, 14); R.wait(0.1f); const bool left = g->evoSel_ != prev && g->scr == S_EVOLVE;
    prev = g->evoSel_; R.tapD(MW - 2, 14); R.wait(0.1f); const bool right = g->evoSel_ != prev && g->scr == S_EVOLVE;
    R.tapD(11, 14); R.wait(0.1f); const bool leftEdge = g->scr == S_EVOLVE;
    R.tapD(MW / 2, 14); R.wait(0.1f); const bool pick = g->scr == S_ANIM;
    check(left && right && leftEdge && pick, "arrows flip both ways, the middle picks");
  }
  int run() {
    fresh(); icons(); pages(); fresh(); menus(); fresh(); home(); fresh(); evolve();
    delete g; g = nullptr;
    printf(fails ? "touchmap: %d FAILED\n" : "touchmap: all ok\n", fails);
    return fails ? 1 : 0;
  }
};
}  // namespace lf
static int cmdTouchMap() { auto* t = new lf::TouchMap(); int r = t->run(); delete t; return r; }
