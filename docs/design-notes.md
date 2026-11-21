# Lil Fella: design decisions

- **Look:** pure 4-shade monochrome like the original (LCD grey palette; optional inverted "night" palette).
- **Screen:** 40x28 dot matrix (8 physical px per dot); 4 icons above (feed, light, play, litter), 4 below (medicine, meter, lock, attention).
- **Input:** touch only; icons are tapped directly. The touch controllers are left at their defaults, except the LCD-1.83's CST816, which is told to stay awake.
- **One mode.** First start = clock, then name (random each time), then push the dropper to squeeze out the first blob.
- **Sleep:** by the clock, like the original: bedtime gets shorter as they grow (Lil Blob 8pm-10am ... Lil Fella 10pm-8am).
  The screen turns off when the device is left untouched for 30 s (Settings: Auto off); only a tap or the button turns it on.
- **Art:** from the graph-paper drawings (`art/sprites.py` notes which drawing each sprite is). Lil Blob has no legs. A new blob comes out of a lab dropper.
  New art in the same style: Mochi, Cheeks (from the full-cheeks drawing), Dapper, Loaf, the blob teens (Bean, Bun, Curly) and adults (Puddle, Jelly, Sprout, Dango, Puff), eating frames, litter box,
  padlock (locked and open).
- **Stages:** Dropper (push it) → Lil Blob → Lil Blub (1 hour) → Lil Blunkus (1 day) → Lil Fella (3 days); the player picks
  the look at every stage. Lil Blob must try the 7 care icons first (Gen 2-style lessons). Upkeep eases off with age.
- **Endings:** every life ends as a puddle the player sucks back up with the dropper (press = squeeze, let go = suck): Recycled (~1 month), Too chonky, Too sick (health 0), Too sad (12 awake hours at zero happiness), Lost trust (they used to run away; now they dissolve like the rest). Health and sadness only build while awake; offline never ends a life. Old friends (Settings) logs the last 8.
- **Eating:** Lil Blob absorbs its food; every other look has its own silly eating routine (`eatfx.cpp`, a port of the
  Eating Lab previews in `art/eating/`). A routine draws the whole 40x28 scene for a time t; the lab and the game share the
  maths and rounding so they match frame for frame. Big Blorbus only eats as the reward for a good ORBIT CATCH round.
- **Lock (v0.08, replaces training):** a padlock icon. Locked = no ageing, no next stage, no endings; needs, sickness and calls carry on. Unlocking picks up where it stopped (a neglected fella gets a little health and trust back so nothing ends at once). Big Blorbus now only needs the age (8 days).
- **Mini-games:** MOVE (one hit; ~30 s ceiling; the young play small), RUN, YEET, and POP: a bubble shooter after Puzzle Bobble (v0.10). With no colours, its five kinds of bubble are shapes (X, triangle, square, empty ring, black); a mark inside a ring did not read at 5 dots across, so the X and triangle are the bubble.
- **Litter box:** only drawn when there's poop in it (or while they go to use it).
- **Weight:** hidden "chonk" (ideal 35, growing to 50 by adulthood); 100 dissolves them. There is no currency.
