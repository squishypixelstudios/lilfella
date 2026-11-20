# Lil Fella: changes

## v0.09 merge, volume 0-5, hold-to-repeat clock (2026-11-18)
- **Merged** the browser-built v0.08 (LOCK, no training, wet synthesised sounds, litter box, Tree page) with the v0.07 follow-up work.
- **Sound volume is 0-5** (0 off, 5 max). The default is 2, a quarter of the old MID: the first-run clock page was far too loud.
- **Auto off** gains **30 Sec** and **1 Min** (5 Sec, 15 Sec, 30 Sec, 1 Min, Never).
- **Set clock:** hold a finger on an arrow to keep changing the value; it speeds up the longer you hold. Only the first press clicks.
- **Sound reliability:** the amp gets a short run of silence to wake up before a sound, so short taps are no longer swallowed, and a full queue drops the oldest sound instead of the newest.
- **New level-up fanfare** (`SND_LEVEL`): a brighter, higher, rising run that plays with the new stage title (LIL BLUNKUS etc.) instead of at the start of the animation.
- **New blob landing:** the "uh oh" cry now ends as the drop lands; the old jingle that played as it began to fall is gone.
- **Title screen:** after the clock is set on a brand new device, "LiL FeLLa" drops in a letter at a time and all ten Lil Fellas parade past to a looping tune; tap to start. (`art/title-preview.html` shows the three designs that were tried.)
- **Digesting:** the food inside a Lil Blob now dissolves in a scrambled order (the leftover dots used to read "54").
- Sprout is named `TN_SPROUT` in the code (it is a Lil Blunkus; it was `AD_SPROUT`). Saves are unaffected.
- Speed (Settings, page 2) is a developer option and is to be removed before the final version.

## v0.08 LOCK, no training, sounds for everything (2026-11-13)
- **Sounds for everything, and they're wet:** all the old beeps are gone. 39 synthesised sounds made of water-drop bloops, squelches and bubbles: taps, menus opening and backing out, page flips, refusals (ZZZ, TIRED, OUT, full up), the light switch, serving food, gulps and chomps, the medicine glug and its sparkle, a queasy gurgle when they fall ill, the squat and the plop, wet scrubbing with a pop per poop, the padlock, POP's bubbles and splats, hearts earned, the YEET band stretching, the whoosh, bounces and the landing splat. Their voice (the coos, yawn, grumble, purr when petted) is rounder and gargly. Every sound comes out slightly different each time.
- Up to four sounds play at once on the device (it was one, each cutting off the last). A tap only makes its plain bloop when it didn't cause a sound of its own.
- The sounds are defined once, in `src/game/synth.cpp`, and the web page plays the very same ones (it has a "hear the sounds" panel). They were tuned by the numbers, not by ear: say which ones to change.
- **LOCK** (the padlock icon, where Train was): for when you love the fella you have. It asks LOCK? (YES / NO); locked, they don't age, don't grow into the next stage and can't die (no ending of any kind, not even old age). Everything else carries on: hunger, poop, sickness, sleep and their calls. The padlock stays lit while locked. Tap it again for UNLOCK? and the game goes on from exactly where it stopped; a badly neglected fella gets a little health and trust back at that moment so nothing ends the second you unlock.
- **Training is gone:** no more mischief (knocked-over cup, scratch marks), good moments, feather wand, clicker or TRAIN bar. The Meter has 5 pages (it was 6). Feeding is never refused for mischief.
- **Big Blorbus** is offered at 8 days old with nothing else needed (it used to need a full TRAIN bar).
- **Lessons:** a Lil Blob now has 7 things to learn (the NAUGHTY lesson is gone; Lock isn't a lesson).
- Saves from v0.07 carry over; the old training points are dropped and everyone starts unlocked.
- **The litter box is on the home screen** (bottom right), but only while there's poop in it waiting to be cleaned: a tray with the poops piled in it (up to 6 shown) and the smell above. They keep off it, walk over to it when they need to go (it comes out for them), and it's put away once it's scrubbed clean. Tap it to clean.
- **Tree page:** each stage now shows when they reach it and what else it takes (15 minutes + every care icon tried; 1 day; 3 days; 8 days), with a "Growing up" section spelling it out.
- **No more "chonk":** nothing on screen mentions weight or chonk. It still works under the hood; the ending it can cause is called **Overfed** (it was "Too chonky"), in the ending caption and in Old friends.

## v0.07 lessons for real, battery, YEET feet (2026-11-11)
- **"ALL LEARNED" has to be earned:** the medicine lesson only counts when a sickness is actually cured, and the
  light lesson only when the light is turned off while they're asleep (tapping those icons at other times no longer
  counts). The starter bug is never skipped any more, and the test nap now comes at 6-9 minutes old (it was 45-58,
  long after a Lil Blob should have grown up). A nap slept through with the light on comes round again 3 minutes
  after they wake; a bug that went by without medicine is given again.
- **Bedtime waits 30 minutes for a new Lil Blob:** one made during sleep time falls asleep 30 minutes afterwards. Naps aren't held back: the test nap comes inside those 30 minutes, and one still going at the 30-minute mark runs on into the night (the light lesson counts the moment the light goes off). If the lessons
  aren't finished when it falls asleep, the rest carry on in the morning.
- **Tap in the dark:** with the light off, a tap on the LCD shows the time.
- **Name and age page:** CHONK is gone; in its place the device's battery (percent, a "+" while charging, and a
  10-segment cell). "H" is now "HRS", one dot further from the number.
- **YEET:** coming down fast, the camera lagged behind and their bottom rows were off the screen when they landed.
  The camera now always keeps their feet (and the ground) in view.
- The simulator's tests (`sim/`) haven't been re-run against these rule changes: its lesson tests expect the old rules.

## v0.07 touch fix for the LCD-1.83, fading suds (2026-11-11)
- **LCD-1.83 touch:** its touch chip is now told to stay awake and report the finger continuously (as Waveshare's
  demo does). At its defaults it only reported now and then, so drags (the scrub, MOVE, YEET) barely registered and
  taps could land on a stale spot (the clock page's OK). The boot log says whether both settings took.
- **Scrub:** the suds fade about a second after they're made instead of piling up (all boards).

## v0.07 with sound (2026-11-11)
- **Sound is switched on** on all three boards (it was compiled out): the ES8311 codec and speaker, volume in
  Settings → Sound. The LCD-1.83 uses the same audio pins as the AMOLED-1.8.
- **Sounds added where there were none:** the light switch (a soft click off, a bright one on), a tap on the empty
  LCD (the clock), plain play with the wand, the dropper's fall (whistle, "uh oh", the spring up, a hello), the new
  look's hello after growing up, MOVE (a chime for each heart earned, a thud for every miss), the ending (the melt,
  the last suck up the tube, a coo when the dropper wants a shake, a rattle per shake), going exploring and waking
  up in the morning. An ending's caption is now a sad sound (a fanfare when recycled), with the melt sound when
  the melting starts.
- **Not heard yet:** it compiles for all three boards; nobody has listened to it on one.

## v0.07 on the LCD-1.83 (2026-11-11)
- **A third board:** the Waveshare ESP32-S3-Touch-LCD-1.83 (240x284 IPS LCD, ST7789 over SPI, CST816 touch). Build
  it with `pio run -e lcd183`. The game is identical; only `src/hal/` and `platformio.ini` changed.
- **Picture:** the panel is smaller than the frame at 2x, so the frame is scaled by 5/4 (to 230x280) with
  area blending; it comes out about the same physical size as on the AMOLEDs. Touches are scaled back to match.
- **Brightness** is the backlight (PWM), not a panel command. Sound isn't set up for this board.
- **Touch:** one shared read path for all three boards now; the AMOLED-1.8's behaviour is unchanged.
- **Not yet run on a board:** it compiles. The README has a first-run checklist (row offset, colours, backlight, touch).

## v0.07 on the AMOLED-2.16 (2026-11-11)
- **A second board:** the Waveshare ESP32-S3-Touch-AMOLED-2.16 (480x480). Build it with `pio run -e amoled216` (now
  the default); the 1.8 is `pio run -e amoled18` and is unchanged. The game itself is identical: only `src/hal/` and
  `platformio.ini` changed.
- **Picture:** the same 368x448 picture, centred on the square panel with the LCD's background shade around it (the
  two panels have almost the same pixel size, so everything is the size it was).
- **Hardware differences handled:** the display clock and reset pins, no IO expander (the reset lines are plain
  GPIOs), the CST9220 touch controller (through SensorLib, turned a quarter turn like the picture), new audio pins
  (sound is still off), and the board's extra key also wakes the screen.
- **Not yet run on a board:** it compiles, and follows Waveshare's demo for the pins and set-up. The README has a
  short first-run checklist (picture orientation, touch orientation, holding and dragging).

## v0.07 (2026-11-10): hardware release
- **Sickness, reworked:** no more random sickness. There's one starter bug in the first 5 minutes (the medicine lesson: it doesn't count as one of their sicknesses, and leaving it isn't a care mistake). After that they only get sick when **hunger hits 0** (once, until they've been fed back up) or when **the light stays on for more than 15% of a night's sleep** (sick when they wake). Sicknesses **stack**: each needs its own dose of medicine ("1 MORE" until the last), and each untreated one drains health faster. A sickness knocks 25 health off; each dose gives 25 back. The meter's last page counts this fella's sicknesses.
- **Sick looks for every fella:** a speckled body, droopy half-shut eyes and a shiver, plus an icon beside the head for each kind (a germ, a crossed-out bowl, a lit bulb), with the count when there's more than one ("SICK X2" from the attention icon too). A fella very low on health without a sickness shows the skull.
- **Night feeds:** waking them in the night (to feed them) no longer turns the light on.
- **Clean is a scrub now:** the box and the mess fill the screen; rub a finger back and forth and the suds build up until it wipes clean (3-5 s scrubbing briskly; you can't fail). Leaving early cleans nothing.
- **POOP UP** (the old clean-up game) is in the Play menu: a "POP EMPTY ONLY" card first, 6 poop bubbles to float away, then a score (hearts by splats).
- **YEET** (Play menu): a card ("PULL BACK AND RELEASE TO YEET!" and the best so far), tap, then press anywhere and drag back to stretch the sling (a dotted line shows the way), let go. Out past the edge: trampolines, springs, bumpers and gusts help; rocks, birds, clouds and mud don't. Score in metres; the best is saved; hearts raise fun. Physics is shared by each life stage (Big Blorbus is heavy: barely bounces, rolls far, ploughs through birds and smashes rocks); every look has its own flight, trail, landing and shout.
- **GUESS is gone.** The Play menu is four games in four big cells: MOVE | RUN, YEET | POP.
- **Renamed:** DODGE is now **MOVE**, POOP UP is now **POP**.
- **Feed is a meal, every time:** no more MEAL / SNACK / DIET menu; the Feed icon just serves a meal (still refused while they're up to mischief or completely stuffed). Big Blorbus's Feed is still Orbit Catch.
- **Scrub, more dynamic:** the litter box is long now (most of the screen), floor mess in front of it at the left. Every poop is different: four shapes (nugget, swirl, log, pile), placed differently each time, and each is easy or stuck-on (1-5x the rubbing; tough ones look crusty, the toughest have flies). Rubbing wears away the poop nearest your finger, dot by dot. All easy still takes 3-5 s; on average a clean takes about twice that, never more than 5x.
- **Medicine can't be cut short:** the dose works the moment you give it, and the animation always plays out (no icon, tap, hold, growing up or the web page's Home button interrupts it).
- **YEET: no more cut-off feet.** Turned on their side, stretched or puffed up, a fella's picture could be taller than its body and was drawn into the ground; now it's always drawn above the ground line.
- **Games left alone** go home after 30 s (they used to keep the screen on for 5 minutes).
- **Bubbles currency removed:** no more floor bubbles or rewards; a found gift is now a "THANKS!" and a little happiness.
- **Overlaps fixed:** POOP UP's hint has its own card; "CLEAN!" is on a clear screen; Dodge/Run/Orbit scores are drawn on a clear screen with the rows spaced; Old friends shows "N blobs raised so far" only in a free row (never over the buttons, never with an empty log); the meter's last page clears the page dots; the dropper's PUSH sits under the tip.

## v0.06 (2026-11-09)
- **Journal (Settings, page 2):** every look you've grown is marked, sorted by stage: Lil Blob, Lil Blub, Lil Blunkus, Lil Fella, Big Blorbus, each with a got/total count. Looks you haven't met yet show "?"; tap one to see its name. Lil Blob is in from the start, and old saves fill in from your old friends and the current fella.
- **Name page:** NEW NAME and FILL IT are gone. Type your own name on the keyboard (up to 10 letters, DEL and a space key), or tap Random name to cycle through the random ones, then tap Let's goo! to start.
- **The end-of-life dropper takes more work:** squeeze and let go three times to suck up the puddle (a third each time), then shake the device three times to shake the dropper, then tap once to squeeze out a new blob. (If the motion sensor never answers, taps count as shakes after 15 s.)
- **RUN is harder:** double jump (tap in the air), birds (stay low), holes (jump them), tall pillars (double jump), and a power-up bolt (5 s faster + invincible, but holes still get you). A look-ahead test proves there's always a way through.
- **Clean icon is soap and bubbles; the litter game is now BUBBLE POP RUSH:** poop bubbles float up and away (that's the cleaning); clear bubbles hover in their way and block them, so pop the clear ones. Tapping a poop bubble splats it (a miss, redone). No splats = perfect (+1 bubble). RUN's bird is now a gull.
- **Eating is absorption now (Lil Blob and Lil Blub):** the body swells to a bit bigger than the food, wraps round it, and draws back with it inside. Stretching no longer drops the bottom row of pixels off the LCD.
- **Bubble Pop Rush look:** empty bubbles are dark (pop these), poop bubbles are greyed out (leave them), way more empty bubbles (a screenful), and the "x LEFT" counter is gone. A row of little grey poops along the top shows how many are left (each leaves a faint dot when its bubble floats away).
- **RUN:** the bird is now one steady bird with both wings spread (no flapping or blinking). 4 hearts (PERFECT) now needs 300 m and a double jump over a tall wall; one tall wall is always sent your way after 120 m if you haven't cleared one. 1-3 hearts are unchanged (12 / 35 / 65 m).
- **Lil Blob lasts 15 minutes** (was 90). **Poop pacing by stage:** Lil Blob every 3 min, Lil Blub 6, Lil Blunkus 10, Lil Fella 15, Big Blorbus 20; the first poop comes within 3 minutes. The cleaning game is tested at every stage of life.
- **No sleep in a new Fella's first 30 minutes,** even at bedtime. After that, normal night sleep hours apply.
- **Bubble Pop Rush checked for winnability:** a bubble can only be blocked by a clear bubble you can tap, no deadlock is possible, and sim players (careful, careless and button-mashing) win every board.
- **ORBIT CATCH replaces STARS (Big Blorbus, in the Feed slot):** the fella is a little planet; a satellite circles it on one of two orbits, tap to hop orbits, catch stars, dodge rocks (3 hits = over, 30 s). Never an unavoidable rock.
- **Every look eats its own way.** Lil Blob keeps the absorb; every other look has a one-of-a-kind routine, ported dot for dot
  from the approved Eating Lab previews (`src/game/eatfx.cpp`, checked frame by frame against `art/eating/engine.js`):
  Blobbert sneezes it out (ah... CHOO!) and re-absorbs it, Mochi wraps it into a daifuku, Cheeks balloons up and floats,
  Bean rattles and hops, Bun splits open, Slosh slurps and spills, Mallow toasts over a campfire, Wobbles slinkies,
  Toasty makes a sandwich, Curly whistles like a teapot, Sprout blooms, Jelly belly-flops, Puddle glugs, SpikyBoi's head
  springs over, Marshy plays pinball (left or right wall, random each feed), Boop is a potion flask, Fubz coughs a hairball,
  Nimbus storms, Ballboi rolls in in his hamster ball and stuffs his cheeks, Puff vacuums, Loaf sploots, Dapper dines with
  a knife and fork. Chomp sounds play on the bites and gulps.
- **Big Blorbus snacks:** a good ORBIT CATCH round (3+ hearts, GREAT or PERFECT) ends with a snack, eaten their own way: Big
  Blobbert is a trampoline, Big Ballboi carries a pancake round his ball, the Alien Cat uses a tractor beam. They still never
  get hungry.
- **Fix:** the ORBIT CATCH result screen couldn't be tapped away (its clock stood still); a tap now leaves it.
- **Fubz:** the bottom of his body now meets the outline on both sides, and his open mouth is wide instead of a stray dot under it.
- **Bubble Pop Rush:** the hint now reads POP EMPTY ONLY (two lines, with a thin outline so it reads over the bubbles), and there are more empty bubbles: a row of up to 3 across the middle and another along the bottom, each on a free spot. The top is unchanged, and so are the rules.
- **Bubble Pop Rush:** empty bubbles now move for real: bubbles that land on top of each other ease apart to make room, rims that touch bounce off each other (and off the screen edges and poop bubbles), and a gentle drift replaces the old fixed sway. Poop bubbles rise and get blocked exactly as before.
- **ORBIT CATCH is now about eating planets:** the fella is a little round chomper circling a sun on the two orbits. Its mouth faces the way it's going, opens wide as a tasty planet comes up (ringed ones, ones with a moon) and gulps it with a chomp and a burst of crumbs; it gets a little fatter with every planet. Spiky planets give it a tummy ache (eyes go x_x; 3 and the round is over). A hint of who's playing: Big Ballboi rolls in his hamster ball, the Alien Cat has ears, Big Blobbert drips. Rules, timing, scoring and fairness are unchanged.
- **Level-up animation:** before the "Pick new form" screen opens, the fella swells and shrinks three times (a little bigger each time) with stars gathering round, a boing on each swell and a sparkle on the last, then a flash and the picker with its fanfare. 2.5 s; never while they're asleep.
- **Code clean-up (no change to how the game plays):** removed unused code (two functions, three sprites, a constant, a dead parameter); the display now sends only the rectangle that changed inside each band, comparing against a copy of what's on screen instead of fingerprinting all 41 KB every frame; the dot matrix and the icons are written straight into the frame; the eating routines' circle tests no longer take a square root per dot (same dots, checked against the lab). Checked with two static analyzers, address/undefined-behaviour sanitizers and a memory checker for uninitialized reads: no bugs found beyond the ORBIT CATCH one fixed above. `./sim bench` times a frame. A full copy of the code before this pass is kept as `lilfella-backup-2026-11-09-v0.06`.

## v0.05 (2026-11-05)
- Eating (Lil Blob and Lil Blub): the food drops in, they hop back in surprise, wobble slowly over, stretch their whole body
  round the food and draw back with it inside; the view zooms in (2x), the food sits inside for a moment, then softens and
  dissolves in place (a few small motes rise).
  Lil Blunkus and Lil Fella still eat in bites. (Their old mouth-open eating frames are gone.)
- The Lil Blub look "Round" is now **Blobbert**: bigger and perfectly round, just eyes (no ears, mouth, tail or legs),
  asleep and in the games too.
- Mochi is smaller (13 dots wide instead of 16), in line with Blobbert and Cheeks; its game-size version too.
- **Evolution tree:** each pick limits the next. Blobbert leads to Sprout, Bun, Curly or Slosh; Mochi to Wobbles, Mallow or
  Toasty; Cheeks to Bean, Bun or Slosh. Final forms depend on the path (see the tree page).
- New looks: Slosh, Wobbles, Mallow, Toasty (Lil Blunkus) and Marshy, Boop, Fubz, Nimbus, Ballboi (Lil Fella). Sprout moves to
  Lil Blunkus, Puff to Lil Fella, Dango is now SpikyBoi, Curly has a silly curl on top. Wobbles' finals (Jelly, Puddle) are a
  stand-in until it gets its own.
- The picked looks are packed differently in the save (the save layout is unchanged); a fella from before is re-packed on start,
  and keeps its look where the tree still has it.
- "Caught being good" (the blinking +) is much rarer: about 2 in the first hour, then every 1-2 h as a Lil Blub, 2-4 h as a Lil
  Blunkus, 4-7 h as a Lil Fella; it lasts 30 s (was 45), and stops once the TRAIN bar is full.
- **New game, RUN (for every look):** a side-scroller in the PLAY menu. They run on the spot, tap anywhere to jump crates, rocks and
  spiky mats coming from the right; it speeds up. Hearts for distance (4 hearts at 100 m).
- **New game, STARS (Big Blorbus, in the Feed slot):** four stars flash in an order, tap them back; each round adds one.
- **Big Blorbus (stage 5):** from 8 days old, a fella with a full TRAIN bar can grow into one (pick it or STAY as they are):
  Big Blobbert (Blobbert line: from Jelly or Puff), Big Ballboi (Cheeks line: from Ballboi), Alien Cat in a Tuxedo (from Nimbus or Loaf).
  They never get hungry (hunger hearts always full) and don't eat: the Feed icon opens STARS instead. Big Blobbert is wider than
  the screen's roaming room and squishes against the sides; Big Ballboi's hamster slides around inside the ball and slams into
  the wall when the ball stops. The games use the Lil Fella they grew from.
- Tree: Wobbles can also become Nimbus, Toasty can also become Marshy, Curly gets Fubz instead of Jelly.
- Night theme: the unselected icons are a clear mid grey.
- **Big Blorbus (stage 5):** from 8 days old, a fella with a full TRAIN bar can grow into one (pick it or STAY as they are):
  Big Blobbert (Blobbert line: from Jelly or Puff), Big Ballboi (Cheeks line: from Ballboi), Alien Cat in a Tuxedo (from Nimbus or Loaf).
  They never get hungry (hunger hearts always full) and don't eat: the Feed icon opens STARS instead. Big Blobbert is wider than
  the screen's roaming room and squishes against the sides; Big Ballboi's hamster slides around inside the ball and slams into
  the wall when the ball stops. The games use the Lil Fella they grew from.
- Tree: Wobbles can also become Nimbus, Toasty can also become Marshy, Curly gets Fubz instead of Jelly.
- Night theme: the unselected icons are a clear mid grey.
- **New litter game:** a side view of one litter box split into three sections (L, M, R). A poop is buried in one; tap a section
  to dig there. Digging is slow, so a wrong guess costs time (the shovel works for ~2 s, then an X). Right first time on every
  round is a perfect clean (+1 bubble). One round per poop; leaving early keeps what you got; floor mess is swept up when you finish.
- The litter box is no longer on the main screen: a full one just smells (stink at the right edge, tap it to clean), the
  fella goes in place, and mess on the floor still shows.
- Fixed: the eating zoom drew the fella a row below the bottom of the screen.
- Fixed: after a trip to the litter box a fella could occasionally stay standing beside it instead of wandering back.
- The first HELLO! asks for attention 1-2 s after the drop splats (it used to be about 90 s later).
- Look picker: "Pick new form:" above the screen (the top icons step aside), and the look's name sits a little lower.

## v0.04 (2026-11-05): hardware release
Ready-built firmware: flash `lil-fella-v0.04-merged.bin` at address 0x0 (see the README). Saves carry over.

- Icons: the litter scoop moved to the top right, medicine to the bottom left.
- Litter game:
  - it takes three swipes over a spot to uncover the poop (the litter thins with each; a hint after two; scrubbing
    back and forth counts every stroke);
  - a quick scoop tap counts even if the finger rolls a little as it lifts (before, many real taps were taken as
    raking, so the game couldn't be finished);
  - once something is uncovered and you lift your finger for a moment, the uncovered clumps blink and the top line
    alternates "TAP POOP" with the count;
  - a little more time for the fast-clean bonus.
- A fella no longer poops in their sleep (they hold it until they wake), so they don't wake up for a trip to the box
  and doze off beside it; after the box they wander back to where they were.
- Text on the full-screen pages (name, Settings, Old friends) shows the tails of g, j, p, q and y (they were cut off).

## v0.03 (2026-11-05): hardware release
Ready-built firmware: flash `lil-fella-v0.03-merged.bin` at address 0x0 (see the README), or build with PlatformIO as
before. The save carries over from v0.02 and v0.01.

- The litter box only appears when there's poop in it (or while they go to use it); the rest of the time the whole
  floor is theirs.
- Picking the device up no longer turns the screen on: a tap or the PWR / BOOT button does, and that first tap only
  wakes the screen. A poop, growing up or an ending no longer switch the screen on by themselves either.
- A gooier landing: the drop splats flat and spatters, the puddle (eyes first) pulls itself back together, springs
  up too tall, squishes and wobbles into the Lil Blob; the spattered droplets slide back in (the squeeze is now 7.6 s).
- A brand new Lil Blob stays awake for its first 2 minutes, even at bedtime, so you can look after it first.
- No more cats anywhere: bubbles (which they blow) replace hairballs as the currency, exploring replaces hunting and
  they bring back found treasures (flower, shell, pebble, berry, gem) instead of mice and birds; a goo trail replaces
  paw prints; the hidden breeds and sounds have blob names. The attention icon is a round blob (no ears) with a !.
- Auto off: 5 Sec (default), 15 Sec or Never; Never really keeps the screen on now (dimmed after 45 s untouched).
- Faster screen updates: only the parts of the screen that changed are sent to the display.
- Smaller firmware: with sound off, the audio code isn't compiled in; leftover code and save fields from the old cat
  game are gone (the save layout is unchanged, so existing fellas carry over; the save moves to its own storage name).
- No fixed default name: every new blob starts with a random name (NEW NAME for another).
- Settings has a third page: New blob and **Erase everything** (asks first; back to the first-start clock page).

## v0.02 (2026-11-04): hardware release
Flash with PlatformIO (VS Code: open the folder, **Upload**). The save format is unchanged: a v0.01 fella carries over.

**Birth and the dropper**
- First start: a lab eyedropper (grey rubber bulb, thin glass, goo inside) hangs on the LCD. Push it and a drop
  squeezes out and falls a long way, past the clouds, to splat on the floor as a Lil Blob (6.4 s).

**Endings**
- Every life ends as a liquid. Captions: BYE BYE / THANK YOU (recycled) or OH NO and the reason. They melt into a
  puddle, the dropper comes down and waits: **press to squeeze the bulb, let go to suck them up** (a tap does both).
  It rises, shakes, then NEW BLOB.
- Endings: **Recycled** (was Retired, ~1 month of good care; keeps bubbles + bonus), **Too chonky**, **Too sick**
  (health runs out), **Too sad** (12 awake hours in a row at zero happiness), **Lost trust** (was Ran away).
- An untreated sickness now gets worse until you give medicine. Health and sadness only drop while they're awake, so
  a night's sleep is safe; time with the device off never ends a life. Simulated: 3 check-ins a day (8am, 3pm, 10pm)
  is a full life; twice a day isn't.
- No more black hole.

**Old friends**
- Settings page 2 → Old friends: the last 8 fellas with their look, name, how it ended, age and stage, plus how many
  blobs you've raised. Starting over from Settings is logged too.

**Growing up and upkeep**
- Stages: Lil Blob → Lil Blub at 90 min → Lil Blunkus at 1 day → Lil Fella at 3 days (was 2 and 7 days).
- Bedtimes by stage: Lil Blob 8pm-10am, Lil Blub 9pm-9am, Lil Blunkus 9:30pm-8:30am, Lil Fella 10pm-8am.
- First hours: 5 poops in hour 1 and 4 in hour 2; a sickness by 30 min (the medicine lesson) and another at 1-2 h;
  one sleepy nap at 45-60 min (the light lesson); 4-5 good moments in hour 1, 2-3 in hour 2. All of it eases off
  as they grow.

**Controls and screen**
- The light icon is a plain on/off switch (no menu).
- Auto off: laid flat and untouched for 5 s, the screen turns off (Settings → Auto off: FLAT 5S / NEVER).
- Settings is two pages with taller rows.
- Bubbles are no longer collected automatically; a buried one in the litter must be found before the last clump.
- Touch: the nearest target wins when two are close; every icon, page row and home-screen object is checked in the
  simulator (`./sim touchmap`).

**Games**
- Guess: the young lean toward the side they'll jump.
- Dodge: Lil Blob and Lil Blub play small; drops vary in shape and size; bigger looks get smaller, fewer drops.

**Cleanup**
- Old looks, the light menu, hard mode, the alarm sound, the black hole, the angel wings and the
  run-away walk-off are removed from the code.

Sound is still off in this build (`LF_AUDIO 0` in `src/hal/hal.h`) until the audio chip is checked on the board.

## v0.01
First labelled build.
