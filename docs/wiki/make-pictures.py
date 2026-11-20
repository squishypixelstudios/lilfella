# Turns the simulator's screenshots into the wiki's pictures.
#
#   cd sim && make && mkdir shots && ./sim wikishots shots && ./sim popshots shots
#   python3 ../docs/wiki/make-pictures.py shots ../docs/wiki/img
#
# Each .ppm (the whole 368x448 screen) becomes a .png named without its number. Needs Pillow.
import glob, os, re, sys
from PIL import Image

src, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
n = 0
for f in sorted(glob.glob(os.path.join(src, '*.ppm'))):
    name = re.sub(r'^\d+_', '', os.path.basename(f)[:-4])
    im = Image.open(f).convert('RGB')
    im.save(os.path.join(out, name + '.png'), optimize=True)
    # the dot-matrix LCD on its own (with its frame), for the pages that only need that
    # (POP has the whole screen and a taller LCD, so its pictures are cut taller)
    im.crop((16, 52, 352, 412) if name.startswith('pop_') else (16, 104, 352, 344)).save(os.path.join(out, name + '-lcd.png'), optimize=True)
    n += 1
print(n, 'pictures in', out)
