# Lil Fella sprite source. Transcribed from the graph-paper drawings (one character = one LCD dot).
#   X = black dot   E = eye (black; hidden/changed when blinking)   M = mouth (black; animates when eating)
#   G = mid-grey dot (rubber, goo: anything that should read lighter than the outline)
#   . = empty
# Run:  python3 sprites.py            -> contact sheet PNG (for checking against the photos)
#       python3 sprites.py --cpp DIR  -> DIR/sprites_data.cpp + DIR/sprites.h
import sys

S = {}

def spr(name, txt, src=''):
    rows = [r.strip() for r in txt.strip('\n').split('\n')]
    w = max(len(r) for r in rows)
    rows = [r.ljust(w, '.') for r in rows]
    S[name] = (rows, src)

# ---------------------------------------------------------------------------------------------
# THE DROPPER: where a Lil Blob comes from. Push the rubber bulb and a drop squeezes out, falls and
# splats into the first Lil Blob.
# ---------------------------------------------------------------------------------------------
spr('DROPPER', '''
...XXX...
..XGGGX..
..XGGGX..
..XGGGX..
..XGGGX..
.XXXXXXX.
...X.X...
...X.X...
...X.X...
...X.X...
...XGX...
...XGX...
...XGX...
....X....
''', 'eyedropper (option A): grey rubber bulb, thin glass, blob goo at the bottom')
spr('DROPPER_SQ', '''
.........
.........
.........
.XXXXXXX.
.XGGGGGX.
.XXXXXXX.
...X.X...
...X.X...
...X.X...
...X.X...
...X.X...
...X.X...
...XGX...
....X....
''', 'the eyedropper squeezed: bulb flattened, the goo pushed down to the tip')
spr('BLOBDROP', '''
.G.
GGG
GGG
.G.
''', 'the drop of blob goo on its way down')
spr('DROPLONG', '''
.G.
.G.
GGG
GGG
.G.
''', 'the drop stretched as it falls fast')
spr('SKYCLOUD', '''
..XX....
.X..XX..
X.....X.
.XXXXXXX
''', 'a little cloud rushing past while the drop falls from the sky')
# The landing, frame by frame: the goo splats flat, pulls itself back together, springs up too far, squishes
# down, and wobbles until it settles as the Lil Blob (BABY_IDLE). All sit on the floor.
spr('GOO_SPLAT', '''
...G......G......G...
.GGGGGGGGGGGGGGGGGGG.
GGGGGGGGGGGGGGGGGGGGG
''', 'landing 1: splat! the goo spreads out flat, spattering')
spr('GOO_POOL', '''
....GGGGGGG....
..GGGGEGEGGGG..
GGGGGGGGGGGGGGG
''', 'landing 2: a puddle with eyes, gathering itself')
spr('GOO_MOUND', '''
...GGGGG...
.GGGGGGGGG.
GGGGEGEGGGG
GGGGGGGGGGG
''', 'landing 3: the goo heaps up')
spr('BLOB_RISE', '''
..X...X..
.X.XXX.X.
.X.....X.
X.......X
X..E.E..X
X.......X
X.......X
.X.....X.
.XGGGGGX.
..XXXXX..
''', 'landing 4: it springs up too tall (goo still settling at the bottom)')
spr('BLOB_SQUASH', '''
.....X...X.....
...XX.XXX.XX...
.XX.........XX.
X....E...E....X
.XXXXXXXXXXXXX.
''', 'landing 5: squished flat on the way back down')
spr('BLOB_TALLISH', '''
...X...X...
..X.XXX.X..
.X.......X.
X...E.E...X
X.........X
X.........X
.X.......X.
..XXXXXXX..
''', 'landing 6: a smaller bounce up')
spr('BLOB_WIDE', '''
....X...X....
..XX.XXX.XX..
.X.........X.
X....E.E....X
X...........X
.XXXXXXXXXXX.
''', 'landing 7: a last wobble, a little wide')

# ---------------------------------------------------------------------------------------------
# BABY (right-hand page of the sideways photo, the little "nam?" set)
# ---------------------------------------------------------------------------------------------
spr('BABY_IDLE', '''
...X...X...
..X.XXX.X..
.X.......X.
X...E.E..XX
X.........X
.X.......X.
..XXXXXXX..
''', 'sideways photo, right page, middle-left')
spr('BABY_HOP', '''
..X...X..
.X.XXX.X.
X.......X
X..E.E..X
X.......X
.XXXXXXX.
''', '"nam?" (legs removed)')
spr('BABY_BLINK', '''
...X...X...
..X.XXX.X..
.X.......X.
.X..X.X..X.
.X.......X.
.X.......X.
..XXXXXXX..
''', 'middle-right (eyes closed)')
spr('BABY_SLEEP1', '''
...X...X...
..X.XXX.X..
.X.......X.
X........XX
X..XX.XX.XX
.X.......X.
..XXXXXXX..
''', 'top-right + Z')
spr('BABY_SLEEP2', '''
...X...X...
..X.XXX.X..
.X.......X.
X........XX
X..XX.XX.XX
.X......XXX
..XXXXXXX..
''', 'bottom-left + Z')
spr('S0C_IDLE', '''
...X...X...
..X.XXX.X..
..X.....X..
.X..E.E..X.
X.........X
X....X....X
.XX.....XX.
..X.....X..
..X..M..X..
...XX.XX...
''', '"full cheeks" drawing, now the Cheeks child look')

# Lil Blunkus (teen) blobs (2026-11-03)
spr('TN_BEAN', '''
...XXXXXX....
..X......X...
.X........X..
.X..E...E..X.
.X..E...E..X.
..X.........X
..X...M.....X
.X...M.M....X
X..........X.
X.........X..
.XXXXXXXXX...
''', 'new: Bean: leaning bean')
spr('TN_BUN', '''
.......X.......
......X.X......
....XX.X.XX....
..XX.......XX..
.X...........X.
X...E.....E...X
X...E.....E...X
X......M......X
X.....M.M.....X
.X...........X.
..XXXXXXXXXXX..
''', 'new: Bun: steamed bun, pinched top')
spr('TN_CURL', '''
....XXX.........
...X...X........
....XX.X........
...XXXXXX.......
.XX......XX.....
X..........X....
X..E....E..X.XX.
X..E....E..XX..X
X..........X.X.X
X...M..M...X..X.
X....MM....X....
.X........X.....
..XXXXXXXX......
''', 'new: Curly: round blob, curly tail and a silly curl on top')
spr('TN_BEAN_EAT', '''
...XXXXXX....
..X......X...
.X........X..
.X..E...E..X.
.X..E...E..X.
..X.........X
..X...M.....X
.X...M.M....X
X....MMM...X.
X.........X..
.XXXXXXXXX...
''', 'eating: mouth open')
spr('TN_BUN_EAT', '''
.......X.......
......X.X......
....XX.X.XX....
..XX.......XX..
.X...........X.
X...E.....E...X
X...E.....E...X
X......M......X
X.....M.M.....X
.X....MMM....X.
..XXXXXXXXXXX..
''', 'eating: mouth open')
spr('TN_CURL_EAT', '''
....XXX.........
...X...X........
....XX.X........
...XXXXXX.......
.XX......XX.....
X..........X....
X..E....E..X.XX.
X..E....E..XX..X
X..........X.X.X
X...M..M...X..X.
X...MMMM...X....
.X...MM...X.....
..XXXXXXXX......
''', 'eating: mouth open')

# ---------------------------------------------------------------------------------------------
# STAGE 0  (portrait page, box [0])
# ---------------------------------------------------------------------------------------------
spr('S0_IDLE', '''
.....XXXXX.....
...XX.....XX...
..X.........X..
.X...........X.
.X...E...E...X.
X....E...E....X
X.............X
X.............X
.X...........X.
..X.........X..
...XX.....XX...
.....XXXXX.....
''', 'Blobbert (Lil Blub): big and round, just eyes (no ears, mouth, tail or legs)')
spr('S0_SLEEP', '''
.....XXXXX.....
...XX.....XX...
..X.........X..
.X...........X.
.X...........X.
X...XX...XX...X
X.............X
X.............X
.X...........X.
..X.........X..
...XX.....XX...
.....XXXXX.....
''', 'Blobbert asleep: eyes shut (Z drawn separately)')

# ---------------------------------------------------------------------------------------------
# STAGE 1  (portrait page box [1]  +  the revised one with the "remove" notes)
# ---------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------
# STAGE 2  ("stage 2" on the sideways photo, and its bigger redraw on the 4th photo)
# ---------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------
# ADULTS (final evolution choice)
# ---------------------------------------------------------------------------------------------

# ---------------------------------------------------------------------------------------------
# NEW ART in the same style (2026-11-03): one more child, teen and adult, so every evolution is a choice
# ---------------------------------------------------------------------------------------------
spr('S0B_IDLE', '''
...X.....X...
..X.X...X.X..
.X...XXX...X.
.X.........X.
X..E.....E..X
X...........X
X.XX..M..XX.X
X....M.M....X
.X.........X.
..XXXXXXXXX..
''', 'Mochi (Lil Blub): round with cheek lines; 13 wide, between Cheeks and Blobbert')
spr('S1C_IDLE', '''
....X.....X.....
...X.X...X.X....
..X...XXX...X...
..X.........X...
.X..E.....E..X..
X.............X.
X......M......X.
XX....M.M....XX.
.XX.........XX..
..X.........X...
..X..XX.XX..X...
..X...XXX...X...
..X..XX.XX..X.X.
..X.........XX..
..X..X...X..X...
..XXXX...XXXX...
''', 'new: "Dapper", round with big cheeks and a bow tie (an adult look)')
spr('AD_LOAF', '''
...X.......X........
..X.X.....X.X.......
.X...XXXXX...X......
.X...........X......
X..E.......E..X.....
X......M......X.....
X.....M.M.....XXXXXX
.X................XX
X..................X
X..................X
.XXXXXXXXXXXXXXXXXX.
''', 'new: "Loaf" adult, a chonky loaf with a curled tail')

# Blob adults (2026-11-03): rounder, less fella-like finals
spr('AD_PUDDLE', '''
.....XXXXXXXX.......
...XX........XX.....
..X............X....
.X....E.....E...X...
.X....E.....E....X..
X........M.......XX.
X..................X
.X..XXX.......XX..X.
..XX...XXXXXXX..XX..
''', 'new: Puddle: melted flat')
spr('AD_JELLY', '''
.....XXXX.....
...XX....XX...
..X........X..
.X.XX.......X.
.X.X........X.
X.....E...E..X
X.....E...E..X
X............X
X.......M....X
X......M.M...X
X............X
X............X
.X.X.X..X.X.X.
X.X.X.XX.X.X.X
''', 'new: Jelly: wobbly gumdrop with a shine')
spr('TN_SPROUT', '''
..XXX.....XXX..
.X...X...X...X.
..XX..X.X..XX..
....XX.X.XX....
.......X.......
....XXXXXXX....
..XX.......XX..
.X...........X.
X...E.....E...X
X...E.....E...X
X......M......X
X.....M.M.....X
.X...........X.
..X.........X..
...XXXXXXXXX...
''', 'new: Sprout: a leaf grew on top')
spr('AD_DANGO', '''
.....XXX.....
...XX...XX...
..X.E...E.X..
..X.E...E.X..
..X...M...X..
...XX...XX...
..X..XXX..X..
.X.........X.
X...........X
X..XX...XX..X
X...........X
.X.........X.
..XXXXXXXXX..
''', 'new: Dango: two blobs, stacked')
spr('AD_PUFF', '''
.....XX.....XX.....
...XX..X...X..XX...
..X.....XXX.....X..
.X...............X.
X....E.......E....X
X....E.......E....X
X.................X
X........M........X
X.......M.M.......X
X.................X
.X...............X.
..XXXXXXXXXXXXXXX..
....X.X.....X.X....
''', 'new: Puff: cloud blob on tiny nubs')

spr('TN_SLOSH', '''
....XXXXXXX....
..XX.......XX..
.X...........X.
X...E.....E...X
X......M......X
X...G.......G.X
X.XX..XX..XX..X
XGGGGG.GGGGG.GX
XGG.GGGGGGGGGGX
.XGGGGGGG.GGGX.
..XXGGGGGGGXX..
....XXXXXXX....
''', 'new: Slosh: half-full liquid blob (Lil Blunkus, from Blobbert or Cheeks)')
spr('TN_WOBBLES', '''
...XXXXXXX...
..X.......X..
.X.........X.
..X..E.E..X..
.X.........X.
..X...M...X..
.X.........X.
..X.......X..
.X.........X.
..XXXXXXXXX..
''', 'new: Wobbles: wobbly-sided jelly (Lil Blunkus, from Mochi)')
spr('TN_MALLOW', '''
..XXXXXXXX..
.XGGGGGGGGX.
XGGGGGGGGGGX
X..........X
X.E......E.X
X.E......E.X
X..........X
X....M.....X
X..........X
.XXXXXXXXXX.
''', 'new: Mallow: toasted marshmallow (Lil Blunkus, from Mochi)')
spr('TN_TOASTY', '''
..XXXXXXXX..
.XGGGGGGGGX.
X..........X
.XX......XX.
..X.E..E.X..
..X..MM..X..
..X......X..
..X......X..
..XXXXXXXX..
''', 'new: Toasty: a slice of toast (Lil Blunkus, from Mochi)')
spr('AD_MARSHY', '''
..XXXXXXXXXXXXX..
.XGGG..GGGGG..GGX
XGG....GGGGGG...X
.XXXXXXXXXXXXXXX.
...X.........X...
...X..E...E..X...
...X....M....X...
...X.........X...
..XXXXXXXXXXXXX..
..X.X..X.X..X.X..
''', 'new: Marshy: a mossy mushroom')
spr('AD_BOOP', '''
.....XXX.....
....XGGGX....
....XGGGX....
.....XXX.....
.....X.X.....
.....X.X.....
..XXXX.XXXX..
.X.........X.
X..E.....E..X
X...........X
X.....M.....X
X...........X
.X.........X.
..XXXXXXXXX..
''', 'new: Boop: a blob with a tall button-stalk')
spr('AD_FUBZ', '''
....X.......X....
...X.X.....X.X...
...X..XXXXX..X...
..X...........X..
..X...E...E...X..
.X......X......X.
X...............X
X.XX.........XX.X
.X..XX.....XX..X.
.X......M......X.
..X...........X..
...XXXXXXXXXXX...
.....X.....X.....
''', 'new: Fubz: Cheeks all grown up')
spr('AD_NIMBUS', '''
.....XXXX........
...XXGGGGXXXX....
..XGGGGGGGGGGXX..
.XGGGGGGGGGGGGGX.
XGGG.E.GGG.E.GGGX
XGGGGGGGMGGGGGGGX
.XGGGGGGGGGGGGGX.
..XXGGGGGGGGGXX..
....XXXXXXXXX....
..G....G....G....
......G....G.....
..G....G....G....
''', 'new: Nimbus: a rain cloud')
spr('AD_BALLBOI', '''
......XXXXX......
....XX.....XX....
..XXG........XX..
.XXG..........XX.
.X.G...........X.
XX.............XX
X....XX...XX....X
X...X..XXX..X...X
X...X.......X...X
XX..X.E...E.X..XX
.X..X...M...X..X.
.XX..X.....X..XX.
..XX..XXXXX..XX..
....XX.....XX....
......XXXXX......
''', 'new: Ballboi: a hamster in a ball')

# open-mouth eating frames for the newer looks: the same sprite with the mouth dropped one dot lower
def eat_from(base):
    rows, _ = S[base]
    out = [list(r) for r in rows]
    for y, r in enumerate(rows[:-1]):
        for x, ch in enumerate(r):
            if ch == 'M' and rows[y + 1][x] == '.':
                out[y + 1][x] = 'M'
    spr(base + '_EAT', '\n'.join(''.join(r) for r in out), 'eating: mouth open')
for _n in ('TN_SLOSH', 'TN_WOBBLES', 'TN_MALLOW', 'TN_TOASTY', 'AD_MARSHY', 'AD_BOOP', 'AD_FUBZ', 'AD_NIMBUS', 'AD_BALLBOI'):
    if _n == 'AD_FUBZ':   # dropping his mouth a dot left a stray dot under it: his open mouth goes wide instead
        spr('AD_FUBZ_EAT', S['AD_FUBZ'][0][:9] and '\n'.join(S['AD_FUBZ'][0][:9] + ['.X.....MMM.....X.'] + S['AD_FUBZ'][0][10:]), 'eating: mouth open (wide)')
    else:
        eat_from(_n)

# ---------------------------------------------------------------------------------------------
# BIG BLORBUS: the last stage (a full TRAIN bar). Drawn in bigsprites.py.
# ---------------------------------------------------------------------------------------------
import bigsprites as _bs
def _dots(rows): return '\n'.join(r.replace(' ', '.') for r in rows)
spr('BB_BLOBBERT', _dots(_bs.big_blobbert()), 'Big Blobbert: just eyes. Squishes against the screen sides')
spr('BB_BALLBOI', _dots(_bs.ballboi_whole()), 'Big Ballboi: a bigger hamster in a bigger ball')
spr('BB_BALL_SHELL', _dots(_bs.ballboi_parts()[0]), 'Big Ballboi: just the ball')
spr('BB_BALL_HAM', _dots(_bs.ballboi_parts()[1]), 'Big Ballboi: just the hamster, who slides around inside')
spr('BB_ALIEN', _dots(_bs.alien_cat()), 'Alien cat in a tuxedo')

# ---------------------------------------------------------------------------------------------
# GAME-ONLY minis: Lil Blob and the Lil Blubs drawn smaller for Guess and Dodge, so the young fit between
# falling things more easily. Bigger looks play at full size (and have a harder time).
# ---------------------------------------------------------------------------------------------
spr('MINI_BLOB', '''
.X...X.
X.XXX.X
X.E.E.X
X.....X
.XXXXX.
''', 'game-only: small Lil Blob')
spr('MINI_ROUND', '''
..XXX..
.X...X.
X.E.E.X
X.....X
.X...X.
..XXX..
''', 'game-only: small Blobbert')
spr('MINI_MOCHI', '''
.X...X.
X.XXX.X
X.E.E.X
X..M..X
.XXXXX.
''', 'game-only: small Mochi')
spr('MINI_CHEEKS', '''
.X...X.
X.XXX.X
X.E.E.X
XX.M.XX
.X...X.
..XXX..
''', 'game-only: small Cheeks')
# dodge: more things to fall
spr('PEBBLE', '''
XX
XX
''', 'dodge: a small pebble')
spr('BOOK', '''
XXXXX
X...X
XXXXX
''', 'dodge: a wide book')

# dodge game: things that fall from the ceiling, and her lives
spr('DROP', '''
.X.
XXX
XXX
.X.
''')
spr('SOCK', '''
XX.
XX.
XXX
.XX
''')

# ---------------------------------------------------------------------------------------------
# EATING: mouth wide open (nose, the two cheek dots, and the jaw dropped under them). Chewing frames use the
# normal drawing with the eyes closed, so nothing flickers.
# ---------------------------------------------------------------------------------------------
spr('S1C_EAT', '''
....X.....X.....
...X.X...X.X....
..X...XXX...X...
..X.........X...
.X..E.....E..X..
X.............X.
X......M......X.
XX....M.M....XX.
.XX...MMM...XX..
..X.........X...
..X..XX.XX..X...
..X...XXX...X...
..X..XX.XX..X.X.
..X.........XX..
..X..X...X..X...
..XXXX...XXXX...
''', 'Dapper eating: mouth open')
spr('AD_LOAF_EAT', '''
...X.......X........
..X.X.....X.X.......
.X...XXXXX...X......
.X...........X......
X..E.......E..X.....
X......M......X.....
X.....M.M.....XXXXXX
.X....MMM.........XX
X..................X
X..................X
.XXXXXXXXXXXXXXXXXX.
''', 'eating: mouth open')
spr('AD_PUDDLE_EAT', '''
.....XXXXXXXX.......
...XX........XX.....
..X............X....
.X....E.....E...X...
.X....E.....E....X..
X.......M.M......XX.
X.......MMM........X
.X..XXX.......XX..X.
..XX...XXXXXXX..XX..
''', 'eating: mouth open')
spr('AD_JELLY_EAT', '''
.....XXXX.....
...XX....XX...
..X........X..
.X.XX.......X.
.X.X........X.
X.....E...E..X
X.....E...E..X
X............X
X.......M....X
X......M.M...X
X......MMM...X
X............X
.X.X.X..X.X.X.
X.X.X.XX.X.X.X
''', 'eating: mouth open')
spr('TN_SPROUT_EAT', '''
..XXX.....XXX..
.X...X...X...X.
..XX..X.X..XX..
....XX.X.XX....
.......X.......
....XXXXXXX....
..XX.......XX..
.X...........X.
X...E.....E...X
X...E.....E...X
X......M......X
X.....M.M.....X
.X....MMM....X.
..X.........X..
...XXXXXXXXX...
''', 'eating: mouth open')
spr('AD_DANGO_EAT', '''
.....XXX.....
...XX...XX...
..X.E...E.X..
..X.E...E.X..
..X..MMM..X..
...XX...XX...
..X..XXX..X..
.X.........X.
X...........X
X..XX...XX..X
X...........X
.X.........X.
..XXXXXXXXX..
''', 'eating: mouth open')
spr('AD_PUFF_EAT', '''
.....XX.....XX.....
...XX..X...X..XX...
..X.....XXX.....X..
.X...............X.
X....E.......E....X
X....E.......E....X
X.................X
X........M........X
X.......M.M.......X
X.......MMM.......X
.X...............X.
..XXXXXXXXXXXXXXX..
....X.X.....X.X....
''', 'eating: mouth open')

# ---------------------------------------------------------------------------------------------
# ITEMS (bubble and "tetris" poo are straight from the drawing)
# ---------------------------------------------------------------------------------------------
spr('BUBBLE', '''
.X.
X.X
.X.
''', 'a bubble they blow (the currency): tap it to pop it')
spr('POO', '''
.X.
XXX
''')
spr('STINK1', '''
X..
.X.
X..
''')
spr('STINK2', '''
.X.
X..
.X.
''')
spr('ZZ', '''
XXXX
..X.
.X..
XXXX
''', 'the Z from the sleeping drawings')
spr('ZBIG', '''
XXXXX
...X.
..X..
.X...
XXXXX
''')
spr('ZSMALL', '''
XXX
.X.
XXX
''')
spr('MEAL', '''
.XXXX...X
X....X.XX
X.X...X.X
X....X.XX
.XXXX...X
''')
spr('MEAL2', '''
.XXXX....
X....X...
X.X...X..
X....X...
.XXXX....
''')
spr('MEAL3', '''
.XX......
X..X.....
X.X......
X..X.....
.XX......
''')
spr('SNACK', '''
.XXX.
X.X.X
XX.XX
X.X.X
.XXX.
''')
spr('SNACK2', '''
.XXX.
X.X..
XX...
X.X..
.XXX.
''')
spr('SNACK3', '''
.X...
X....
X....
X....
.X...
''')
spr('HEART', '''
.XX.XX.
XXXXXXX
XXXXXXX
.XXXXX.
..XXX..
...X...
''')
spr('HEART_E', '''
.XX.XX.
X..X..X
X.....X
.X...X.
..X.X..
...X...
''')
spr('SKULL', '''
.XXXXX.
XXXXXXX
X..X..X
XXXXXXX
.XX.XX.
.X.X.X.
''')
# YEET: the sling and what's out there past the edge of the screen
spr('Y_SLING', '''
X...X
X...X
.X.X.
..X..
..X..
..X..
..X..
.XXX.
''', 'YEET: the slingshot (the band is drawn between the prongs)')
spr('Y_TRAMP', '''
XXXXXXX
.X...X.
X.....X
''', 'YEET: a trampoline (big bounce up)')
spr('Y_ROCK', '''
..XXX.
.X...X
X..X.X
XXXXXX
''', 'YEET: a rock (knocks you back)')
spr('Y_BIRD1', '''
X...X
.X.X.
..X..
''', 'YEET: a bird, wings up')
spr('Y_BIRD2', '''
.....
.XXX.
X.X.X
''', 'YEET: a bird, wings down')
spr('Y_CLOUD', '''
..XX....
.X..XX..
X.....X.
.XXXXXXX
''', 'YEET: a cloud (soft: slows you, or bounces you gently off the top)')
spr('Y_MUD', '''
.GGGGGG.
GGGGGGGG
''', 'YEET: mud (stops a roll dead)')
spr('Y_SPRING', '''
.XXX.
XX.XX
..X..
.X.X.
..X..
.XXX.
''', 'YEET: a spring mushroom (boing: up and on)')
spr('Y_BUMPER', '''
.XXX.
X.G.X
XGGGX
X.G.X
.XXX.
''', 'YEET: a bumper (pinball: bounces you off it, harder)')
spr('Y_POST', '''
X
X
X
''', 'YEET: a distance marker')
# What made them sick (shown beside the head, blinking; a number beside it when they've got more than one)
spr('SICK_BUG', '''
X.X.X.X
.XXXXX.
XX.X.XX
.XXXXX.
X.X.X.X
''', 'sick: the starter bug (a germ)')
spr('SICK_HUNGER', '''
..X.X..
...X...
..X.X..
XXXXXXX
.X...X.
..XXX..
''', 'sick: from going hungry (an empty bowl, crossed out)')
spr('SICK_LIGHT', '''
X.....X
..XXX..
.X...X.
.X...X.
..X.X..
..XXX..
''', 'sick: a night with the light on (a lit bulb)')
spr('SYRINGE', '''
....X.......
.XXXXXXXXXX.
XX........XX
.XXXXXXXXXX.
....X.......
''')
spr('FLOWER', '''
.X.X.
X.X.X
.X.X.
..X..
..X..
''', 'a found treasure: a flower')
spr('STONE', '''
.XX.
X.XX
.XX.
''', 'a found treasure: a nice pebble')
spr('BERRY', '''
...X.
..X..
.XXX.
XX.XX
.XXX.
''', 'a found treasure: a berry')
spr('GEM', '''
.XXX.
X.X.X
.X.X.
..X..
''', 'a found treasure: a gem (rare)')
spr('SHELL', '''
..X..
.XXX.
X.X.X
XX.XX
.XXX.
''', 'a found treasure: a shell')
spr('ANGRY', '''
X.X
.X.
X.X
''')
spr('SPARK', '''
.X.
XXX
.X.
''')
spr('TRAIL', '''
.G.
GGG
''', 'out exploring: their goo trail')
spr('ARROW_R', '''
X..
XX.
XXX
XX.
X..
''')
spr('BALL', '''
.XX.
XXXX
XXXX
.XX.
''')

# ---------------------------------------------------------------------------------------------
# ICONS (16x16 logical px, LCD "segment" style: faint when off, black when on)
# ---------------------------------------------------------------------------------------------
spr('IC_FEED', '''
................
.....XXXXX....X.
...XX.....X..XX.
..X..X.....XX.X.
..X.........X.X.
...XX.....XX.XX.
.....XXXXX....X.
................
XXXXXXXXXXXXXXXX
X..............X
.X............X.
..X..........X..
...XXXXXXXXXX...
................
................
................
''')
spr('IC_LIGHT', '''
......XXXX......
....XX....XX....
...X........X...
..X..........X..
..X...X..X...X..
..X....XX....X..
...X...XX...X...
....X..XX..X....
.....X.XX.X.....
.....XXXXXX.....
.....X....X.....
.....XXXXXX.....
.....X....X.....
......XXXX......
................
................
''')
spr('IC_PLAY', '''
......XXXX......
....XX.X..XX....
...X..X..X..X...
..X..X..X..X.X..
..X.X..X..X..X..
.X.X..X..X..X.X.
.XX..X..X..X..X.
.X..X..X..X..XX.
.X.X..X..X..X.X.
..X..X..X..X.X..
..X.X..X..X..X..
...X..X..X..X...
....XX..X.XX....
......XXXX.X....
............X...
.............XX.
''')
spr('IC_MEDS', '''
................
.....XXXXXX.....
.....X....X.....
.....X....X.....
.....X....X.....
.XXXXX....XXXXX.
.X............X.
.X............X.
.X............X.
.X............X.
.XXXXX....XXXXX.
.....X....X.....
.....X....X.....
.....X....X.....
.....XXXXXX.....
................
''')
spr('IC_LITTER', '''
...XXX..........
..X...X...XXX...
.X.....X.X...X..
.X..X..X.X...X..
.X.....X..XXX...
..X...X.........
...XXX......XX..
...........X..X.
............XX..
.XXXXXXXXXXXXXX.
X..............X
X..X.....X.....X
X..............X
.XXXXXXXXXXXXXX.
................
................
''')
spr('IC_METER', '''
.......XX.......
.......XX.......
.XXXXXXXXXXXXXX.
.X......X.....X.
X.X.....X....X.X
X.X.....X....X.X
X..X....X...X..X
...X....X...X...
XXXXX...X..XXXXX
.XXX....X...XXX.
........X.......
........X.......
......XXXX......
....XXXXXXXX....
................
................
''')
spr('IC_LOCK', '''
................
................
.....XXXXXX.....
....XX....XX....
....X......X....
....X......X....
..XXXXXXXXXXXX..
..X..........X..
..X....XX....X..
..X....XX....X..
..X.....X....X..
..X..........X..
..XXXXXXXXXXXX..
................
................
................
''', 'LOCK, locked: a closed padlock (they stay as they are and can\'t die)')
spr('IC_UNLOCK', '''
................
.........XXXXXX.
........XX....XX
........X......X
........X......X
........X.......
..XXXXXXXXXXXX..
..X..........X..
..X....XX....X..
..X....XX....X..
..X.....X....X..
..X..........X..
..XXXXXXXXXXXX..
................
................
................
''', 'LOCK, unlocked: an open padlock')
spr('IC_CALL', '''
................
.....XXXX.....X.
...XX....XX...X.
..X........X..X.
.X..X....X..X.X.
.X..........X.X.
.X...X..X...X...
..X...XX...X..X.
...XXXXXXXX.....
................
................
................
................
................
................
................
''', 'attention: a round blob (no ears) and a !')


def render_sheet(path):
    from PIL import Image, ImageDraw
    names = list(S.keys())
    cell = 6
    pad = 10
    cols = 6
    tiles = []
    for n in names:
        rows, _ = S[n]
        tiles.append((n, rows))
    colw = max(len(r[0]) for _, r in tiles) * cell + pad * 2
    rowh = max(len(r) for _, r in tiles) * cell + pad * 2 + 12
    nrows = (len(tiles) + cols - 1) // cols
    im = Image.new('RGB', (cols * colw, nrows * rowh), (201, 204, 193))
    d = ImageDraw.Draw(im)
    for i, (n, rows) in enumerate(tiles):
        ox = (i % cols) * colw + pad; oy = (i // cols) * rowh + pad
        d.text((ox, oy - 8), n, fill=(60, 60, 60))
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                c = {'X': (34, 37, 31), 'E': (34, 37, 31), 'M': (34, 37, 31), 'G': (112, 116, 106)}.get(ch, (182, 186, 172))
                d.rectangle([ox + x * cell, oy + 4 + y * cell, ox + x * cell + cell - 2, oy + 4 + y * cell + cell - 2], fill=c)
    im.save(path)


def emit_cpp(outdir):
    names = list(S.keys())
    h = ['// Generated by art/sprites.py from the graph-paper drawings. Do not edit by hand.',
         '#pragma once', '#include <stdint.h>', '', 'namespace lf {', '',
         '// One sprite: w x h dots, 1 byte per dot: 0 empty, 1 black, 2 eye, 3 mouth, 4 mid grey.',
         'struct Sprite { uint8_t w, h; const uint8_t* px; };', '',
         'enum SpriteId : uint8_t {']
    for n in names: h.append('  SPR_%s,' % n)
    h += ['  SPR_COUNT', '};', '', 'extern const Sprite SPRITES[SPR_COUNT];', '', '}  // namespace lf', '']
    c = ['// Generated by art/sprites.py from the graph-paper drawings. Do not edit by hand.',
         '#include "sprites.h"', '', 'namespace lf {', '']
    m = {'.': 0, 'X': 1, 'E': 2, 'M': 3, 'G': 4}
    for n in names:
        rows, src = S[n]
        if src: c.append('// %s' % src)
        c.append('static const uint8_t px_%s[] = {' % n)
        for r in rows:
            c.append('  ' + ','.join(str(m[ch]) for ch in r) + ',  // ' + r)
        c.append('};')
    c.append('')
    c.append('const Sprite SPRITES[SPR_COUNT] = {')
    for n in names:
        rows, _ = S[n]
        c.append('  {%d, %d, px_%s},' % (len(rows[0]), len(rows), n))
    c += ['};', '', '}  // namespace lf', '']
    open(outdir + '/sprites.h', 'w').write('\n'.join(h))
    open(outdir + '/sprites_data.cpp', 'w').write('\n'.join(c))


if __name__ == '__main__':
    if len(sys.argv) > 2 and sys.argv[1] == '--cpp':
        emit_cpp(sys.argv[2])
    else:
        render_sheet(sys.argv[1] if len(sys.argv) > 1 else 'sheet.png')
