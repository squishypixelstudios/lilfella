// ===== Lil Fella eating lab: a 40x28 dot-matrix engine + one eating routine per character =====
const W = 40, H = 28;
const buf = new Uint8Array(W * H);
let invert = false;
const clr = () => { buf.fill(0); invert = false; };
const set = (x, y, v) => { x = Math.round(x); y = Math.round(y); if (x >= 0 && x < W && y >= 0 && y < H) buf[y * W + x] = v; };
const cl = v => Math.max(0, Math.min(1, v));
const pr = (t, a, b) => cl((t - a) / (b - a));
const ez = k => k * k * (3 - 2 * k);
const lerp = (a, b, k) => a + (b - a) * k;
const rnd = i => { const s = Math.sin(i * 12.9898 + 4.1) * 43758.5453; return s - Math.floor(s); };
const sw = n => SP[n][0].length, sh = n => SP[n].length;

// draw a sprite. o: flip, shut (eyes hidden = closed), mid (draw everything grey), dx(row), dy(col), rmin/rmax (row range)
function put(n, x, y, o) {
  o = o || {};
  const R = SP[n], w = R[0].length;
  for (let r = 0; r < R.length; r++) {
    if (o.rmin != null && (r < o.rmin || r > o.rmax)) continue;
    for (let c = 0; c < w; c++) {
      if (o.cmin != null && (c < o.cmin || c > o.cmax)) continue;
      const ch = R[r][o.flip ? w - 1 - c : c];
      if (o.noG && ch === 'G') continue;
      if (o.noE && ch === 'E') continue;
      if (o.onlyE && ch !== 'E') continue;
      let v = ch === 'X' || ch === 'M' ? 3 : ch === 'E' ? (o.shut ? 0 : 3) : ch === 'G' ? 2 : 0;
      if (!v) continue;
      if (o.mid && v === 3) v = 2;
      if (o.dark && v === 2) v = 3;
      set(x + c + (o.dx ? o.dx(r) : 0), y + r + (o.dy ? o.dy(c) : 0), v);
    }
  }
}
// the same, stretched or squashed to w x h dots (end points map to end points)
function warp(n, x, y, w, h, o) {
  o = o || {};
  w = Math.max(1, Math.round(w)); h = Math.max(1, Math.round(h));
  const R = SP[n], sw_ = R[0].length, sh_ = R.length;
  for (let dy = 0; dy < h; dy++) for (let dx = 0; dx < w; dx++) {
    const mx = o.flip ? w - 1 - dx : dx;
    const sx = w > 1 ? Math.round(mx * (sw_ - 1) / (w - 1)) : 0, sy = h > 1 ? Math.round(dy * (sh_ - 1) / (h - 1)) : 0;
    const ch = R[sy][sx];
    let v = ch === 'X' || ch === 'M' ? 3 : ch === 'E' ? (o.shut ? 0 : 3) : ch === 'G' ? 2 : 0;
    if (!v) continue;
    if (o.mid && v === 3) v = 2;
    set(x + dx + (o.dx ? o.dx(dy) : 0), y + dy, v);
  }
}
const FOODK = { kind: 'meal' };
const FD = () => FOODK.kind === 'meal' ? { n: ['MEAL', 'MEAL2', 'MEAL3'], w: 9, h: 5 } : { n: ['SNACK', 'SNACK2', 'SNACK3'], w: 5, h: 5 };
// food centred on cx, top at y, stage 0..2 (3 = gone)
function food(cx, y, stage, o) { const f = FD(); if (stage >= 3) return; put(f.n[stage], Math.round(cx - f.w / 2), y, o); }
const dropY = (t, t0, d, y) => { const k = pr(t, t0, t0 + d); return Math.round(y - (1 - k) * (1 - k) * 22); };
const spark = (x, y) => put('SPARK', x, y);
const bubble = (x, y) => put('BUBBLE', x, y);
const line = (x0, y0, x1, y1, v) => { const n = Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0), 1); for (let i = 0; i <= n; i++) set(x0 + (x1 - x0) * i / n, y0 + (y1 - y0) * i / n, v); };
const disc = (cx, cy, r, v) => { for (let y = -r; y <= r; y++) for (let x = -r; x <= r; x++) if (x * x + y * y <= r * r + r * 0.5) set(cx + x, cy + y, v); };
const rect = (x, y, w, h, v) => { for (let j = 0; j < h; j++) for (let i = 0; i < w; i++) set(x + i, y + j, v); };
const hop = (k, n, amp) => Math.round(Math.abs(Math.sin(k * n * Math.PI)) * amp);

// ---------------------------------------------------------------------------------------------
// WOBBLES (Lil Blunkus): a slinky. Stretches up, arches over (an upside-down U), clamps the food with the top
// of his head; it travels down the whole U and dissolves in his feet. Then he boings back.
// ---------------------------------------------------------------------------------------------
function mkPath(x0, y0, A1, R, al, A2) {
  const step = 0.25, pts = []; let x = x0, y = y0; const Lt = A1 + R * al + A2;
  for (let s = 0; s <= Lt + 0.001; s += step) {
    let hx, hy;
    if (s < A1) { hx = 0; hy = -1; }
    else if (s < A1 + R * al) { const p = (s - A1) / R; hx = Math.sin(p); hy = -Math.cos(p); }
    else { hx = Math.sin(al); hy = -Math.cos(al); }
    pts.push({ x, y, hx, hy }); x += hx * step; y += hy * step;
  }
  return { Lt, at: a => pts[Math.min(pts.length - 1, Math.max(0, Math.round(a / step)))] };
}
function wobbles(t) {
  const R = 8.5, B = 27, X0 = 11, fcx = X0 + 2 * R;
  let lam = 0, Ls = 10;
  if (t < 0.7) Ls = 10;
  else if (t < 1.0) Ls = lerp(10, 8, ez(pr(t, 0.7, 1)));
  else if (t < 1.8) Ls = lerp(8, 25, ez(pr(t, 1, 1.8)));
  else if (t < 3.0) { Ls = 25; lam = ez(pr(t, 1.8, 3.0)); }
  else if (t < 5.6) { Ls = 25; lam = 1; }
  else if (t < 6.2) { Ls = 25; lam = 1 - ez(pr(t, 5.6, 6.2)); }
  else { const u = t - 6.2; Ls = 10 + 15 * Math.exp(-4.5 * u) * Math.cos(9 * u); }
  const open = (t > 2.7 && t < 3.5) || (t > 3.4 && t < 5.0 && Math.floor(t * 6) % 2 === 0);
  const shut = t > 3.0 && t < 6.4;
  if (t < 3.4) food(Math.round(fcx), dropY(t, 0, 0.45, 23), 0);
  const A1 = lam > 0 ? lerp(25, 11, ez(cl(lam * 1.6))) : Ls, al = lam * Math.PI, A2 = lam * 11;
  if (lam === 0 && Ls < 10.5) { const h = Math.max(6, Math.round(Ls)); warp(open ? 'TN_WOBBLES_EAT' : 'TN_WOBBLES', 5, 28 - h, 13, h, { shut }); }
  else {
    const P = mkPath(X0, B, A1, R, al, A2), n = Math.round(P.Lt);
    const idle = SP.TN_WOBBLES, hd = SP[open ? 'TN_WOBBLES_EAT' : 'TN_WOBBLES'];
    const lines = [idle[9], idle[8]]; const m = n - 8;
    if (m <= 2) { if (m >= 2) lines.push(idle[7]); if (m >= 1) lines.push(idle[6]); }
    else for (let i = 0; i < m; i++) lines.push(i % 2 ? '.............' : '.XXXXXXXXXXX.');
    for (let r = 5; r >= 0; r--) lines.push(hd[r]);
    const curved = al > 0.05;
    for (let k = 0; k < lines.length; k++) {
      const p = P.at((k + 0.5) * P.Lt / lines.length), nx = -p.hy, ny = p.hx;
      for (let c = 0; c < 13; c++) {
        const ch = lines[k][c]; if (ch === '.') continue;
        if (ch === 'E' && shut) continue;
        const v = ch === 'G' ? 2 : 3;
        set(p.x + nx * (c - 6), p.y + ny * (c - 6), v);
        if (curved && c < 12 && lines[k][c + 1] !== '.') set(p.x + nx * (c - 5.5), p.y + ny * (c - 5.5), v);   // fill the gaps on the outside of a bend
      }
    }
    if (t >= 3.4 && t < 5.0) {   // the food travels the whole way round
      const f = ez(pr(t, 3.4, 5.0)), p = P.at(lerp(P.Lt - 3.5, 2.5, f));
      for (const [dx, dy] of [[0, 0], [1, 0], [-1, 0], [0, 1], [0, -1]]) set(p.x + dx, p.y + dy, dx || dy ? 2 : 3);
    }
  }
  if (t >= 5.0 && t < 5.7) {   // it dissolves in his feet
    for (let i = 0; i < 7; i++) { const life = pr(t, 5.0 + i * 0.05, 5.7); if (life < 1) set(X0 - 4 + rnd(i) * 8, 26 - life * 6 * (0.5 + rnd(i + 9)), life < 0.5 ? 3 : 2); }
  }
  if (t > 6.9 && Math.floor(t * 5) % 2 === 0) spark(17, 16);
}

// ---------------------------------------------------------------------------------------------
// JELLY: hops over, belly-flops on the food, springs back up with it inside, jiggling; it floats up through
// the see-through body and fizzles away; a bubble burp.
// ---------------------------------------------------------------------------------------------
function jelly(t) {
  const fcx = 28;
  if (t < 2.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  let left = 3, w = 14, h = 14, lift = 0;
  if (t < 0.4) { left = 3; }
  else if (t < 1.5) { const k = pr(t, 0.4, 1.5); left = 3 + Math.round(k * 11); lift = hop(k, 3, 3); }
  else if (t < 2.2) { const k = pr(t, 1.5, 2.2); left = 14 + Math.round(ez(k) * 7); lift = Math.round(Math.sin(k * Math.PI) * 9); }
  else if (t < 2.6) { const k = ez(pr(t, 2.2, 2.6)); w = 14 + Math.round(10 * k); h = 14 - Math.round(10 * k); left = 28 - Math.round(w / 2); }
  else { left = 21; const k = pr(t, 2.6, 3.4); const e = 1 - Math.pow(1 - k, 2) * Math.cos(k * 9); w = Math.round(lerp(24, 14, cl(k)) ); h = Math.round(lerp(4, 14, e)); left = 28 - Math.round(w / 2); }
  const inside = t >= 2.6 && t < 5.1, shut = inside;
  const amp = t >= 2.6 && t < 5.8 ? 2.2 * Math.exp(-(t - 2.6) * 0.7) : 0;
  const y = 28 - h - lift;
  warp(t > 5.2 && t < 5.5 ? 'AD_JELLY_EAT' : 'AD_JELLY', left, y, w, h, { shut, dx: r => Math.round(Math.sin(t * 14 + r * 0.8) * amp) });
  if (inside && h > 9) {   // the food inside the see-through belly, drifting up and fading
    const k = pr(t, 3.3, 5.0), fy = Math.round(lerp(28 - 14 + 7, 28 - 14 + 2, ez(k))), st = k < 0.35 ? 0 : k < 0.7 ? 1 : 2;
    if (k < 1) food(28, fy + (14 - h), st, { mid: true });
  }
  if (t > 5.0 && t < 5.9) { const k = pr(t, 5.0, 5.9); bubble(Math.round(30 + Math.sin(k * 6) * 2), Math.round(10 - k * 8)); if (k > 0.85) spark(30, 2); }
  if (t > 6.0 && Math.floor(t * 4) % 2 === 0) put('SPARK', 38, 9);
}

// ---------------------------------------------------------------------------------------------
// PUDDLE: flows out flat across the food (you can see it inside him), glugs it down in three gulps with
// bubbles, bunches up into a tall dome, burps, and slumps back.
// ---------------------------------------------------------------------------------------------
function puddle(t) {
  const fcx = 29;
  const stage = t < 2.0 ? 0 : t < 2.6 ? 1 : t < 3.2 ? 2 : 3;
  if (t < 0.45) food(fcx, dropY(t, 0, 0.45, 23), 0);
  let left = 2, w = 20, h = 9, shut = false, open = false;
  if (t < 0.5) { /* idle */ }
  else if (t < 1.7) { const k = ez(pr(t, 0.5, 1.7)), k2 = ez(pr(t, 0.9, 1.7)); const right = lerp(22, 38, k); left = Math.round(lerp(2, 14, k2)); w = Math.round(right - left); h = Math.round(lerp(9, 4, k)); }
  else if (t < 3.6) { left = 14; w = 24; h = 4 + Math.round(2 * Math.max(0, Math.sin((t - 2.0) * 10.5) * (t > 2.0 && t < 3.3 ? 1 : 0))); shut = true; }
  else if (t < 4.5) { const k = ez(pr(t, 3.6, 4.5)); w = Math.round(lerp(24, 14, k)); h = Math.round(lerp(4, 12, k)); left = Math.round(lerp(14, 20, k)); shut = true; }
  else { const k = pr(t, 4.5, 5.6); const e = Math.exp(-3 * k) * Math.cos(k * 14); w = Math.round(20 - 6 * e); h = Math.round(9 + 3 * e); left = 20 - Math.round((w - 14) / 3); shut = k < 0.6; }
  if (t >= 1.5 && t < 3.6) { food(fcx, 23, stage, { mid: true }); }
  warp(open ? 'AD_PUDDLE_EAT' : 'AD_PUDDLE', left, 28 - h, w, h, { shut });
  if (t >= 1.7 && t < 3.7) for (let i = 0; i < 3; i++) { const k = ((t * 0.9 + i * 0.37) % 1); bubble(Math.round(18 + i * 7 + Math.sin(t * 5 + i) * 1.5), Math.round(22 - k * 9)); }
  if (t > 4.4 && t < 5.0) { const k = pr(t, 4.4, 5.0); bubble(24, Math.round(14 - k * 8)); }
  if (t > 5.6 && Math.floor(t * 5) % 2 === 0) spark(32, 14);
}

// ---------------------------------------------------------------------------------------------
// TOASTY: walks to the food, pops his top crust up like a toaster ("ding!"), the food slides in, the crust slams
// shut: a fish sandwich. He chews with a bounce and crumbs fly. Crust lifts, burp, all gone.
// ---------------------------------------------------------------------------------------------
function toasty(t) {
  // The crust (with his eyes!) lifts off like a toaster; the fish hops up off the floor, slides into the middle
  // and the crust comes down on it: a sandwich. His mouth stays on the bottom slice and does the chewing.
  const y0 = 19, mid = 20;   // mid: x of the middle of his body once he's parked
  let x = 3, bob = 0, lid = 0, fx = 28, fy = 23, fstage = 0, fshow = true;
  if (t < 0.4) fy = dropY(t, 0, 0.4, 23);
  if (t >= 0.4 && t < 1.5) { const k = pr(t, 0.4, 1.5); x = 3 + Math.round(k * 11); bob = hop(k, 5, 1); }
  else if (t >= 1.5) x = 14;
  // crust: pops up (DING), waits, comes down onto the fish, then sinks as he eats it
  if (t >= 1.5 && t < 1.85) lid = Math.round(ez(pr(t, 1.5, 1.85)) * 9);
  else if (t >= 1.85 && t < 3.0) lid = 9;
  else if (t >= 3.0 && t < 3.3) lid = Math.round(lerp(9, 5, ez(pr(t, 3.0, 3.3))));
  else if (t >= 3.3 && t < 3.7) lid = 5;
  else if (t >= 3.7 && t < 4.3) lid = Math.round(lerp(5, 3, ez(pr(t, 3.7, 3.9))));
  else if (t >= 4.3 && t < 4.9) lid = Math.round(lerp(3, 1, ez(pr(t, 4.3, 4.5))));
  else if (t >= 4.9 && t < 5.3) lid = Math.round(lerp(1, 0, ez(pr(t, 4.9, 5.1))));
  else if (t >= 5.3 && t < 5.9) lid = Math.round(Math.sin(pr(t, 5.3, 5.9) * Math.PI) * 4);
  // the fish: hops up off the floor, slides into the gap, rests on the bottom slice
  if (t >= 1.9 && t < 2.4) { const k = ez(pr(t, 1.9, 2.4)); fy = Math.round(lerp(23, 18, k)) - Math.round(Math.sin(k * Math.PI) * 3); }
  else if (t >= 2.4 && t < 3.0) { fy = 18; fx = Math.round(lerp(28, mid, ez(pr(t, 2.4, 3.0)))); }
  else if (t >= 3.0) { fy = 18; fx = mid; }
  fstage = t < 3.7 ? 0 : t < 4.3 ? 1 : t < 4.9 ? 2 : 3;
  const chew = t >= 3.3 && t < 5.1 ? Math.abs(Math.sin((t - 3.3) * 6.5)) > 0.55 : false;
  const open = (t >= 1.5 && t < 3.0 && false) || (t >= 3.3 && t < 5.1 && Math.floor((t - 3.3) * 6.5 / Math.PI * 2) % 2 === 0);
  const spr = open ? 'TN_TOASTY_EAT' : 'TN_TOASTY';
  const happy = t >= 5.1 && t < 6.0;
  if (fstage < 3 && !(t < 0.4 && false)) food(fx, fy + (chew ? 1 : 0), fstage);
  const by = y0 - bob + (chew ? 1 : 0) * 0;
  // bottom slice: everything below the crust, minus the eyes
  put(spr, x, by, { rmin: 4, rmax: 8, noE: true });
  // crust, with the eyes riding along on it
  put(spr, x, by - lid, { rmin: 0, rmax: 3 });
  put(spr, x, by - lid, { rmin: 4, rmax: 4, onlyE: true, shut: happy });
  if (t >= 1.55 && t < 2.0 && Math.floor(t * 12) % 2 === 0) { rect(x + 13, y0 - 8, 1, 3, 3); rect(x + 13, y0 - 4, 1, 1, 3); }   // DING!
  if (t >= 3.7 && t < 5.1) for (let i = 0; i < 4; i++) { const k = ((t - 3.7) * 1.6 + i * 0.25) % 1; set(x + 6 + (rnd(i) - 0.5) * 18 * k, 21 + 6 * k * k * 3 - 6 * k, 2); }
  if (t > 6.0 && Math.floor(t * 5) % 2 === 0) spark(x + 12, y0 - 1);
}

// ---------------------------------------------------------------------------------------------
// MARSHY: a sticky marshmallow tongue shoots out, glues to the food, stringily reels it in, three squishy
// chews; a long goo string hangs from his lip and gets slurped back.
// ---------------------------------------------------------------------------------------------
const MARSHY = { v: 0 };   // 0: the food bounces off the left wall, 1: off the right wall (picked at random every feed)
function marshy(t) {
  // Pinball: the food drops from the sky, bounces off the top of his head, hits a side wall and ricochets straight
  // into his open mouth. Then three squishy marshmallow chews. He sits in the middle; the wall is picked at random.
  const x = 11, y0 = 18, mx = 19.5, my = 23;
  const F = FD(), flip = MARSHY.v === 1, X = v => flip ? 39 - v : v;
  const cw = 0.5 + F.w / 2;               // food centre when it touches the wall
  const headTop = y0 - F.h;
  let fx = 15, fy = -8, scl = 1, show = true;
  if (t < 0.3) show = false;
  else if (t < 0.85) { const k = pr(t, 0.3, 0.85); fx = 15; fy = lerp(-6, headTop, k * k); }
  else if (t < 1.65) { const k = pr(t, 0.85, 1.65); fx = lerp(15, cw, k); fy = lerp(headTop, 5, k) - Math.sin(k * Math.PI) * 5; }
  else if (t < 1.8) { fx = cw; fy = 5; }
  else if (t < 2.7) { const k = pr(t, 1.8, 2.7); fx = lerp(cw, mx, k); fy = lerp(5, my - 1, k * k * (1.4 - 0.4 * k)); scl = k < 0.65 ? 1 : Math.max(0.25, 1 - (k - 0.65) / 0.35 * 0.75); }
  else show = false;
  let h = 10, w = 17;
  const dent = t >= 0.85 && t < 1.05 ? 1 : 0;
  if (t >= 2.75 && t < 4.55) { const ch = Math.floor((t - 2.75) / 0.6), k = ((t - 2.75) % 0.6) / 0.6; const e = ch < 3 ? Math.sin(k * Math.PI) : 0; w = 17 + Math.round(2 * e); h = 10 + (ch < 3 ? Math.round(2 * e) : 0); }
  h -= dent;
  const open = t > 1.8 && t < 2.8;
  warp(open ? 'AD_MARSHY_EAT' : 'AD_MARSHY', x - Math.round((w - 17) / 2), 28 - h, w, h, { shut: t > 2.75 && t < 4.75 });
  if (show) {
    const ww = Math.max(2, Math.round(F.w * scl)), hh = Math.max(2, Math.round(F.h * scl));
    warp(F.n[0], Math.round(X(fx) - ww / 2), Math.round(fy), ww, hh);
  }
  if (t >= 1.65 && t < 1.9 && Math.floor(t * 20) % 2 === 0) put('SPARK', flip ? 37 : 0, 3);   // the wall bonk
  if (t >= 0.85 && t < 1.05) put('SPARK', X(14) - 1, 11);                                    // the head bonk
  if (t > 4.8 && Math.floor(t * 5) % 2 === 0) spark(x + 18, 16);
}

// ---------------------------------------------------------------------------------------------
// BOOP: a potion flask. His cork pops off, the food is flicked up and drops down the neck (plop), the
// "liquid" rises as it dissolves and fizzes; cork pops back on; steam; slowly drains.
// ---------------------------------------------------------------------------------------------
function boop(t) {
  const x = 3, y0 = 14, neckX = x + 6;
  if (t < 1.2) food(30, dropY(t, 0, 0.4, 23), 0);
  let cork = 0;
  if (t >= 0.5 && t < 1.0) cork = Math.round(ez(pr(t, 0.5, 0.7)) * 8);
  else if (t >= 1.0 && t < 4.3) cork = 8;
  else if (t >= 4.3 && t < 4.7) cork = Math.round((1 - pr(t, 4.3, 4.7)) * 8);
  const level = t < 2.5 ? 0 : t < 3.4 ? ez(pr(t, 2.5, 3.4)) * 0.6 : t < 5.2 ? 0.6 : Math.max(0, 0.6 * (1 - pr(t, 5.2, 6.8)));
  // liquid in the body (rows 7..12), behind the outline
  const rows = Math.round(level * 6);
  for (let k = 0; k < rows; k++) { const r = 12 - k, y = y0 + r; for (let c = 1; c < 12; c++) if (SP.AD_BOOP[r][c] === '.' && c > 0) set(x + c, y, 2); }
  const bounce = t > 4.3 && t < 4.9 ? Math.round(Math.sin((t - 4.3) * 25)) : 0;
  const shut = t > 1.8 && t < 5.2, open = t > 0.9 && t < 2.0;
  put(open ? 'AD_BOOP_EAT' : 'AD_BOOP', x + bounce, y0, { rmin: 4, rmax: 13, shut });
  put('AD_BOOP', x + bounce, y0 - cork, { rmin: 0, rmax: 3 });
  if (t >= 1.0 && t < 2.0) {   // the food arcs up over the neck and drops in
    const k = pr(t, 1.0, 2.0), fx = lerp(30, neckX, ez(k)), arc = Math.sin(k * Math.PI) * 10;
    const fy = k < 0.6 ? lerp(23, 4, k / 0.6) - 0 : lerp(4, y0 + 5, (k - 0.6) / 0.4);
    const ww = k < 0.55 ? FD().w : Math.max(3, Math.round(lerp(FD().w, 3, (k - 0.55) / 0.45)));
    warp(FD().n[0], Math.round(fx - ww / 2), Math.round(fy - 0), ww, k < 0.55 ? 5 : 3, {});
    void arc;
  }
  if (t >= 2.0 && t < 3.6) { const k = pr(t, 2.0, 3.6); const fy = Math.round(lerp(y0 + 5, 25, Math.min(1, k * 1.6))); if (k < 0.9) warp(FD().n[k < 0.5 ? 0 : 1], neckX - 2 + (k > 0.8 ? 0 : 0), fy - (k > 0.35 ? 0 : 0), 5, 3, { mid: k > 0.5 }); }
  if (t > 2.4 && t < 5.4) for (let i = 0; i < 3; i++) { const k = (t * 1.2 + i * 0.33) % 1; if (rows > 0) set(x + 4 + i * 3 + Math.round(Math.sin(t * 9 + i)), 25 - k * (rows * 1.0 + 2), 3); }
  if (t > 4.4 && t < 5.4) { const k = pr(t, 4.4, 5.4); bubble(neckX - 1 + Math.round(Math.sin(k * 8) * 2), Math.round(10 - k * 10)); }
  if (t > 6.7 && Math.floor(t * 5) % 2 === 0) spark(x + 13, 13);
}

// ---------------------------------------------------------------------------------------------
// CURLY: a little teapot. The lid hops, the food drops in; he heats up, shaking and whistling steam out of
// the spout; the lid rattles, then one big steam blast.
// ---------------------------------------------------------------------------------------------
function curly(t) {
  const x = 3, y0 = 15;
  if (t < 1.8) { if (t < 0.4) food(30, dropY(t, 0, 0.4, 23), 0); else if (t < 0.9) food(30, 23, 0); else { const k = pr(t, 0.9, 1.7); const fx = lerp(30, x + 7, ez(k)), fy = lerp(23, 2, Math.sin(k * Math.PI / 2)) + (k > 0.8 ? (k - 0.8) / 0.2 * 6 : 0); const ww = Math.max(3, Math.round(lerp(FD().w, 3, k))); warp(FD().n[0], Math.round(fx - ww / 2), Math.round(fy), ww, Math.max(3, Math.round(lerp(5, 3, k))), {}); } }
  let lid = 0;
  if (t >= 0.8 && t < 1.8) lid = 3;
  else if (t >= 2.0 && t < 4.6) lid = Math.floor(t * 14) % 2;
  else if (t >= 4.6 && t < 5.2) lid = Math.round(Math.sin(pr(t, 4.6, 5.2) * Math.PI) * 6);
  const heating = t > 1.9 && t < 4.7, shake = heating ? Math.round(Math.sin(t * 40) * (0.5 + (t - 1.9) / 3)) : 0;
  const shut = t > 1.8 && t < 5.4, open = t > 1.0 && t < 1.9;
  put(open ? 'TN_CURL_EAT' : 'TN_CURL', x + shake, y0 + 0, { rmin: 4, rmax: 12, shut });
  put('TN_CURL', x + shake, y0 - lid, { rmin: 0, rmax: 3 });
  const spx = x + 14, spy = y0 + 7;   // the spout
  if (t > 2.0 && t < 6.0) for (let i = 0; i < 6; i++) {
    const age = ((t - 2.0) * 1.4 + i / 6) % 1; if (t > 4.7 && t < 5.2) continue;
    const e = t > 4.6 ? 0.5 : 1;
    const px = Math.round(spx + age * 12 * e), py = Math.round(spy - age * 9 + Math.sin(age * 9 + i) * 1.2);
    if (age < 0.25) set(px, py, 3); else disc(px, py, age < 0.7 ? 1 : 2, 2);
  }
  if (t > 4.6 && t < 5.5) for (let i = 0; i < 7; i++) { const age = pr(t, 4.7 + i * 0.03, 5.5); if (age < 1) set(x + 6 + (rnd(i) - 0.5) * 10 * age * 2, y0 - 2 - age * 11, age < 0.5 ? 3 : 2); }
  if (t > 6.0 && Math.floor(t * 5) % 2 === 0) spark(x + 14, y0 - 1);
}

// ---------------------------------------------------------------------------------------------
// PUFF: a big vacuum inhale: suction lines drag the food across the floor into his mouth while he puffs up
// like a balloon (feet leave the floor); a squeaky hiccup, then he leaks air, whizzing little puffs, and drifts down.
// ---------------------------------------------------------------------------------------------
function puff(t) {
  const x0 = 3, fy = 23;
  const inh = pr(t, 0.9, 2.6), fx = t < 0.9 ? 30 : lerp(30, 14, ez(inh));
  if (t < 2.7) { const sc = t > 1.9 ? Math.max(0, 1 - pr(t, 1.9, 2.6)) : 1; if (t < 0.4) food(30, dropY(t, 0, 0.4, fy), 0); else { const ww = Math.max(2, Math.round(FD().w * sc)); warp(FD().n[0], Math.round(fx - ww / 2), 28 - Math.max(2, Math.round(5 * sc)) - (inh > 0 ? 0 : 0), ww, Math.max(2, Math.round(5 * sc)), {}); } }
  let sc = 1, lift = 0;
  if (t >= 1.4 && t < 3.0) sc = lerp(1, 1.32, ez(pr(t, 1.4, 3.0)));
  else if (t >= 3.0 && t < 3.5) { sc = 1.32 + 0.06 * Math.sin((t - 3.0) * 50); }
  else if (t >= 3.5 && t < 5.4) { sc = lerp(1.32, 1, ez(pr(t, 3.5, 5.4))); lift = Math.round(Math.sin(pr(t, 3.5, 5.4) * Math.PI) * 5); }
  const w = Math.round(19 * sc), h = Math.round(13 * sc), x = Math.round(12.5 - w / 2 + x0 - 3 + 3);
  const open = t > 0.8 && t < 2.7;
  warp(open ? 'AD_PUFF_EAT' : 'AD_PUFF', x, 28 - h - lift, w, h, { shut: t > 2.6 && t < 5.6 });
  if (t > 0.9 && t < 2.6) for (let i = 0; i < 3; i++) { const k = ((t * 2.2 + i / 3) % 1); const lx = Math.round(lerp(fx + 6, 22, k)); const ly = 24 - i * 3; line(lx, ly, lx + 3, ly, k < 0.3 ? 2 : 3); }
  if (t > 3.0 && t < 3.5 && Math.floor(t * 16) % 2 === 0) put('SPARK', 24, 12);
  if (t > 3.5 && t < 5.4) for (let i = 0; i < 4; i++) { const k = pr(t, 3.5 + i * 0.25, 4.6 + i * 0.25); if (k > 0 && k < 1) bubble(Math.round(x + (i % 2 ? w : -3) + (i % 2 ? 1 : -1) * k * 6), Math.round(28 - h / 2 - lift - k * 4 * (i % 2 ? 1 : -1) )); }
  if (t > 5.6 && Math.floor(t * 5) % 2 === 0) spark(22, 12);
}

// ---------------------------------------------------------------------------------------------
// LOAF: a cat loaf. Butt wiggle, pounce, sploot flat over the food; a lump travels down the body from head to
// tail; he rises like proving bread, then slumps back down with a content sploot.
// ---------------------------------------------------------------------------------------------
function loaf(t) {
  const fcx = 7;
  if (t < 1.5) food(fcx, dropY(t, 0, 0.4, 23), 0);
  let left = 19, w = 20, h = 11, lift = 0, bump = null, dxr = null;
  if (t >= 0.6 && t < 1.3) { dxr = r => 0; }
  const wig = t >= 0.6 && t < 1.3 ? (Math.sin(t * 40) > 0 ? 0 : -1) : 0;
  if (t >= 1.3 && t < 2.0) { const k = ez(pr(t, 1.3, 2.0)); left = Math.round(lerp(19, 4, k)); w = Math.round(lerp(20, 25, Math.sin(k * Math.PI / 1) ) + (k > 0.5 ? lerp(0, 0, 0) : 0)); w = k < 0.5 ? Math.round(lerp(20, 27, k * 2)) : Math.round(lerp(27, 24, (k - 0.5) * 2)); lift = Math.round(Math.sin(k * Math.PI) * 7); }
  else if (t >= 2.0) { left = 4; w = 24; }
  if (t >= 2.0 && t < 2.3) { h = 11 - Math.round(Math.sin(pr(t, 2.0, 2.3) * Math.PI) * 3); }
  const eaten = t > 2.3;
  if (t >= 2.0 && t < 2.3) { /* landing sploot */ }
  let colH = null;
  if (t >= 2.4 && t < 4.4) { const k = pr(t, 2.4, 4.4), cx = lerp(2, w - 2, k); colH = c => Math.round(3 * Math.exp(-Math.pow((c - cx) / 5, 2))); }
  else if (t >= 4.4 && t < 5.8) { const k = ez(pr(t, 4.4, 5.4)); colH = c => Math.round(3 * k * (c > 2 && c < w - 2 ? 1 : 0.4)); }
  else if (t >= 5.8 && t < 6.3) { const k = 1 - ez(pr(t, 5.8, 6.3)); colH = c => Math.round(3 * k); }
  const shut = t > 2.3 && t < 6.5, open = t > 1.5 && t < 2.4;
  const n = open ? 'AD_LOAF_EAT' : 'AD_LOAF';
  const R = SP[n], sw_ = 20;
  for (let cx = 0; cx < w; cx++) {
    const extra = colH ? colH(cx) : 0, ch = h + extra, sx = w > 1 ? Math.round(cx * (sw_ - 1) / (w - 1)) : 0;
    const wg = (t >= 0.6 && t < 1.3 && cx >= 15) ? wig : 0;   // only the rear end wiggles; head and eyes stay put
    for (let dy = 0; dy < ch; dy++) {
      const sy = ch > 1 ? Math.round(dy * (11 - 1) / (ch - 1)) : 0, c = R[sy][sx];
      let v = c === 'X' || c === 'M' ? 3 : c === 'E' ? (shut ? 0 : 3) : c === 'G' ? 2 : 0; if (v) set(left + cx, 28 - ch - lift + dy + wg, v);
    }
  }
  if (t > 6.3 && Math.floor(t * 5) % 2 === 0) spark(left + w - 3, 11);
}

// ---------------------------------------------------------------------------------------------
// NIMBUS: floats over the food and vortexes it up into his belly; he goes dark and grumbly, lightning cracks
// (screen flash), it pours; then he clears up and a little rainbow appears.
// ---------------------------------------------------------------------------------------------
function nimbus(t) {
  let left = 3, by = 16, rain = t < 1.0 || (t > 3.9 && t < 6.0), storm = t > 3.3 && t < 5.9;
  if (t >= 1.0 && t < 2.2) left = 3 + Math.round(ez(pr(t, 1.0, 2.2)) * 19);
  else if (t >= 2.2 && t < 6.2) left = 22; else if (t >= 6.2) left = 22;
  const hover = Math.round(Math.sin(t * 3) * 1);
  by = 16 + (t >= 1.0 ? -3 : 0) + hover;
  if (t < 3.4) {
    const fcx = 31;
    if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
    else if (t < 2.4) food(fcx, 23, 0);
    else { const k = ez(pr(t, 2.4, 3.4)); const ww = Math.max(2, Math.round(FD().w * (1 - k * 0.7))); const ang = k * 9; if (k < 1) warp(FD().n[0], Math.round(fcx - ww / 2 + Math.sin(ang) * 3 * (1 - k)), Math.round(lerp(23, by + 4, k)), ww, Math.max(2, Math.round(5 * (1 - k * 0.5))), { mid: k > 0.5 }); }
  }
  const body = { shut: t > 3.3 && t < 5.9, dark: storm };
  put(storm ? 'AD_NIMBUS_EAT' : 'AD_NIMBUS', left, by, { rmin: 0, rmax: 8, shut: body.shut, dark: storm && Math.floor(t * 3) % 2 === 0 || (storm && t > 3.6) });
  if (rain) for (let c = 0; c < 3; c++) for (let i = 0; i < 3; i++) { const sp = storm ? 12 : 5, yy = by + 9 + ((t * sp + c * 2 + i * 3.1) % 10); if (yy < 28) set(left + 2 + c * 5 + (storm ? i % 2 : 0), yy, storm ? 3 : 2); if (storm && yy < 28 && c !== 1) set(left + 5 + c * 4, ((yy + 4) % 12) + by + 9 > 27 ? 27 : by + 9 + ((yy + 4) % 12 - by) % 10, 2); }
  if (t >= 3.3 && t < 3.55) invert = Math.floor(t * 30) % 2 === 0;
  if (t >= 3.35 && t < 3.7) { let x = left + 8, y = by + 9; for (let i = 0; i < 5; i++) { const nx = x + (i % 2 ? 2 : -2), ny = y + 3; line(x, y, nx, ny, 3); x = nx; y = ny; if (y > 27) break; } }
  if (t >= 3.9 && t < 6.0) { const k = pr(t, 3.9, 5.8); const w = Math.round(k * 12); rect(left + 8 - Math.round(w / 2), 27, w, 1, 2); }
  if (t >= 5.9) {
    put('AD_NIMBUS', left, by, { rmin: 0, rmax: 8, shut: t < 6.4 });
    const k = pr(t, 6.0, 6.8); for (let a = 0; a < 17 * k; a++) { const ang = Math.PI * (1 - a / 17); const cx = left + 8, cy = by - 1; set(cx + Math.cos(ang) * 9, cy - Math.sin(ang) * 7, 3); set(cx + Math.cos(ang) * 8, cy - Math.sin(ang) * 6, 2); }
  }
}

// ---------------------------------------------------------------------------------------------
// BALLBOI: the hamster in the ball rolls over, then stuffs his cheeks one gulp at a time: they balloon out
// of the ball. Chews, shuts his eyes, then ptooey: a tiny fish skeleton flies out and the cheeks deflate.
// ---------------------------------------------------------------------------------------------
// rotate a sprite about its centre (nearest neighbour)
// ---------------------------------------------------------------------------------------------
// BALLBOI: the hamster in the ball rolls over, then stuffs his cheeks one gulp at a time: they balloon out
// of the ball. Chews, shuts his eyes, then ptooey: a tiny fish skeleton flies out and the cheeks deflate.
// ---------------------------------------------------------------------------------------------
// rotate a sprite about its centre (nearest neighbour). half: draw at half size (zoomed out) by 2x2 downsampling.
function rotSprite(name, cx, cy, ang, half) {
  const R = SP[name], w = R[0].length, h = R.length, mx = (w - 1) / 2, my = (h - 1) / 2, c = Math.cos(ang), sn = Math.sin(ang), N = 18;
  const tmp = new Uint8Array(N * N);
  for (let dy = 0; dy < N; dy++) for (let dx = 0; dx < N; dx++) {
    const ux = dx - (N - 1) / 2, uy = dy - (N - 1) / 2;
    const ix = Math.round(c * ux + sn * uy + mx), iy = Math.round(-sn * ux + c * uy + my);
    if (ix < 0 || iy < 0 || ix >= w || iy >= h) continue;
    const ch = R[iy][ix]; tmp[dy * N + dx] = ch === 'X' || ch === 'E' || ch === 'M' ? 3 : ch === 'G' ? 2 : 0;
  }
  if (!half) { for (let dy = 0; dy < N; dy++) for (let dx = 0; dx < N; dx++) if (tmp[dy * N + dx]) set(cx - N / 2 + dx, cy - N / 2 + dy, tmp[dy * N + dx]); return; }
  for (let dy = 0; dy < N / 2; dy++) for (let dx = 0; dx < N / 2; dx++) {
    let on = 0, mid = 0;
    for (let j = 0; j < 2; j++) for (let i = 0; i < 2; i++) { const v = tmp[(dy * 2 + j) * N + dx * 2 + i]; if (v === 3) on++; else if (v === 2) mid++; }
    if (on) set(cx - N / 4 + dx, cy - N / 4 + dy, 3); else if (mid >= 2) set(cx - N / 4 + dx, cy - N / 4 + dy, 2);
  }
}
// ---------------------------------------------------------------------------------------------
// BALLBOI: the camera zooms out, and the hamster ball rolls in from off-screen, spinning twice, across to the food.
// Zoom back in: he stuffs his cheeks one gulp at a time (they balloon out of the ball), chews, then ptooey:
// a tiny fish skeleton flies out and the cheeks deflate.
// ---------------------------------------------------------------------------------------------
const BB_T0 = 3.25;
// The roll is a hamster ball: the clear shell spins (two grey seams whirl round the rim) while Ballboi's
// head stays upright inside, running: bobbing, little feet paddling, leaning into it, then sloshing
// forward when the ball skids to a stop.
// his real face (the inside of his sprite), sitting at the BOTTOM of the ball like a real hamster ball:
// the shell rolls round him, he stays down, running (bobbing, leaning into the front of the ball).
const BB_HEAD = ['.XX...XX.', 'X..XXX..X', 'X.......X', 'X.E...E.X', 'X...M...X', '.X.....X.', '..XXXXX..'];
function hamsterBall(cx, cy, ang, lean, bob) {
  const R = 6;
  for (let dy = -R - 1; dy <= R + 1; dy++) for (let dx = -R - 1; dx <= R + 1; dx++) {
    const r = Math.hypot(dx, dy); if (r < R - 0.5 || r >= R + 0.5) continue;
    let th = Math.atan2(dy, dx) - ang, seam = false;
    for (const o of [0, Math.PI]) { let q = ((th - o) % (2 * Math.PI) + 3 * Math.PI) % (2 * Math.PI) - Math.PI; if (Math.abs(q) < 0.42) seam = true; }
    set(cx + dx, cy + dy, seam ? 2 : 3);
  }
  set(Math.round(cx + Math.cos(ang + Math.PI / 2) * 4.6), Math.round(cy + Math.sin(ang + Math.PI / 2) * 4.6), 2);   // air vent, spins with the shell
  const hx = cx - 4 + lean, hy = cy - 1 + bob;   // bottom row of his face rests just above the bottom of the shell
  BB_HEAD.forEach((r, y) => { for (let x = 0; x < 9; x++) if (r[x] !== '.') set(hx + x, hy + y, 3); });
}
function ballboi(t) {
  if (t >= BB_T0) return ballboiScene(t - BB_T0 + 1.5);
  const F = FD();
  // zoomed out: the food at half size
  const fw = Math.max(2, Math.round(F.w / 2)), fh = Math.max(2, Math.round(F.h / 2));
  warp(F.n[0], 35 - Math.round(fw / 2), t < 0.4 ? dropY(t, 0, 0.4, 27 - fh) : 27 - fh + 1, fw, fh);
  const R = 6, x0 = -8, x1 = 26 - Math.round(fw / 2);
  const q0 = pr(t, 0.2, 2.5), k = 1 - (1 - q0) * (1 - q0), d = (x1 - x0) * k;
  const moving = q0 > 0 && q0 < 1;
  // after the stop the ball rocks back and forth a little; the hamster sloshes forward then settles
  const rock = t > 2.5 ? Math.sin((t - 2.5) * 14) * Math.exp(-(t - 2.5) * 5) : 0;
  const cx = Math.round(x0 + d + rock * 1.2), cy = 27 - R;
  const ang = (d / R) * 2 + rock * 0.6;                          // spins about twice on the way in
  const spd = moving ? 2 * (1 - q0) : 0;                        // how hard he's running
  const lean = moving ? (spd > 0.4 ? 1 : 0) : (t > 2.5 && t < 2.8 ? 1 : 0);
  const step = Math.floor(t * (6 + spd * 8)) % 2;
  const bob = (moving && step) || (t > 2.5 && t < 2.65) ? -1 : 0;   // running bob; and he rides up the front when it stops
  hamsterBall(cx, cy, ang, lean, bob);
  for (let i = 0; i < 3 && moving && k < 0.95; i++) set(cx - 8 - i * 3 - Math.round(rnd(Math.floor(t * 10) + i) * 2), 26 - (i % 2), 2);   // dust
  if (t > 2.5 && t < 2.8 && Math.floor(t * 20) % 2 === 0) set(cx + R + 1, cy - 3, 3);   // bonk
  if (t >= 3.0) { const q = pr(t, 3.0, BB_T0); const xa = Math.round(q * 8), xb = 39 - Math.round(q * 4), ya = Math.round(q * 12); for (let x = xa; x <= xb; x++) { set(x, ya, 3); set(x, 27, 3); } for (let y = ya; y <= 27; y++) { set(xa, y, 3); set(xb, y, 3); } }
}
function ballboiScene(t) {
  const fcx = 31;
  const gul = t < 1.6 ? 0 : t < 2.1 ? 1 : t < 2.6 ? 2 : 3;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0); else if (t < 2.7) food(fcx, 23, Math.min(2, gul));
  let x = 2, bob = 0;
  if (t >= 0.4 && t < 1.5) { const k = pr(t, 0.4, 1.5); x = 2 + Math.round(k * 9); bob = hop(k, 4, 1); }
  else if (t >= 1.5) x = 11;
  let cheek = t < 1.6 ? 0 : t < 2.7 ? gul + pr(t, 1.6 + (gul - 1) * 0.5, 2.1 + (gul - 1) * 0.5) * 0 : t < 4.2 ? 3 : t < 4.8 ? 3 * (1 - pr(t, 4.2, 4.8)) : 0;
  cheek = Math.round(cheek);
  const open = t > 1.5 && t < 2.7 && Math.floor(t * 6) % 2 === 0;
  const shut = t > 2.7 && t < 5.0;
  const w = 17 + cheek, hh = 15;
  warp(open ? 'AD_BALLBOI_EAT' : 'AD_BALLBOI', x - Math.round(cheek / 2), 13 - bob, w, hh, { shut });
  if (cheek > 0) for (const sd of [-1, 1]) { const cx = sd < 0 ? x - Math.round(cheek / 2) + 2 : x - Math.round(cheek / 2) + w - 3; disc(cx, 21, Math.min(2, 1 + (cheek > 1 ? 1 : 0)), 2); }
  if (t >= 1.6 && t < 2.7) { const g = (t - 1.6) / 0.5, gi = Math.floor(g), k = g - gi; if (gi < 3) { const px = lerp(fcx, x + 8, k), py = 23 - Math.sin(k * Math.PI) * 6 + k * 3; warp(FD().n[0], Math.round(px - 2), Math.round(py), 4, 3, { mid: false }); } }
  if (t >= 4.3 && t < 5.4) { const k = pr(t, 4.3, 5.4); const bx = x + 14 + k * 18, by_ = 20 + 14 * k * k - 10 * k; put('BONE', Math.round(bx), Math.round(by_)); }
  if (t > 5.4 && Math.floor(t * 5) % 2 === 0) spark(x + 17, 12);
}

// ---------------------------------------------------------------------------------------------
// SPROUT: hops onto the food, which fizzles into glowing dots that travel up his stem; his fronds shoot up
// on a long stem, a flower buds and blooms at the top, sways; then it pops and the petals drift down.
// ---------------------------------------------------------------------------------------------
function sprout(t) {
  const fcx = 29, x0 = 3;
  let x = x0, hopY = 0;
  if (t >= 0.4 && t < 1.5) { const k = pr(t, 0.4, 1.5); x = x0 + Math.round(k * 18); hopY = hop(k, 3, 3); }
  else if (t >= 1.5) x = x0 + 18;
  const stage = t < 1.7 ? 0 : t < 2.1 ? 1 : t < 2.5 ? 2 : 3;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0); else food(fcx, 23, stage, { mid: stage > 0 });
  let grow = 0;
  if (t >= 2.3 && t < 3.3) grow = Math.round(ez(pr(t, 2.3, 3.3)) * 6);
  else if (t >= 3.3 && t < 5.6) grow = 6; else if (t >= 5.6 && t < 6.2) grow = Math.round((1 - ez(pr(t, 5.6, 6.2))) * 6);
  const shut = t > 1.7 && t < 5.9;
  const y0 = 13 - hopY;
  put('TN_SPROUT', x, y0 + 0, { rmin: 5, rmax: 14, shut });
  for (let i = 1; i <= grow; i++) set(x + 7, y0 + 5 - i + 1, 3);   // the stem
  const sway = t > 3.3 && t < 5.6 ? Math.round(Math.sin(t * 4) * 1) : 0;
  put('TN_SPROUT', x + sway, y0 - grow, { rmin: 0, rmax: 4 });
  if (t >= 1.7 && t < 2.8) for (let i = 0; i < 4; i++) { const k = ((t - 1.7) * 1.3 + i * 0.25) % 1; set(x + 7 + Math.round(Math.sin(k * 8 + i) * 1.5), y0 + 13 - k * 12, 3); }
  if (t >= 3.2 && t < 5.7) { const k = ez(pr(t, 3.2, 3.9)); const s = Math.max(1, Math.round(5 * k)); if (s >= 5) put('FLOWER', x + 5 + sway, y0 - grow - 5); else if (s > 1) rect(x + 7 + sway, y0 - grow - s + 1, 1, s, 3); }
  if (t >= 5.6 && t < 6.6) { const k = pr(t, 5.6, 6.6); for (let i = 0; i < 4; i++) set(x + 7 + (i - 1.5) * 3 * (0.4 + k) + Math.sin(k * 6 + i) * 1, y0 - 2 + k * 14 + i, 3); }
  if (t > 6.2 && Math.floor(t * 5) % 2 === 0) spark(x + 14, y0 + 1);
}

// ---------------------------------------------------------------------------------------------
// FUBZ: three polite bites... then the hack: he heaves (body jolts and stretches), and a fuzzy grey
// hairball the size of the meal rolls out and tumbles away. Relieved.
// ---------------------------------------------------------------------------------------------
function fubz(t) {
  const x = 3, bite = Math.min(3, Math.floor(t / 0.8)), openb = t < 2.4 && (t % 0.8) < 0.4;
  if (t < 0.4) food(24, dropY(t, 0, 0.4, 23), 0); else if (t < 2.4) food(24, 23, bite);
  const heave = t >= 2.6 && t < 4.4;
  const dxs = heave ? Math.round(Math.sin(t * 32) * (0.5 + pr(t, 2.6, 4.4))) : 0;
  const st = heave ? (Math.floor(t * 5) % 2) : 0;           // stretches upward by one row, feet stay on the floor
  const bounce = !heave && t < 2.4 && (t % 0.8) > 0.4 && (t % 0.8) < 0.55;
  const sprn = openb || (heave && t < 4.2 && Math.floor(t * 4) % 2 === 0) ? 'AD_FUBZ_EAT' : 'AD_FUBZ';
  const shut = (!openb && t >= 0.8 && t < 2.5 && (t % 0.8) > 0.4) || heave || (t > 5.2 && t < 6.2);
  const h = 13 + st;
  warp(sprn, x + dxs, 28 - h - (bounce ? 1 : 0), 17, h, { shut });
  if (t >= 4.1 && t < 6.2) {   // the hairball comes out of his mouth and rolls away
    const k = pr(t, 4.1, 5.2), bx = x + 9 + k * 20, by = 24 - Math.sin(Math.min(1, k) * Math.PI) * 8;
    const roll = t > 5.2 ? Math.min(8, (t - 5.2) * 14) : 0;
    const cx = Math.round(bx + roll), cy = Math.round(t > 5.2 ? 25 : by);
    disc(cx, cy, 2, 2); set(cx - 1, cy - 1, 3); set(cx + 1, cy, 3);
  }
  if (t > 5.0 && t < 5.4 && Math.floor(t * 20) % 2 === 0) put('SPARK', x + 19, 12);
  if (t > 6.2 && Math.floor(t * 5) % 2 === 0) spark(x + 16, 13);
}

// ---------------------------------------------------------------------------------------------
// DAPPER: dinner is served. A floating knife & fork cut the meal in pieces (tink!); the fork delivers a bite
// to his mouth, he chews with his eyes shut, another bite, then he dabs his mouth with a napkin.
// ---------------------------------------------------------------------------------------------
function dapper(t) {
  const x = 2, y0 = 12;
  const fl = 24;
  // plate
  rect(fl - 1, 27, FD().w + 2, 1, 2);
  const cut = t > 1.9, st = t < 3.2 ? 0 : t < 4.4 ? 1 : t < 5.3 ? 2 : 3;
  if (t < 0.4) food(fl + 4, dropY(t, 0, 0.4, 22), 0); else if (st < 3 && !(t >= 3.0 && t < 3.2)) food(fl + 4, 22, st);
  const openm = (t > 3.0 && t < 3.5) || (t > 4.2 && t < 4.7);
  const shut = (t >= 3.5 && t < 4.2) || (t >= 4.7 && t < 5.4);
  const eat = openm ? 'S1C_EAT' : 'S1C_IDLE';
  put(eat, x, y0 - (t > 0.4 && t < 0.6 ? 1 : 0), { shut });
  // knife & fork
  if (t >= 0.5 && t < 1.2) { const k = ez(pr(t, 0.5, 1.2)); put('FORK', fl + 6, Math.round(lerp(-5, 15, k))); put('KNIFE', fl + 1, Math.round(lerp(-5, 15, k))); }
  else if (t >= 1.2 && t < 2.9) { const sw_ = t > 1.7 && t < 2.8 ? Math.round(Math.sin((t - 1.7) * 16) > 0 ? 1 : 0) : 0; put('FORK', fl + 6, 15 + 0); put('KNIFE', fl + 1 + sw_, 15); if (sw_ && Math.floor(t * 8) % 4 === 0) spark(fl + 3, 12); }
  else if (t >= 2.9 && t < 3.5) { const k = ez(pr(t, 2.9, 3.5)); put('FORK', Math.round(lerp(fl + 6, x + 14, k)), Math.round(lerp(15, y0 + 8, k))); const ww = 3; warp(FD().n[2], Math.round(lerp(fl + 6, x + 14, k)) + 1, Math.round(lerp(15, y0 + 8, k)) - 3, ww, 3, {}); put('KNIFE', fl + 1, 15); }
  else if (t >= 3.5 && t < 4.0) { const k = ez(pr(t, 3.5, 4.0)); put('FORK', Math.round(lerp(x + 14, fl + 6, k)), Math.round(lerp(y0 + 8, 15, k))); put('KNIFE', fl + 1, 15); }
  else if (t >= 4.0 && t < 4.7) { const k = ez(pr(t, 4.0, 4.5)); put('FORK', Math.round(lerp(fl + 6, x + 14, k)), Math.round(lerp(15, y0 + 8, k))); warp(FD().n[2], Math.round(lerp(fl + 6, x + 14, k)) + 1, Math.round(lerp(15, y0 + 8, k)) - 3, 3, 3, {}); put('KNIFE', fl + 1, 15); }
  else if (t >= 4.7 && t < 5.4) { const k = ez(pr(t, 4.7, 5.4)); put('FORK', Math.round(lerp(x + 14, fl + 6, k)), Math.round(lerp(y0 + 8, -6, k))); put('KNIFE', fl + 1, Math.round(lerp(15, -6, k))); }
  if (t >= 5.2 && t < 6.4) { const k = pr(t, 5.2, 6.4), nx = Math.round(lerp(x + 15, x + 9, Math.min(1, k * 3))), wipe = Math.round(Math.sin(k * 20) * 1.2); rect(nx + wipe, y0 + 8, 4, 3, 2); rect(nx + wipe, y0 + 8, 4, 1, 3); }
  if (t > 6.4 && Math.floor(t * 5) % 2 === 0) spark(x + 16, y0 + 1);
}

// =============================================================================================
// ROUND 2: the rest of the cast
// =============================================================================================
// a floor-sitting sprite stretched to w x h, centred on cx, bottom on the floor (or lifted)
function floorWarp(n, cx, w, h, lift, o) { warp(n, Math.round(cx - w / 2), 28 - Math.round(h) - (lift || 0), w, h, o); }
function ring(cx, cy, r, v, fill) {
  for (let dy = -r - 1; dy <= r + 1; dy++) for (let dx = -r - 1; dx <= r + 1; dx++) {
    const d = Math.hypot(dx, dy);
    if (fill != null && d < r - 0.5) set(cx + dx, cy + dy, fill);
    else if (d >= r - 0.5 && d < r + 0.5) set(cx + dx, cy + dy, v);
  }
}
function shiftBuf(dx) {   // screen shake
  if (!dx) return; const c = buf.slice();
  for (let y = 0; y < H; y++) for (let x = 0; x < W; x++) { const sx = x - dx; buf[y * W + x] = sx >= 0 && sx < W ? c[y * W + sx] : 0; }
}
const stepIn = (t, ts) => { let i = 0; while (i < ts.length && t >= ts[i]) i++; return i; };

// ---------------------------------------------------------------------------------------------
// BEAN: a jumping bean. Swallows the food whole, then it rattles around inside him and he can't stop
// hopping: three wild hops, tilting, the lump bouncing about. Lands, a little "pop", and calm.
// ---------------------------------------------------------------------------------------------
function bean(t) {
  const F = FD(), fcx = 31, y0 = 17;
  let x = 3, lift = 0, tilt = 0, sq = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0); else if (t < 1.2) food(fcx, 23, 0);
  if (t >= 0.4 && t < 1.2) { const k = pr(t, 0.4, 1.2); x = Math.round(lerp(3, 13, ez(k))); lift = hop(k, 2, 2); }
  else if (t >= 1.2) x = 13;
  // the gulp: the whole meal slides into his mouth, shrinking
  if (t >= 1.2 && t < 1.9) { const k = ez(pr(t, 1.2, 1.9)); const w = lerp(F.w, 1, k), h = lerp(F.h, 1, k); warp(F.n[0], Math.round(lerp(fcx, x + 6, k) - w / 2), Math.round(lerp(25, y0 + 8, k) - h / 2), w, h); }
  // three wild hops
  const hops = [[2.3, 3.0, 6, -3], [3.05, 3.85, 8, 4], [3.9, 4.5, 4, -2]];
  let hx = 0;
  for (const [a, b, ht, dx] of hops) {
    if (t >= b) hx += dx;
    else if (t >= a) { const k = pr(t, a, b); lift = Math.round(Math.sin(k * Math.PI) * ht); hx += Math.round(dx * k); tilt = Math.sign(dx) * (k < 0.5 ? 1 : -1); }
  }
  for (const [, b] of hops) if (t >= b && t < b + 0.08) sq = 1;
  if (t >= 4.5 && t < 4.65) sq = 1;
  x += hx;
  const open = (t >= 1.2 && t < 1.9) || (t >= 4.85 && t < 5.0);
  const shut = t > 5.0 && t < 6.2;
  const h = 11 - sq, w = 13 + sq * 2;
  warp(open ? 'TN_BEAN_EAT' : 'TN_BEAN', x - sq, 28 - h - lift, w, h, { shut, dx: r => Math.round((5 - r) * tilt * 0.2) });
  // the lump rattling around inside
  if (t >= 1.9 && t < 4.85) {
    const lx = 5 + Math.round(Math.sin(t * 13) * 2.4), ly = (lift > 2 ? 7 : 3) + Math.round(Math.cos(t * 17));
    rect(x + lx - tilt * Math.round((5 - ly) * 0.2), 28 - h - lift + ly, 3, 2, 2);
    if (t > 2.3 && Math.floor(t * 12) % 2 === 0) { set(x - 2, 28 - h - lift + 3, 3); set(x + 15, 28 - h - lift + 5, 3); set(x - 3, 28 - h - lift + 6, 3); }   // rattle lines
  }
  if (t >= 1.95 && t < 2.3) put('SPARK', x + 6, y0 - 6);   // uh-oh
  if (t >= 4.85 && t < 5.15) { ring(x + 6, y0 - 3, 1 + Math.round(pr(t, 4.85, 5.15) * 2), 3); }   // pop!
  if (t > 5.6 && Math.floor(t * 5) % 2 === 0) spark(x + 14, y0);
}

// ---------------------------------------------------------------------------------------------
// BUN: a hot-dog bun. Splits wide open down the middle (an eye on each half), the food hops in sideways,
// he claps shut with a SQUISH and puffs steam while he chews.
// ---------------------------------------------------------------------------------------------
function bun(t) {
  const F = FD(), x = 8, y0 = 17, fcx = 32;
  let g = 0;
  if (t >= 0.7 && t < 1.3) g = ez(pr(t, 0.7, 1.3)) * 5;
  else if (t >= 1.3 && t < 2.1) g = 5 + Math.round(Math.sin(t * 9) * 0.5);
  else if (t >= 2.1 && t < 2.3) g = lerp(5, -1, pr(t, 2.1, 2.3));
  else if (t >= 2.3 && t < 2.45) g = -1;
  g = Math.round(g);
  const chewing = t >= 2.6 && t < 4.4, chewk = (t - 2.6) / 0.6, chewOpen = chewing && (chewk % 1) < 0.5;
  const st = t < 2.6 ? 0 : Math.min(3, Math.floor(chewk) + (chewk % 1 > 0.5 ? 1 : 0));
  const bob = chewing && (chewk % 1) < 0.5 ? 1 : 0;
  // the food
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  else if (t < 1.3) food(fcx, 23, 0);
  else if (t < 2.1) {   // flips up and lands sideways in the gap
    const k = pr(t, 1.3, 2.1), cx = lerp(fcx, x + 7.5, ez(k)), cy = 23 - Math.sin(k * Math.PI) * 11 + k * 0;
    food(cx, Math.round(cy), 0, { flip: k > 0.5 });
  }
  else if (t < 2.3) food(x + 7.5, 22, 0);
  else if (st < 3) { food(x + 7.5, 21 + bob, st, { mid: true }); }
  const name = chewOpen ? 'TN_BUN_EAT' : 'TN_BUN';
  const shut = t > 4.5 && t < 5.6, top = y0 + bob;
  put(name, x - g, top, { cmin: 0, cmax: 7, shut });
  put(name, x + g, top, { cmin: 7, cmax: 14, shut });
  if (t >= 2.3 && t < 2.5) { for (const sx of [x - 3, x + 17]) { set(sx, y0 + 4, 3); set(sx - Math.sign(sx - x) , y0 + 6, 3); set(sx, y0 + 8, 3); } }   // SQUISH
  for (let i = 0; i < 3; i++) {   // steam puffs
    const t0 = 2.45 + i * 0.55; if (t < t0 || t > t0 + 1.3) continue;
    const k = pr(t, t0, t0 + 1.3); disc(Math.round(x + 7 + Math.sin(k * 7 + i) * 2 + (i - 1) * 2), Math.round(y0 - 2 - k * 12), k < 0.6 ? 1 : 0, 2);
  }
  if (t > 5.5 && Math.floor(t * 5) % 2 === 0) spark(x + 17, y0);
}

// ---------------------------------------------------------------------------------------------
// SLOSH: a jar of liquid. Slurps the food up as a stream of dots and his level rises; then he sloshes
// side to side, a drop splashes over the rim... he hurries over and slurps that up too.
// ---------------------------------------------------------------------------------------------
function sloshBody(name, x, y, L, T, wph, o) {
  const R = SP[name], w = R[0].length, mid = (w - 1) / 2;
  for (let r = 0; r < R.length; r++) {
    const row = R[r]; let l = row.indexOf('X'), rr = row.lastIndexOf('X');
    if (l < 0) continue;
    let le = l; while (row[le + 1] === 'X') le++;
    let rs = rr; while (row[rs - 1] === 'X') rs--;
    const edge = r === 0 || r === R.length - 1;
    const dx = o && o.dx ? o.dx(r) : 0;
    for (let c = 0; c < w; c++) {
      const ch = row[c];
      if (ch === 'X' && (edge || c <= le || c >= rs)) set(x + c + dx, y + r, 3);
      if (!edge && c > le && c < rs) {
        const surf = L + T * (c - mid) / mid;
        if (r > surf + 0.5) set(x + c + dx, y + r, 2);
        else if (Math.abs(r - surf) <= 0.5) set(x + c + dx, y + r, ((c + wph) % 4 + 4) % 4 < 2 ? 3 : 2);
      }
      if (ch === 'E' && !(o && o.shut)) set(x + c + dx, y + r, 3);
      if (ch === 'M') set(x + c + dx, y + r, 3);
    }
  }
}
function slosh(t) {
  const fcx = 31, y0 = 16, F = FD();
  let x = 2, L = 6.4, T = 0, lift = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.0) { const k = pr(t, 0.4, 1.0); x = Math.round(lerp(2, 11, ez(k))); lift = hop(k, 2, 1); T = Math.sin(k * 12) * 1.2; }
  else if (t >= 1.0) x = 11;
  // drinking
  const st = stepIn(t, [1.5, 2.0, 2.5]);
  if (t >= 0.4 && t < 2.6) food(fcx, 23, Math.min(3, st));
  if (t >= 1.0 && t < 2.6) {
    L = lerp(6.4, 4.6, pr(t, 1.0, 2.6));
    for (let i = 0; i < 4; i++) { const k = ((t - 1.0) * 2.2 + i / 4) % 1; set(Math.round(lerp(fcx - 3, x + 8, k)), Math.round(lerp(25, y0 + 5, k) - Math.sin(k * Math.PI) * 3), 2); }
  } else if (t >= 2.6) L = 4.6;
  // the slosh
  let sx = 0;
  if (t >= 2.6 && t < 4.4) { const k = t - 2.6; T = 2.6 * Math.sin(k * 7) * Math.exp(-k * 0.6); sx = -Math.round(Math.sin(k * 7) * Math.exp(-k * 0.6)); }
  // the spilled drop
  const dropT0 = 2.6 + (Math.PI / 2) / 7;   // the first big tilt to the right
  let dx_ = null, dy_ = null;
  if (t >= dropT0 && t < dropT0 + 0.55) { const k = pr(t, dropT0, dropT0 + 0.55); dx_ = Math.round(lerp(x + 14, x + 21, k)); dy_ = Math.round(y0 + 4 - Math.sin(k * Math.PI) * 5 + k * 18); }
  else if (t >= dropT0 + 0.55 && t < 4.75) { dx_ = x + 21; dy_ = 26; }
  // the mop-up: hop right onto the drop, slurp, hop back
  if (t >= 4.4 && t < 4.75) { const k = pr(t, 4.4, 4.75); sx = Math.round(k * 6); lift = hop(k, 1, 2); }
  else if (t >= 4.75 && t < 5.0) sx = 6;
  else if (t >= 5.0 && t < 5.5) { const k = pr(t, 5.0, 5.5); sx = Math.round((1 - k) * 6); lift = hop(k, 1, 2); }
  if (t >= 4.75) L = 4.3;
  if (dx_ != null) { if (dy_ < 26) put('DROP', dx_ - 1, dy_ - 2); else { set(dx_ - 1, 27, 2); set(dx_, 27, 2); set(dx_ + 1, 27, 2); set(dx_, 26, 2); } }
  const open = (t >= 1.0 && t < 2.6) || (t >= 4.7 && t < 4.95);
  const wph = Math.floor(t * 6);
  sloshBody(open ? 'TN_SLOSH_EAT' : 'TN_SLOSH', x + sx, y0 - lift, L, T, wph, { shut: t > 5.6 && t < 6.6 });
  if (t >= 4.75 && t < 4.95) spark(x + 16, y0 - 2);
  if (t > 6.0 && Math.floor(t * 5) % 2 === 0) spark(x + 16, y0);
}

// ---------------------------------------------------------------------------------------------
// MALLOW: a s'more. Hops on top of the food, a tiny campfire lights beside him and toasts him: he goes
// melty and wide, chocolate drips down over the food and the two melt together. Fire out, he plops down
// in a happy puddle and slowly puffs back up.
// ---------------------------------------------------------------------------------------------
function campfire(x, t, out) {
  // two crossed logs
  set(x, 27, 3); set(x + 4, 27, 3); set(x + 1, 26, 3); set(x + 3, 26, 3); set(x + 2, 27, 3);
  if (out) return;
  const f = Math.floor(t * 10) % 3;   // the flame flickers and leans toward him
  const shapes = [
    [[1, 25], [2, 25], [3, 25], [1, 24], [2, 24], [3, 24], [2, 23], [3, 23], [3, 22]],
    [[1, 25], [2, 25], [3, 25], [2, 24], [3, 24], [4, 24], [3, 23], [4, 23], [4, 22], [5, 21]],
    [[1, 25], [2, 25], [3, 25], [1, 24], [2, 24], [3, 24], [2, 23], [2, 22]]];
  for (const [a, b] of shapes[f]) set(x + a, b, 3);
  set(x + 2 + f, 21 - (f === 1 ? 1 : 0), 2);   // ember
}
function mallow(t) {
  const F = FD(), fcx = 28, fx0 = 14;
  let cx = 9, lift = 0, w = 12, h = 10, base = 28;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  // hop onto the food
  if (t >= 0.4 && t < 1.2) { const k = pr(t, 0.4, 1.2); cx = lerp(9, fcx, ez(k)); base = lerp(28, 24, ez(k)); lift = Math.round(Math.sin(k * Math.PI) * 6); }
  else if (t >= 1.2) { cx = fcx; base = 24; }
  if (t >= 1.2 && t < 1.35) { h = 9; w = 13; }
  const melt = t >= 1.9 && t < 4.3 ? ez(pr(t, 1.9, 3.8)) : 0;
  const st = t < 2.4 ? 0 : t < 3.1 ? 1 : t < 3.8 ? 2 : 3;
  if (t >= 0.4 && st < 3 && t < 4.3) food(fcx, 23, st, { mid: t >= 2.4 });
  if (melt) { h = Math.round(10 - melt * 3); w = Math.round(12 + melt * 4); }
  // plop down to the floor, a happy puddle, then slowly puffs back up
  if (t >= 4.3) { cx = fcx; const k = pr(t, 4.3, 4.5); base = Math.round(lerp(24, 28, k)); h = 7; w = 16; }
  if (t >= 4.9) { const k = ez(pr(t, 4.9, 6.0)); h = Math.round(lerp(7, 10, k)); w = Math.round(lerp(16, 12, k)); }
  const by = base - h - lift;
  warp('TN_MALLOW', Math.round(cx - w / 2), by, w, h, { shut: t >= 2.0 && t < 6.0 });
  // chocolate drips: down his face from the chocolate top, and on over the food underneath
  if (melt > 0.1 || (t >= 4.3 && t < 5.2)) {
    const len = t < 4.3 ? Math.round(melt * (h + 2)) : Math.max(0, Math.round((h - 2) * (1 - pr(t, 4.3, 5.2))));
    const lx = Math.round(cx - w / 2);
    for (const [c, f] of [[2, 1], [5, 0.7], [w - 4, 0.9]]) {
      const n = Math.round(len * f);
      for (let i = 0; i < n; i++) set(lx + c, by + 3 + i, 2);
      if (n > 1) { set(lx + c + 1, by + 2 + n, 2); set(lx + c, by + 2 + n, 2); }   // drop blob on the end
    }
  }
  // the campfire
  if (t >= 1.5 && t < 4.6) campfire(fx0, t, t >= 3.9);
  if (t >= 1.5 && t < 1.9 && Math.floor(t * 10) % 2 === 0) spark(fx0 - 4, 20);
  if (t >= 3.9 && t < 4.8) for (let i = 0; i < 3; i++) { const k = ((t - 3.9) * 1.4 + i * 0.3) % 1; set(fx0 + 2 + Math.round(Math.sin(k * 6 + i) * 1.5), Math.round(25 - k * 10), 2); }
  if (t >= 5.0 && t < 5.6) disc(Math.round(cx + 9), Math.round(20 - pr(t, 5.0, 5.6) * 6), 1, 2);   // contented sigh
  if (t > 6.0 && Math.floor(t * 5) % 2 === 0) spark(cx + 8, 16);
}

// ---------------------------------------------------------------------------------------------
// SPIKYBOI: his head is on a spring. It boings off his body across to the food, chomps three times,
// and twangs back. The lump drops into his belly, which inflates like a balloon... and lets go with a burp.
// ---------------------------------------------------------------------------------------------
function spiky(t) {
  if (!SP.SPIKY_HEAD) { SP.SPIKY_HEAD = SP.AD_DANGO.slice(0, 6); SP.SPIKY_HEADO = SP.AD_DANGO_EAT.slice(0, 6); SP.SPIKY_BODY = SP.AD_DANGO.slice(6); }
  const F = FD(), x0 = 4, fcx = 31;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  const st = t < 1.6 ? 0 : t < 2.0 ? 1 : t < 2.4 ? 2 : 3;
  if (t >= 0.4 && st < 3) food(fcx, 23, st);
  // body (rows 6..12): squashes, then inflates
  let bw = 13, bh = 7;
  if (t >= 0.4 && t < 0.8) bh = 6;
  if (t >= 3.6 && t < 5.0) { const k = ez(pr(t, 3.6, 4.4)); bw = Math.round(13 + k * 5 + Math.sin(t * 20) * 0.6 * k); bh = Math.round(7 + k * 2); }
  if (t >= 5.0 && t < 5.3) { const k = pr(t, 5.0, 5.3); bw = Math.round(18 - k * 5); bh = Math.round(9 - k * 2); }
  const bcx = x0 + 6.5;
  floorWarp('SPIKY_BODY', bcx, bw, bh, 0);
  const btop = 28 - bh;
  // head
  const restX = x0, restY = btop - 6;
  let hx = restX, hy = restY;
  const fx = fcx - 6.5, fy = 17;
  if (t >= 0.4 && t < 0.8) hy = restY;
  else if (t >= 0.8 && t < 1.4) { const k = ez(pr(t, 0.8, 1.4)); hx = lerp(restX, fx, k); hy = lerp(restY, fy, k) - Math.sin(k * Math.PI) * 7; }
  else if (t >= 1.4 && t < 2.6) { hx = fx; hy = fy + (((t - 1.4) % 0.4) < 0.2 ? 1 : 0); }
  else if (t >= 2.6 && t < 3.6) {   // twang back, overshoot and settle
    const k = pr(t, 2.6, 2.95); hx = lerp(fx, restX, ez(k)); const base_ = lerp(fy, restY, ez(k));
    hy = base_ - (t > 2.95 ? Math.round(6 * Math.exp(-(t - 2.95) * 6) * Math.cos((t - 2.95) * 18)) : Math.sin(k * Math.PI) * 4);
  }
  else hy = restY - (t >= 5.0 && t < 5.25 ? 2 : 0);
  hx = Math.round(hx); hy = Math.round(hy);
  // the spring
  const ax = bcx, ay = btop, bx = hx + 6.5, by = hy + 5;
  const n = Math.round(Math.hypot(bx - ax, by - ay));
  if (n > 2) for (let i = 0; i <= n; i++) {
    const k = i / n, px = lerp(ax, bx, k), py = lerp(ay, by, k), nx = -(by - ay) / n, ny = (bx - ax) / n, z = (i % 3 === 1 ? 1 : i % 3 === 2 ? -1 : 0);
    set(Math.round(px + nx * z), Math.round(py + ny * z), 3);
  }
  const chomp = t >= 1.4 && t < 2.6 && ((t - 1.4) % 0.4) < 0.2;
  const open = (t >= 1.25 && t < 1.4) || chomp || (t >= 5.0 && t < 5.4);
  put(open ? 'SPIKY_HEADO' : 'SPIKY_HEAD', hx, hy, { shut: t >= 5.4 && t < 6.2 });
  // crumbs
  if (chomp) { set(fcx + 5 + Math.round(rnd(Math.floor(t * 10)) * 2), 21, 3); set(fcx - 6, 22 - Math.round(rnd(Math.floor(t * 10) + 3) * 2), 3); }
  // the lump drops into his belly
  if (t >= 2.95 && t < 3.6) { const k = pr(t, 2.95, 3.6); disc(Math.round(bcx), Math.round(lerp(restY + 4, 24, k)), 1, 2); }
  if (t >= 3.6 && t < 5.0) disc(Math.round(bcx), 24, Math.round(1 + ez(pr(t, 3.6, 4.4))), 2);
  if (t >= 5.0 && t < 5.6) { const k = pr(t, 5.0, 5.6); ring(Math.round(restX + 14 + k * 4), Math.round(restY + 2 - k * 6), 1, 3); }   // burp
  if (t > 5.8 && Math.floor(t * 5) % 2 === 0) spark(x0 + 15, restY);
}

// ---------------------------------------------------------------------------------------------
// BLOBBERT: absorbs it... ah... ah... CHOO! Sneezes the meal right back out across the room. Blinks at it,
// charges, and absorbs it again properly, ending up bigger and very wobbly.
// ---------------------------------------------------------------------------------------------
function blobbert(t) {
  const F = FD(), fcx = 29, fx2 = 8;
  let cx = 10.5, w = 15, h = 12, shut = false;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.2) { const k = pr(t, 0.4, 1.2); cx = lerp(10.5, fcx, ez(k)); w = 15 + Math.round(Math.sin(k * Math.PI) * 3); h = 12 - Math.round(Math.sin(k * Math.PI) * 2); }
  else if (t >= 1.2 && t < 2.6) cx = fcx;
  if (t >= 0.4 && t < 1.2) food(fcx, 23, 0);
  // inside: rises to the middle
  if (t >= 1.2 && t < 2.6) { const k = ez(pr(t, 1.2, 1.9)); food(fcx, Math.round(lerp(23, 20, k)), t < 1.7 ? 0 : 1, { mid: true }); }
  // ah... ah...
  if (t >= 2.0 && t < 2.3) { w = 16; h = 13; }
  if (t >= 2.3 && t < 2.6) { w = 17; h = 14; shut = true; }
  // CHOO
  if (t >= 2.6 && t < 2.8) { w = 19; h = 9; cx = fcx + 2; shut = true; }
  if (t >= 2.8 && t < 3.6) { cx = fcx + Math.round(2 * (1 - pr(t, 2.8, 3.2))); w = 15 + Math.round(Math.sin((t - 2.8) * 20) * Math.exp(-(t - 2.8) * 4) * 2); }
  if (t >= 2.6 && t < 3.6) {   // the food flies out, bounces, lands whole
    const k = pr(t, 2.6, 3.15), k2 = pr(t, 3.15, 3.45);
    const fy = t < 3.15 ? 20 - Math.sin(k * Math.PI) * 6 + k * 3 : 23 - Math.sin(k2 * Math.PI) * 2;
    food(t < 3.15 ? lerp(fcx, fx2, k) : fx2, Math.round(fy), 0);
    if (t < 2.9) for (let i = 0; i < 4; i++) set(Math.round(fcx - 9 - i * 2 - (t - 2.6) * 20), 18 + (i % 3) * 2, 2);   // spray
  }
  if (t >= 3.3 && t < 3.42) shut = true;   // blink
  // charge!
  if (t >= 3.6 && t < 4.2) { const k = pr(t, 3.6, 4.2); cx = lerp(fcx, fx2, ez(k)); w = 19 - Math.round(Math.abs(k - 0.5) * 6); h = 10 + Math.round(Math.abs(k - 0.5) * 3); food(fx2, 23, 0); }
  // absorb again, properly
  if (t >= 4.2) {
    cx = fx2;
    const st = t < 4.6 ? 0 : t < 5.0 ? 1 : t < 5.3 ? 2 : 3;
    if (st < 3) food(fx2, 21, st, { mid: true });
    const g = ez(pr(t, 4.2, 5.3));
    w = 15 + g * 3; h = 12 + g * 2;
    if (t >= 5.3) { const k = t - 5.3, a = Math.exp(-k * 1.6); w = 17 + Math.sin(k * 13) * 2.4 * a; h = 13 - Math.sin(k * 13) * 1.6 * a; shut = k > 0.3 && k < 1.1; }
  }
  floorWarp('S0_IDLE', cx, Math.round(w), Math.round(h), 0, { shut });
  if (t >= 2.6 && t < 2.85) { put('SPARK', Math.round(cx - w / 2) - 5, 14); }
  if (t > 6.2 && Math.floor(t * 5) % 2 === 0) spark(Math.round(cx + 10), 13);
}

// ---------------------------------------------------------------------------------------------
// MOCHI: makes himself into a daifuku. He melts flat into a sheet, slides under the food and scoots it to
// the middle, folds his edges up and over it, and pinches shut on top with a little twist. Wobbles while the
// filling sinks in, then the twist pops undone and he's himself again.
// ---------------------------------------------------------------------------------------------
function flap(px, py, a, len, v) {   // a 2-dot-thick flap from a pivot at angle a (0 = right, up is positive)
  for (let i = 0; i <= len; i++) {
    const x = px + Math.cos(a) * i, y = py - Math.sin(a) * i;
    set(Math.round(x), Math.round(y), v);
    set(Math.round(x - Math.sin(a) * 1), Math.round(y - Math.cos(a) * 1), v);
  }
}
function mochi(t) {
  const F = FD(), fcx0 = 30, cx = 20;
  if (t < 0.4) food(fcx0, dropY(t, 0, 0.4, 23), 0);
  const eyes = (x0, y0) => { set(x0, y0, 3); set(x0 + 6, y0, 3); };
  if (t < 0.6) { put('S0B_IDLE', 4, 18); food(fcx0, 23, 0); return; }
  if (t < 1.8) {
    // melt flat and spread right, under the food; then gather in toward the middle carrying it
    let l, r;
    if (t < 1.2) { const k = ez(pr(t, 0.6, 1.2)); l = lerp(4, 2, k); r = lerp(16, 37, k); }
    else { const k = ez(pr(t, 1.3, 1.8)); l = lerp(2, cx - 7, k); r = lerp(37, cx + 7, k); }
    const h = t < 0.9 ? Math.round(lerp(10, 3, ez(pr(t, 0.6, 0.9)))) : 3;
    if (h > 3) warp('S0B_IDLE', Math.round(l), 28 - h, Math.round(r - l) + 1, h);
    else { for (let x = Math.round(l); x <= Math.round(r); x++) { set(x, 25, 3); set(x, 27, 3); } set(Math.round(l), 26, 3); set(Math.round(r), 26, 3); eyes(Math.round((l + r) / 2) - 3, 26); }
    const under = r > fcx0 - F.w / 2;
    const fx = t < 1.3 ? fcx0 : lerp(fcx0, cx, ez(pr(t, 1.3, 1.8)));
    food(fx, under ? 20 : 23, 0);
    return;
  }
  if (t < 2.6) {
    // fold the edges up and over
    const k = ez(pr(t, 1.85, 2.5)), len = 8;
    for (let x = cx - 7; x <= cx + 7; x++) set(x, 27, 3);
    set(cx - 7, 26, 3); set(cx + 7, 26, 3);
    food(cx, 21, 0);
    flap(cx - 7, 25, lerp(Math.PI, Math.PI / 3, k), len, 3);
    flap(cx + 7, 25, lerp(0, Math.PI * 2 / 3, k), len, 3);
    eyes(cx - 3, 26);
    return;
  }
  // the daifuku: round, a twist on top, filling sinking in
  let w = 15, h = 12, shut = t >= 3.0 && t < 4.4, lift = 0;
  if (t < 2.75) { w = 17; h = 10; }
  else if (t < 4.4) { const k = t - 2.75; w = 15 + Math.round(Math.sin(k * 12) * 1.2 * Math.exp(-k * 0.8)); h = 12 - Math.round(Math.sin(k * 12) * 0.8 * Math.exp(-k * 0.8)); }
  const st = t < 3.3 ? 0 : t < 3.8 ? 1 : t < 4.3 ? 2 : 3;
  const bx = Math.round(cx - w / 2), by = 28 - h;
  ring(cx, by + 6, 0, 0);
  for (let y = 0; y < h; y++) for (let x = 0; x < w; x++) {   // a round bun outline
    const nx = (x - (w - 1) / 2) / ((w - 1) / 2), ny = (y - (h - 1) / 2) / ((h - 1) / 2), d = nx * nx + ny * ny;
    const nx2 = (Math.abs(x - (w - 1) / 2) + 1) / ((w - 1) / 2), ny2 = (Math.abs(y - (h - 1) / 2) + 1) / ((h - 1) / 2);
    if (d <= 1 && (nx2 * nx2 + ny * ny > 1 || nx * nx + ny2 * ny2 > 1)) set(bx + x, by + y, 3);
  }
  if (st < 3) food(cx, by + 4, st, { mid: true });
  if (!shut) eyes(cx - 3, by + 4); else { set(cx - 4, by + 4, 3); set(cx - 2, by + 4, 3); set(cx + 2, by + 4, 3); set(cx + 4, by + 4, 3); }
  set(cx, by + 7, 3);   // mouth
  // the twist on top, then it pops undone
  if (t < 4.4) { set(cx, by - 1, 3); set(cx - 1, by - 2, 3); set(cx + 1, by - 2, 3); set(cx, by - 3, 3); }
  if (t >= 4.4 && t < 4.8) { const k = pr(t, 4.4, 4.8); set(cx - 1 - Math.round(k * 4), by - 2 - Math.round(k * 5), 3); set(cx + 1 + Math.round(k * 4), by - 2 - Math.round(k * 5), 3); }
  if (t >= 4.4) {   // back to himself
    clr(); lift = t < 4.7 ? hop(pr(t, 4.4, 4.7), 1, 3) : 0;
    put('S0B_IDLE', cx - 6, 18 - lift, { shut: t > 5.0 && t < 5.6 });
    if (t < 4.8) { const k = pr(t, 4.4, 4.8); set(cx - 3 - Math.round(k * 4), 15 - Math.round(k * 5), 3); set(cx + 3 + Math.round(k * 4), 15 - Math.round(k * 5), 3); }
    if (t > 5.0 && Math.floor(t * 5) % 2 === 0) spark(cx + 8, 15);
  }
}

// ---------------------------------------------------------------------------------------------
// CHEEKS: nibbles at chipmunk speed, crumbs flying, stuffing it all into his cheeks... which swell so big
// he floats off the ground like a balloon. GULP: the cheeks go, and he drops back down. Bonk.
// ---------------------------------------------------------------------------------------------
function cheeks(t) {
  const F = FD(), fcx = 28, y0 = 18;
  let x = 3, lift = 0, sq = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  if (t >= 0.4 && t < 1.0) { const k = pr(t, 0.4, 1.0); x = Math.round(lerp(3, 12, ez(k))); lift = hop(k, 2, 2); }
  else if (t >= 1.0) x = 12;
  const st = t < 1.6 ? 0 : t < 2.2 ? 1 : t < 2.8 ? 2 : 3;
  if (t >= 0.4 && st < 3) food(fcx, 23, st);
  const nib = t >= 1.0 && t < 2.8;
  let r = 0;
  if (t >= 1.0 && t < 4.2) r = Math.min(4, 0.5 + pr(t, 1.0, 2.8) * 3.5);
  if (t >= 4.2 && t < 4.35) r = 4 * (1 - pr(t, 4.2, 4.35));
  // floating
  if (t >= 2.8 && t < 4.2) lift = Math.round(ez(pr(t, 2.8, 3.8)) * 9 + Math.sin(t * 5) * 1);
  if (t >= 4.2 && t < 4.5) { const k = pr(t, 4.2, 4.5); lift = Math.round(9 * (1 - k * k)); }
  if (t >= 4.5 && t < 4.65) sq = 1;
  const top = 28 - 10 - lift;
  const shake = nib ? (Math.floor(t * 24) % 2) : 0;
  const shut = t >= 4.7 && t < 5.6;
  if (sq) floorWarp('S0C_IDLE', x + 5.5, 13, 8, 0, { shut }); else put('S0C_IDLE', x + shake, top, { shut });
  if (nib && Math.floor(t * 12) % 2 === 0) { set(x + 4 + shake, top + 8, 3); set(x + 6 + shake, top + 8, 3); set(x + 5 + shake, top + 9, 0); }   // nom nom nom
  if (nib) for (let i = 0; i < 2; i++) { const k = ((t * 3 + i * 0.5) % 1); set(Math.round(fcx - 4 + (i ? 10 : -3) * k), Math.round(22 - Math.sin(k * Math.PI) * 4 + k * 4), 3); }   // crumbs
  // the cheek pouches
  const R = Math.round(r);
  if (R > 0 && !sq) { const o = Math.round(R * 0.6); ring(x + 1 - o + shake, top + 6, R, 3, 0); ring(x + 9 + o + shake, top + 6, R, 3, 0); }
  // little feet dangling
  if (lift > 1 && t < 4.2) { set(x + 3, top + 10, 3); set(x + 7, top + 10 + (Math.floor(t * 6) % 2), 3); }
  if (t >= 4.2 && t < 4.45) spark(x + 12, top - 2);   // GULP
  if (t >= 4.6 && t < 5.0) { const k = pr(t, 4.6, 5.0); set(x + 2 + Math.round(k * 7), 13 - Math.round(Math.sin(k * Math.PI) * 2), 3); }   // dizzy star
  if (t > 5.4 && Math.floor(t * 5) % 2 === 0) spark(x + 12, y0 - 2);
}

// ---------------------------------------------------------------------------------------------
// BIG BLOBBERT: fills the screen. Crouches like a trampoline, the food lands on his head, he BOINGS it up,
// catches it, and it sinks slowly down through him while his eyes follow it. Ripples, then a burp that
// shakes the whole screen.
// ---------------------------------------------------------------------------------------------
function bigBlobbert(t) {
  const F = FD(), x = 2, cx = 20;
  let h = 25, w = 36, fy = null, mid = false, st = 0, ey = 0, shake = 0, shut = false;
  if (t < 0.5) h = Math.round(lerp(25, 21, ez(pr(t, 0, 0.5))));
  else if (t < 1.0) h = 21;
  else if (t < 1.3) h = Math.round(lerp(21, 24, pr(t, 1.0, 1.15)));
  else if (t < 1.7) h = t < 1.6 ? 24 : 22;
  else if (t < 3.8) h = 22;
  if (t >= 3.8 && t < 4.6) { const k = t - 3.8; h = Math.round(22 + Math.sin(k * 15) * 2 * Math.exp(-k * 3)); w = Math.round(36 - Math.sin(k * 15) * 1.5 * Math.exp(-k * 3)); }
  if (t >= 4.6 && t < 5.0) { h = 24; w = 37; }
  if (t >= 5.0) { h = 22; w = 36; }
  if (t >= 5.0 && t < 5.5) shake = Math.floor(t * 20) % 2 ? 1 : -1;
  const top = 28 - h;
  // the food
  if (t >= 0.3 && t < 1.0) fy = dropY(t, 0.3, 0.6, 21 - 4) ;
  if (t >= 0.6 && t < 1.0) fy = 28 - 21 - 4;
  if (t >= 1.0 && t < 1.6) { const k = pr(t, 1.0, 1.6); fy = Math.round(lerp(3, 28 - 24 - 4, k) - Math.sin(k * Math.PI) * 9); }
  if (t >= 1.6 && t < 3.8) { const k = pr(t, 1.7, 3.8); fy = Math.round(lerp(top - 4, 18, k)); mid = fy > top - 1; st = t < 2.7 ? 0 : t < 3.3 ? 1 : 2; }
  if (fy != null) { ey = Math.max(-2, Math.min(2, Math.round((fy - 9) / 4))); }
  if (t < 0.3) ey = -2;
  if (t >= 5.6) shut = true;
  const bx = Math.round(cx + 0.5 - w / 2);
  warp('BB_BLOBBERT', bx, top, w, h, { shut: true });
  const eyy = top + Math.round(10 * (h - 1) / 24);
  for (const ex of [bx + Math.round(11 * (w - 1) / 35), bx + Math.round(24 * (w - 1) / 35)]) {
    if (shut) { set(ex - 1, eyy + 2, 3); set(ex, eyy + 1, 3); set(ex + 1, eyy + 2, 3); }
    else for (let i = 0; i < 3; i++) set(ex, eyy + i + ey, 3);
  }
  if (fy != null) food(cx, fy, st, { mid });
  if (t >= 1.0 && t < 1.2) { spark(bx + 4, top - 2); spark(bx + w - 9, top - 2); }   // BOING
  // burp bubble
  if (t >= 5.0 && t < 6.4) { const k = pr(t, 5.0, 6.4); const by = Math.round(top - 2 - k * 10); if (by > -3) put('BUBBLE', cx - 1 + Math.round(Math.sin(k * 8)), by); }
  if (t > 6.0 && Math.floor(t * 5) % 2 === 0) spark(bx + w - 7, top + 1);
  shiftBuf(shake);
}

// ---------------------------------------------------------------------------------------------
// BIG BALLBOI: rolls right over the food and squashes it flat: it sticks to his ball. He rolls back and the
// pancake rides round the shell to the top, peels off, drops in through the hatch, bonks him on the head
// and slides into his mouth.
// ---------------------------------------------------------------------------------------------
function bigShell(cx, cy, ang, hatch) {
  const R = 13;
  for (let dy = -R - 1; dy <= R + 1; dy++) for (let dx = -R - 1; dx <= R + 1; dx++) {
    const r = Math.hypot(dx, dy); if (r < R - 0.5 || r >= R + 0.5) continue;
    const th = Math.atan2(dy, dx);
    const near = a => Math.abs(((th - a) % (2 * Math.PI) + 3 * Math.PI) % (2 * Math.PI) - Math.PI);
    if (hatch != null && near(hatch) < 0.24) continue;
    set(cx + dx, cy + dy, 3);
  }
  // rotation marks: two little dark ticks on the inside of the rim, opposite each other, spinning with the shell
  for (const o of [0, Math.PI]) for (const rr of [11, 12]) set(Math.round(cx + Math.cos(ang + o) * rr), Math.round(cy + Math.sin(ang + o) * rr), 3);
  // the glare stays put (it's a reflection)
  for (const [a, rr] of [[-2.45, 11], [-2.3, 11], [-2.15, 11], [-2.4, 10]]) set(Math.round(cx + Math.cos(a) * rr), Math.round(cy + Math.sin(a) * rr), 2);
}
function bigBallboi(t) {
  // one clear story: roll right onto the food (squash, it sticks to the bottom of the shell), roll back left
  // exactly half a turn so the pancake rides up the back of the ball to the very top, sits there a beat,
  // the hatch opens under it and it drops straight down onto his head, then into his mouth.
  const F = FD(), fcx = 26, cy = 14, R = 13, M = 1.6;
  let cx = 13, pan = null, lean = 0, bob = 0, open = false;
  const xs = [13, 26, 13];
  if (t >= 0.5 && t < 1.6) cx = lerp(xs[0], xs[1], ez(pr(t, 0.5, 1.6)));
  else if (t >= 1.6 && t < 2.0) cx = xs[1];
  else if (t >= 2.0 && t < 3.5) { const k = pr(t, 2.0, 3.5); cx = lerp(xs[1], xs[2], k < 0.8 ? k * 0.9 / 0.8 : 0.9 + (1 - Math.pow(1 - (k - 0.8) / 0.2, 2)) * 0.1); }
  else if (t >= 3.5) cx = xs[2];
  const M2 = Math.PI * R / (xs[1] - xs[2]);            // half a turn on the way back
  const dR = t >= 1.6 ? xs[1] - xs[0] : cx - xs[0], dL = t >= 2.0 ? xs[1] - cx : 0;
  const angMax = (xs[1] - xs[0]) / R * M;
  const ang = dR / R * M - dL / R * M2;
  const moving = (t >= 0.5 && t < 1.6) || (t >= 2.0 && t < 3.5);
  if (moving) { lean = t < 1.6 ? 1 : -1; bob = Math.floor(t * 10) % 2 ? -1 : 0; }
  // the food, then the pancake stuck to the shell
  const squashAt = 1.45, peel = 3.75;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  else if (t < squashAt) food(fcx, 23, 0);
  else if (t < 1.6) { const k = pr(t, squashAt, 1.6); warp(F.n[0], fcx - 4, Math.round(lerp(23, 26, k)), F.w, Math.max(1, Math.round(lerp(F.h, 1, k)))); }   // squashing flat
  if (t >= 1.6 && t < peel) pan = Math.PI / 2 + (ang - angMax);
  const hatch = t >= 3.6 && t < peel + 0.5 ? -Math.PI / 2 : null;
  bigShell(Math.round(cx), cy, ang, hatch);
  if (pan != null) {   // a fat grey pancake pressed onto the rim (stays on screen even at the very top)
    for (let i = -3; i <= 3; i++) { const a = pan + i / 13; set(Math.round(cx + Math.cos(a) * 13), Math.round(cy + Math.sin(a) * 13), 2); }
    for (let i = -2; i <= 2; i++) { const a = pan + i / 14; set(Math.round(cx + Math.cos(a) * 14), Math.round(cy + Math.sin(a) * 14), 3); }
  }
  // the hamster
  const hx = Math.round(cx) - 8 + lean, hy = 9 + bob;
  if (t >= 1.6 && t < 2.0) { set(hx + 5, hy + 8, 3); set(hx + 11, hy + 8, 3); }   // looks down at it
  put('BB_BALL_HAM', hx, hy, { shut: t >= 5.8 && t < 6.8 });
  // in through the hatch, bonk, and down into his mouth
  if (t >= peel && t < 4.05) { const k = pr(t, peel, 4.05); warp(F.n[0], Math.round(cx) - 2, Math.round(lerp(1, hy - 2, k * k)), 5, 2, {}); }
  if (t >= 4.05 && t < 4.3) { warp(F.n[0], hx + 6, hy - 2, 5, 2, {}); spark(hx + 13, hy - 4); }
  if (t >= 4.3 && t < 4.6) { const k = pr(t, 4.3, 4.6); warp(F.n[0], hx + 6, Math.round(lerp(hy - 1, hy + 9, k)), 5, 2, {}); open = true; }
  const chew = t >= 4.6 && t < 5.8, ck = (t - 4.6) / 0.4;
  if (chew) { const cst = Math.min(2, Math.floor(ck)); if (cst < 2 && (ck % 1) < 0.5) warp(F.n[cst + 1], hx + 7, hy + 10, 3, 2, { mid: true }); open = (ck % 1) < 0.5; }
  if (open) { set(hx + 7, hy + 11, 3); set(hx + 9, hy + 11, 3); set(hx + 8, hy + 12, 3); }
  if (t > 6.2 && Math.floor(t * 5) % 2 === 0) spark(Math.round(cx) + 14, 3);
}

// ---------------------------------------------------------------------------------------------
// ALIEN CAT IN A TUXEDO: his eyes glow and a tractor beam lifts the food up to his mouth. Three bites, a
// hiccup spins his bow tie, a paw straightens it (ding), and he burps out a tiny spiral galaxy.
// ---------------------------------------------------------------------------------------------
function alien(t) {
  const F = FD(), x = 3, y = 3, fcx = 31;
  const glow = t >= 0.7 && t < 3.9;
  // eyes: drawn by hand so they can glow
  put('BB_ALIEN', x, y, { noE: true });
  const R = SP.BB_ALIEN;
  const blink = t >= 5.0 && t < 5.15;
  for (let r = 0; r < R.length; r++) for (let c = 0; c < 17; c++) if (R[r][c] === 'E') {
    if (blink) continue;
    const v = glow ? (Math.floor(t * 8 + c + r) % 3 === 0 ? 2 : 3) : 3; set(x + c, y + r, v);
  }
  if (blink) { for (const ex of [4, 10]) for (let i = 0; i < 4; i++) set(x + ex + i, y + 10, 3); }
  // the food
  let fx = fcx, fy = 23, st = 0;
  if (t < 0.4) fy = dropY(t, 0, 0.4, 23);
  if (t >= 1.2 && t < 2.6) { const k = ez(pr(t, 1.2, 2.6)); fx = lerp(fcx, x + 22, k); fy = lerp(23, y + 10, k) + Math.sin(t * 9) * 0.8; }
  const bites = [2.8, 3.25, 3.7];
  if (t >= 2.6) { st = stepIn(t, bites); fx = x + 22 - st * 1.5; fy = y + 10; }
  if (st < 3) food(Math.round(fx), Math.round(fy), Math.min(2, st));
  // tractor beam
  if (glow && t >= 1.0 && st < 3) {
    for (const ex of [x + 5, x + 12]) {
      const ey = y + 9.5, tx = fx, ty = fy + 2, n = Math.round(Math.hypot(tx - ex, ty - ey));
      for (let i = 2; i < n - 3; i++) if ((i + Math.floor(t * 12)) % 3 !== 0) set(Math.round(lerp(ex, tx, i / n)), Math.round(lerp(ey, ty, i / n)), 2);
    }
  }
  // chomping: mouth opens, the food is reeled in
  const open = st < 3 && t >= 2.6 && bites.some(b => t >= b - 0.2 && t < b);
  if (open || (t >= 5.2 && t < 5.5)) { set(x + 7, y + 14, 3); set(x + 8, y + 14, 0); set(x + 9, y + 14, 3); set(x + 8, y + 15, 3); set(x + 7, y + 13, 0); set(x + 9, y + 13, 0); }
  // the bow tie (drawn grey on his black tux)
  const bt = [[[-1, -1], [-1, 0], [-1, 1], [0, 0], [1, -1], [1, 0], [1, 1]], [[-1, -1], [0, -1], [1, -1], [0, 0], [-1, 1], [0, 1], [1, 1]], [[-1, 0], [-2, 1], [0, 0], [1, -1], [1, 0], [2, -1], [-1, 1]]];
  let bi = 0;
  if (t >= 3.95 && t < 4.4) bi = Math.floor(t * 14) % 2;
  else if (t >= 4.4 && t < 4.95) bi = 2;   // crooked
  for (const [a, b] of bt[bi]) set(x + 8 + a, y + 16 + b, 2);
  if (t >= 3.9 && t < 4.05) spark(x + 15, y + 12);   // hic!
  // the paw comes up and straightens it
  if (t >= 4.5 && t < 5.1) { const k = pr(t, 4.5, 4.8), k2 = pr(t, 4.85, 5.1); const px = Math.round(lerp(x + 2, x + 6, k) - k2 * 4), py = Math.round(lerp(y + 21, y + 17, k) + k2 * 4); rect(px, py, 2, 2, 0); set(px - 1, py, 3); set(px + 2, py, 3); }
  if (t >= 4.9 && t < 5.15) spark(x + 12, y + 14);   // ding
  // the tiny galaxy
  if (t >= 5.3 && t < 6.8) {
    const k = pr(t, 5.3, 6.8), gx = lerp(x + 10, x + 26, k), gy = lerp(y + 14, y + 3, k), rot = t * 5, rr = Math.min(4, 1 + k * 6);
    set(Math.round(gx), Math.round(gy), 3);
    for (let i = 0; i < 8; i++) { const a = rot + i * 0.8, r2 = rr * (0.35 + i / 10); set(Math.round(gx + Math.cos(a) * r2), Math.round(gy + Math.sin(a) * r2 * 0.6), i % 2 ? 2 : 3); }
    for (let i = 0; i < 4; i++) if (Math.floor(t * 6 + i) % 3 === 0) set(Math.round(gx + (rnd(i) - 0.5) * 16), Math.round(gy + (rnd(i + 9) - 0.5) * 10), 3);
  }
}

const ANIMS = [
  { id: 'wobbles', name: 'Wobbles', stage: 'Lil Blunkus', spr: 'TN_WOBBLES', dur: 7.6, fn: wobbles, tag: 'The slinky',
    blurb: 'Stretches up like a slinky, arches over into an upside-down U, clamps the food with the top of his head. It travels the whole U and dissolves in his feet. Boing back.' },
  { id: 'jelly', name: 'Jelly', stage: 'Lil Fella', spr: 'AD_JELLY', dur: 7, fn: jelly, tag: 'Belly flop',
    blurb: 'Hops over, belly-flops on the food, springs up with it inside his see-through body. It floats up, fizzles out, and he burps a bubble.' },
  { id: 'puddle', name: 'Puddle', stage: 'Lil Fella', spr: 'AD_PUDDLE', dur: 6.6, fn: puddle, tag: 'Flow & glug',
    blurb: 'Flows flat across the food (you can see it inside him), three gulps with bubbles, bunches into a tall dome, burps, slumps back.' },
  { id: 'toasty', name: 'Toasty', stage: 'Lil Blunkus', spr: 'TN_TOASTY', dur: 6.6, fn: toasty, tag: 'Toaster sandwich',
    blurb: 'Pops his top crust up like a toaster (DING), eyes riding along on it. The fish hops off the floor and slides into the middle, the crust comes down: a fish sandwich. His mouth chews below.' },
  { id: 'marshy', name: 'Marshy', stage: 'Lil Fella', spr: 'AD_MARSHY', dur: 6.2, fn: marshy, tag: 'Pinball', random: 'marshy',
    blurb: 'The food falls from the sky, bounces off his head, bonks the side wall and ricochets straight into his open mouth. Three squishy marshmallow chews.' },
  { id: 'boop', name: 'Boop', stage: 'Lil Fella', spr: 'AD_BOOP', dur: 7.2, fn: boop, tag: 'Potion flask',
    blurb: 'Cork pops off, the food is flicked up and plops down the neck. The "liquid" rises and fizzes as it dissolves. Cork pops back on with a steam puff.' },
  { id: 'curly', name: 'Curly', stage: 'Lil Blunkus', spr: 'TN_CURL', dur: 6.8, fn: curly, tag: 'Teapot',
    blurb: 'The lid hops, the food drops in. He heats up, shaking, whistling steam out of the spout, lid rattling, then one big steam blast.' },
  { id: 'puff', name: 'Puff', stage: 'Lil Fella', spr: 'AD_PUFF', dur: 6.6, fn: puff, tag: 'Vacuum & balloon',
    blurb: 'Inhales the food across the floor with suction lines while puffing up like a balloon. A squeaky hiccup, then he leaks air, whizzing puffs, drifting down.' },
  { id: 'loaf', name: 'Loaf', stage: 'Lil Fella', spr: 'AD_LOAF', dur: 7, fn: loaf, tag: 'Sploot & rise',
    blurb: 'Butt wiggle, pounce, sploot over the food. A lump travels down his body from head to tail, he rises like proving bread, then slumps back.' },
  { id: 'nimbus', name: 'Nimbus', stage: 'Lil Fella', spr: 'AD_NIMBUS', dur: 7.4, fn: nimbus, tag: 'Thunderstorm',
    blurb: 'Drifts over the food and vortexes it up into his belly. He goes dark and grumbly, lightning cracks, it pours, then he clears and a rainbow appears.' },
  { id: 'ballboi', name: 'Ballboi', stage: 'Lil Fella', spr: 'AD_BALLBOI', dur: 8.4, fn: ballboi, tag: 'Roll & stuff', fresh: true,
    blurb: 'Zoomed out, he runs in from off-screen inside his hamster ball: the shell spins twice round him while he stays at the bottom, upright, bobbing as he runs and leaning into the front. Skids to a stop, then zooms back in and stuffs his cheeks one gulp at a time. Chews, eyes shut, then ptooey: a fish skeleton flies out.' },
  { id: 'sprout', name: 'Sprout', stage: 'Lil Blunkus', spr: 'TN_SPROUT', dur: 7.2, fn: sprout, tag: 'Photosynthesis',
    blurb: 'Hops onto the food, which fizzles into glowing dots that travel up his stem. The fronds shoot up, a flower blooms, sways, then pops into drifting petals.' },
  { id: 'fubz', name: 'Fubz', stage: 'Lil Fella', spr: 'AD_FUBZ', dur: 7, fn: fubz, tag: 'Hairball',
    blurb: 'Three polite bites... then the hack. He heaves and a fuzzy hairball rolls out and tumbles away. Very relieved.' },
  { id: 'dapper', name: 'Dapper', stage: 'Lil Fella', spr: 'S1C_IDLE', dur: 7.2, fn: dapper, tag: 'Fine dining',
    blurb: 'A floating knife and fork cut the meal (tink!). The fork delivers a bite, he chews with eyes shut, another bite, then dabs his mouth with a napkin.' },
  // ----- round 2: the rest of the cast -----
  { id: 'bean', name: 'Bean', stage: 'Lil Blunkus', spr: 'TN_BEAN', dur: 6.4, fn: bean, tag: 'Jumping bean', group: 2,
    blurb: 'Swallows the meal whole. Uh-oh: it rattles around inside him and he can\'t stop hopping. Three wild, tilting hops with the lump bouncing about, then a little "pop" and calm.' },
  { id: 'bun', name: 'Bun', stage: 'Lil Blunkus', spr: 'TN_BUN', dur: 6.2, fn: bun, tag: 'Hot-dog bun', group: 2,
    blurb: 'Splits wide open down the middle (an eye on each half). The food does a flip into the gap, he claps shut with a SQUISH and puffs steam while he chews.' },
  { id: 'slosh', name: 'Slosh', stage: 'Lil Blunkus', spr: 'TN_SLOSH', dur: 6.8, fn: slosh, tag: 'Slurp & spill', group: 2,
    blurb: 'Slurps the food up as a stream of dots and his liquid level rises. Sloshes side to side, a drop splashes over the rim... he hops over and slurps that up too.' },
  { id: 'mallow', name: 'Mallow', stage: 'Lil Blunkus', spr: 'TN_MALLOW', dur: 6.6, fn: mallow, tag: 'Campfire s\'more', group: 2,
    blurb: 'Hops on top of the food. A tiny campfire lights and toasts him: he goes melty and wide, chocolate drips over the food and they melt together. Fire out, he plops into a happy puddle and slowly puffs back up.' },
  { id: 'spiky', name: 'SpikyBoi', stage: 'Lil Fella', spr: 'AD_DANGO', dur: 6.4, fn: spiky, tag: 'Spring neck', group: 2,
    blurb: 'His head boings off on a spring, across to the food, chomps three times and twangs back. The lump drops into his belly, which blows up like a balloon... and lets go with a burp.' },
  { id: 'blobbert', name: 'Blobbert', stage: 'Lil Blub', spr: 'S0_IDLE', dur: 6.8, fn: blobbert, tag: 'Ah... CHOO!', group: 2,
    blurb: 'Absorbs it... ah... ah... CHOO! Sneezes the meal right back out across the room. Blinks at it, charges, absorbs it again properly, and ends up bigger and very wobbly.' },
  { id: 'mochi', name: 'Mochi', stage: 'Lil Blub', spr: 'S0B_IDLE', dur: 5.8, fn: mochi, tag: 'Daifuku wrap', group: 2, fresh: true,
    blurb: 'Turns himself into a daifuku: melts flat into a sheet, slides under the food and scoots it to the middle, folds his edges up and over it and pinches shut with a twist on top. Wobbles while the filling sinks in, then the twist pops undone and he\'s himself again.' },
  { id: 'cheeks', name: 'Cheeks', stage: 'Lil Blub', spr: 'S0C_IDLE', dur: 6.2, fn: cheeks, tag: 'Balloon cheeks', group: 2,
    blurb: 'Nibbles at chipmunk speed, crumbs flying, stuffing it all in his cheeks... which swell so big he floats off the ground, feet dangling. GULP: the cheeks go and he drops back down. Bonk.' },
  { id: 'bigblobbert', name: 'Big Blobbert', stage: 'Big Blorbus', spr: 'BB_BLOBBERT', dur: 6.8, fn: bigBlobbert, tag: 'Trampoline', group: 2,
    blurb: 'Fills the screen. Crouches like a trampoline, the food lands on his head, he BOINGS it up and catches it. It sinks slowly down through him while his eyes follow it. Ripples, then a burp that shakes the screen.' },
  { id: 'bigballboi', name: 'Big Ballboi', stage: 'Big Blorbus', spr: 'BB_BALLBOI', dur: 7.0, fresh: true, fn: bigBallboi, tag: 'Pancake express', group: 2,
    blurb: 'Rolls over the food and squashes it flat: it sticks to his ball. He rolls back exactly half a turn, so you can watch the pancake ride up the back of the ball to the very top. A hatch opens under it and it drops straight in, bonks him on the head and slides into his mouth.' },
  { id: 'alien', name: 'Alien Cat', stage: 'Big Blorbus', spr: 'BB_ALIEN', dur: 6.9, fn: alien, tag: 'Tractor beam', group: 2,
    blurb: 'His eyes glow and a tractor beam lifts the food to his mouth. Three bites, a hiccup spins his bow tie, a paw straightens it (ding), and he burps out a tiny spiral galaxy.' },
];
function render(a, t) { clr(); a.fn(t); }
function restart(a) { if (a.random === 'marshy') MARSHY.v = Math.random() < 0.5 ? 0 : 1; }
