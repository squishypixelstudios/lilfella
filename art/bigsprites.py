# Big Blorbus (stage 5) pictures, built from shapes. Each function returns rows: X ink, E eye, M mouth, G grey, . empty.
def grid(w,h): return [['.']*w for _ in range(h)]
def ellipse(g,cx,cy,rx,ry,ch='X'):
    # outline: cells inside ellipse whose 4-neighbours include an outside cell
    h=len(g); w=len(g[0])
    def inside(x,y): return ((x-cx)/rx)**2+((y-cy)/ry)**2<=1.0
    for y in range(h):
        for x in range(w):
            if inside(x,y) and not all(0<=x+dx<w and 0<=y+dy<h and inside(x+dx,y+dy) for dx,dy in((1,0),(-1,0),(0,1),(0,-1))):
                g[y][x]=ch
def put(g,x,y,rows):
    for j,r in enumerate(rows):
        for i,c in enumerate(r):
            if c!=' ' and 0<=y+j<len(g) and 0<=x+i<len(g[0]): g[y+j][x+i]=c
def out(g): return [''.join(r) for r in g]

# 1) Giant Blobbert: 36x25, just eyes (taller eyes, like Blobbert), a soft shine
def big_blobbert():
    g=grid(36,25); ellipse(g,17.5,12,17.5,12)
    put(g,10,10,['E','E','E'])   # eyes 1 wide x3 tall (Blobbert: 1x2 scaled)
    put(g,25,10,['E','E','E'])
    put(g,6,5,['GG','G.'])       # shine
    return out(g)

# 2) Giant Ballboi: 26x26 ball, big hamster inside
def big_ballboi():
    g=grid(27,27); ellipse(g,13,13,13,13)
    ham=[
 "  XX      XX  ",
 " X..X    X..X ",
 " X..XXXXXX..X ",
 "  XX......XX  ",
 " X..E....E..X ",
 "X...........X ",
 "X.GG..MM..GGX ",
 "X.GG......GGX ",
 " X..........X",
 " X..........X",
 "  XX......XX ",
 "  X.X....X.X ",
]
    ham=[r.replace('.',' ') if False else r for r in ham]
    put(g,6,8,[r.replace('.',' ') for r in ham])
    put(g,5,5,['G','G'])
    put(g,4,7,['G','G'])
    put(g,16,4,['GG','G.'])   # glint
    # ball highlight arc
    for x in range(5,10): g[3+ (0 if x>6 else 1)][x] = 'G' if g[3][x]=='.' else g[3][x]
    return out(g)

# 3) Alien cat in a tuxedo: 17x26
def alien_cat():
    return [r for r in """
.X...........X..
.X...........X..
..X.........X...
..X.........X...
.XXX.......XXX..
.X.XXXXXXXXX.X..
.X...........X..
X..............X
X.XXXX...XXXX..X
X.XEEEX.XEEEXX.X
X..XXX...XXX...X
.X.....M.......X
..X...XXX....X..
...XXX...XXXX...
....X..G..X.....
...XXXXXXXXXX...
..X.X.GGG.X.X...
.X..XXGGGXX..X..
.X..X.X.X.X..X..
.XX.X.XXX.X.XX..
..XXXXXXXXXXX...
..X...X.X...X...
..XX..X.X..XX...
""".strip('\n').split('\n')]

def ell(g,x0,y0,w,h,ch='X',fill=False):
    cx=x0+w/2; cy=y0+h/2
    def inside(x,y): return ((x+.5-cx)/(w/2))**2+((y+.5-cy)/(h/2))**2<=1.0
    W=len(g[0]); H=len(g)
    for y in range(y0,y0+h):
        for x in range(x0,x0+w):
            if not inside(x,y): continue
            edge=not all(inside(x+dx,y+dy) for dx,dy in((1,0),(-1,0),(0,1),(0,-1)))
            if edge or fill: g[y][x]=ch
def big_blobbert():
    g=grid(36,25); ell(g,0,0,36,25)
    put(g,11,10,['E','E','E']); put(g,24,10,['E','E','E'])
    put(g,5,5,['GG','G ']); return out(g)
def big_ballboi():
    g=grid(27,27); ell(g,0,0,27,27)
    ell(g,5,6,5,5); ell(g,17,6,5,5)                       # ears
    for y,x in((8,7),(8,19)): g[y][x]='G'                  # inner ear
    ell(g,5,8,17,15)                                         # body/head
    g[13][10]='E'; g[14][10]='E'; g[13][16]='E'; g[14][16]='E'
    for y in range(15,19):
        for x in (7,8,9,17,18,19): g[y][x]='G'              # stuffed cheeks
    g[16][12]='M'; g[16][13]='M'; g[16][14]='M'; g[17][13]='M'  # nose and mouth
    for x in (9,10,16,17): g[22][x]='X'                      # feet
    put(g,4,3,['GGG','G  ']); put(g,20,3,[' G','G ']) if False else None
    return out(g)
def alien_cat():
    g=grid(19,26)
    # antennae
    for y in range(0,4): g[y][6]='X'; g[y][12]='X'
    g[0][6]='G'; g[0][12]='G'; g[0][5]='G'; g[0][7]='G'; g[0][11]='G'; g[0][13]='G'
    # ears
    for (x,y) in((2,3),(1,4),(2,4),(2,5),(1,5),(3,5)): pass
    ears=[" X  "," XX ","X X ","X XX","X  X"]
    # head
    ell(g,1,5,17,11)
    # ears over head top
    for y,xs in enumerate([(2,),(2,3),(2,4),(3,5)]): 
        for x in xs: g[3+y][x]='X'; g[3+y][18-x]='X'
    g[3][2]='X'
    # big slanted alien eyes (filled, E)
    for i,(x0,y0) in enumerate(((4,8),(12,8))):
        for r,(a,b) in enumerate([(0,3),(0,4),(1,4)] if i==0 else [(0,3),(-1,3),(-1,2)]):
            for x in range(x0+a,x0+b): g[y0+r][x]='E'
    g[12][9]='M'; g[13][8]='M'; g[13][10]='M'   # tiny w mouth
    # body: black tux, white chest
    for y in range(16,24):
        for x in range(4,15): g[y][x]='X'
    for y in range(16,24):
        w=max(0,4-(y-16)//2)
        for x in range(9-w,10+w):
            if y>17: g[y][x]='.'
    g[16][8]='X'; g[16][10]='X'
    for x in (8,9,10): g[17][x]='X'                         # bow tie
    g[18][7]='X'; g[18][11]='X'; g[19][9]='G'; g[21][9]='G'
    # arms + feet
    for y in range(18,23): g[y][3]='X'; g[y][15]='X'
    for x in (5,6,7,11,12,13): g[24][x]='X'
    for x in (4,5,6,7,11,12,13,14): g[25][x]='X'
    return out(g)
def alien_cat():
    W=17; g=grid(W,25)
    def sym(x,y,ch):
        g[y][x]=ch; g[y][W-1-x]=ch
    # antennae with glowing tips
    for y in (1,2,3,4): sym(6,y,'X')
    for x in (5,6,7): g[0][x]=g[0][W-1-x]='G'
    sym(6,0,'G')
    # head
    ell(g,1,5,15,11)
    # pointy cat ears rising from the head's top corners
    for y,xs in enumerate([(1,),(1,2),(1,2,3),(1,4)]):
        pass
    for (x,y) in((2,2),(2,3),(3,3),(2,4),(4,4),(2,5),(3,5),(4,5),(3,6)): sym(x,y,'X')

    # big slanted alien eyes
    for (x,y) in((4,8),(5,8),(3,9),(4,9),(5,9),(6,9),(3,10),(4,10),(5,10),(6,10),(4,11),(5,11)): sym(x,y,'E')
    sym(4,9,'.')   # a sparkle in each big round eye
    g[13][8]='M'; g[14][7]='M'; g[14][9]='M'; g[13][6]='.'   # small w mouth
    # tuxedo: black jacket, white shirt in a V, bow tie
    for y in range(16,23):
        for x in range(2,15): g[y][x]='X'
    for y,(a,b) in zip(range(16,22),[(6,10),(6,10),(6,10),(7,9),(7,9),(8,8)]):
        for x in range(a,b+1): g[y][x]='.'
    for x in (6,7,9,10): g[16][x]='X'
    g[16][8]='G'
    for x in (3,4,5,6,10,11,12,13): g[23][x]='X'
    for x in (2,3,4,5,6,10,11,12,13,14): g[24][x]='X'
    return out(g)


# Big Ballboi in two layers so the hamster can slide around inside the ball: the shell (27x27) and the hamster (17x17,
# which sits at (5, 6) in the shell when the ball is still).
HAM_AT = (5, 6)
def ballboi_parts():
    sh = grid(27, 27); ell(sh, 0, 0, 27, 27); put(sh, 4, 3, ['GGG', 'G  '])
    h = grid(17, 17)
    ell(h, 0, 0, 5, 5); ell(h, 12, 0, 5, 5)                  # ears
    h[2][2] = 'G'; h[2][14] = 'G'
    ell(h, 0, 2, 17, 15)                                       # body
    for (x, y) in ((5, 7), (5, 8), (11, 7), (11, 8)): h[y][x] = 'E'
    for y in range(9, 13):
        for x in (2, 3, 4, 12, 13, 14): h[y][x] = 'G'          # stuffed cheeks
    for x in (7, 8, 9): h[10][x] = 'M'
    h[11][8] = 'M'
    for x in (4, 5, 11, 12): h[16][x] = 'X'                   # feet
    return out(sh), out(h)
def ballboi_whole():
    sh, h = ballboi_parts(); g = [list(r) for r in sh]; ax, ay = HAM_AT
    for y, r in enumerate(h):
        for x, c in enumerate(r):
            if c != '.' and c != ' ': g[ay + y][ax + x] = c
    return out(g)
