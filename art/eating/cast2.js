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
    const side = k > 0.25 && k < 0.85;
    if (side) warp(F.n[0], Math.round(cx - 2), Math.round(cy - 2), 3, F.w, {}); else food(cx, Math.round(cy), 0);
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
  // logs
  set(x, 27, 3); set(x + 1, 27, 3); set(x + 3, 27, 3); set(x + 4, 27, 3); set(x + 1, 26, 3); set(x + 2, 26, 3); set(x + 3, 26, 3);
  if (out) return;
  const f = Math.floor(t * 9) % 3;
  const shapes = [[[2, 25], [1, 24], [2, 24], [3, 24], [2, 23], [3, 22]], [[2, 25], [1, 24], [2, 24], [3, 24], [3, 23], [4, 22], [4, 21]], [[2, 25], [2, 24], [3, 24], [1, 23], [2, 23], [2, 22]]];
  for (const [a, b] of shapes[f]) set(x + a, b, 3);
  set(x + 2 + (f === 1 ? 2 : 0), 20 + f, 2);   // spark
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
  // chocolate drips
  if (melt > 0.15 || (t >= 4.3 && t < 5.4)) {
    const len = t < 4.3 ? Math.round(melt * 6) : Math.max(0, 6 - Math.round((t - 4.3) * 6));
    const lx = Math.round(cx - w / 2), rx = lx + w - 1;
    for (let i = 0; i < len; i++) { set(lx, by + 3 + i, 2); set(rx, by + 3 + i, 2); if (i < len - 2) set(lx + 4, by + h + i - 1, 2); }
    if (t < 4.3 && Math.floor(t * 4) % 2 === 0) set(rx, Math.min(27, by + 3 + len + 1), 2);
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
// MOCHI: stretchy rice cake. His right side gloops out in a long gooey pull (his face stretched across the
// room), grabs the food, and SNAPS back with it stuck to his belly. It slowly sinks in.
// ---------------------------------------------------------------------------------------------
function mochi(t) {
  const F = FD(), fcx = 31, y0 = 18;
  let x = 2, s = 0;
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  const reach = fcx + 2 - (x + 12);   // right cap reaches the far side of the food
  if (t >= 0.6 && t < 1.6) { const k = pr(t, 0.6, 1.6); s = reach * (1 - Math.pow(1 - k, 2)) + Math.sin(k * 14) * (1 - k) * 1.2; }
  else if (t >= 1.6 && t < 1.95) s = reach + (t < 1.75 ? 0 : -1);
  else if (t >= 1.95 && t < 2.15) s = lerp(reach, -3, pr(t, 1.95, 2.15));
  else if (t >= 2.15 && t < 2.5) s = lerp(-3, 0, pr(t, 2.15, 2.5));
  s = Math.round(s);
  // momentum: thumps back into the wall
  if (t >= 2.1 && t < 2.5) x = 2 - Math.round(Math.sin(pr(t, 2.1, 2.5) * Math.PI) * 2);
  // the food
  if (t >= 0.4 && t < 1.6) food(fcx, 23, 0);
  else if (t >= 1.6 && t < 2.15) food(x + 9 + s + 1, 23, 0);
  const sinkSt = t < 2.9 ? 0 : t < 3.5 ? 1 : t < 4.1 ? 2 : 3;
  if (t >= 2.15 && sinkSt < 3) { const k = pr(t, 2.15, 4.1); food(Math.round(lerp(x + 10, x + 6.5, k)), Math.round(lerp(23, y0 + 3, k)), sinkSt, { mid: t > 2.5 }); }
  // jiggle while it sinks in
  let jig = 0; if (t >= 2.5 && t < 4.6) jig = Math.sin((t - 2.5) * 16) > 0.6 ? 1 : 0;
  const shut = (t >= 1.75 && t < 2.5) || (t >= 4.4 && t < 5.4);
  if (s <= 1 && s >= -1 && !jig) put('S0B_IDLE', x, y0, { shut, cmin: 0, cmax: 12 });
  else if (jig) floorWarp('S0B_IDLE', x + 6.5, 14, 9, 0, { shut });
  else {
    put('S0B_IDLE', x, y0, { shut, cmin: 0, cmax: 6 });
    put('S0B_IDLE', x + s, y0, { shut, cmin: 7, cmax: 12 });
    // the gooey band, thinner in the middle
    const n = Math.max(0, s);
    for (let i = 0; i < n; i++) {
      const sag = Math.round(Math.sin(Math.PI * (i + 0.5) / n) * Math.min(3, n / 6));
      set(x + 7 + i, y0 + 2 + sag, 3); set(x + 7 + i, y0 + 9 - sag, 3);
    }
  }
  if (t >= 1.95 && t < 2.15) { set(x + 15, y0 + 2, 3); set(x + 16, y0 + 5, 3); set(x + 15, y0 + 8, 3); }   // snap lines
  if (t > 5.0 && Math.floor(t * 5) % 2 === 0) spark(x + 14, y0 - 2);
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
    const seam = near(ang) < 0.16 || near(ang + Math.PI) < 0.16;
    set(cx + dx, cy + dy, seam ? 2 : 3);
  }
  // the glare stays put (it's a reflection)
  for (const [a, rr] of [[-2.45, 11], [-2.3, 11], [-2.15, 11], [-2.4, 10]]) set(Math.round(cx + Math.cos(a) * rr), Math.round(cy + Math.sin(a) * rr), 2);
}
function bigBallboi(t) {
  const F = FD(), fcx = 30, cy = 14, R = 13, M = 2.2;
  let cx = 13, ang = 0, pan = null, lean = 0, bob = 0, open = false;
  const xs = [13, 26, 11];
  if (t >= 0.5 && t < 1.6) { const k = ez(pr(t, 0.5, 1.6)); cx = lerp(xs[0], xs[1], k); }
  else if (t >= 1.6 && t < 1.9) cx = xs[1];
  else if (t >= 1.9 && t < 3.4) { const k = ez(pr(t, 1.9, 3.4)); cx = lerp(xs[1], xs[2], k); }
  else if (t >= 3.4) cx = xs[2];
  // the shell angle follows the distance travelled (spinning faster than it rolls: he's sprinting).
  // on the way back it spins just enough to carry the stuck pancake round to the very top.
  const squashAt = 1.25;
  const angAt = (lerp(xs[0], xs[1], ez(pr(squashAt, 0.5, 1.6))) - xs[0]) / R * M;
  const angMax = (xs[1] - xs[0]) / R * M;
  const M2 = (Math.PI / 2 + (angMax - angAt) + Math.PI / 2) * R / (xs[1] - xs[2]);
  const dR = Math.min(cx, xs[1]) - xs[0];
  const dL = t >= 1.9 ? xs[1] - cx : 0;
  ang = dR / R * M - dL / R * M2;
  const moving = (t >= 0.5 && t < 1.6) || (t >= 1.9 && t < 3.4);
  if (moving) { lean = t < 1.6 ? 1 : -1; bob = Math.floor(t * 10) % 2 ? -1 : 0; }
  // the food: squashed flat when the ball reaches it, and stuck to the shell
  if (t < 0.4) food(fcx, dropY(t, 0, 0.4, 23), 0);
  else if (t < squashAt) food(fcx, 23, 0);
  const peel = 3.45, panEnd = -Math.PI / 2;
  if (t >= squashAt && t < peel) pan = Math.PI / 2 + (ang - angAt);
  if (t >= squashAt && t < squashAt + 0.15) for (let i = -4; i <= 4; i++) set(fcx + i, 27, 2);   // squish
  bigShell(Math.round(cx), cy, ang, t >= peel - 0.05 && t < peel + 0.6 ? panEnd : null);
  if (pan != null) for (let i = -2; i <= 2; i++) { const a = pan + i / 13; set(Math.round(cx + Math.cos(a) * 13), Math.round(cy + Math.sin(a) * 13), 2); set(Math.round(cx + Math.cos(a) * 14), Math.round(cy + Math.sin(a) * 14), 3); }
  // the hamster
  const hx = Math.round(cx) - 8 + lean, hy = 9 + bob;
  const shut = t >= 5.6 && t < 6.6;
  put('BB_BALL_HAM', hx, hy, { shut });
  // the pancake drops in, bonks him, slides to his mouth
  const px0 = Math.round(cx + Math.cos(panEnd) * 12), py0 = Math.round(cy + Math.sin(panEnd) * 12);
  if (t >= peel && t < 3.85) { const k = pr(t, peel, 3.85); const px = Math.round(lerp(px0, hx + 8, k)), py = Math.round(lerp(py0, hy - 1, k * k)); warp(F.n[0], px - 2, py, 5, 2, {}); }
  if (t >= 3.85 && t < 4.1) { warp(F.n[0], hx + 6, hy - 2, 5, 2, {}); spark(hx + 13, hy - 4); }
  if (t >= 4.1 && t < 4.4) { const k = pr(t, 4.1, 4.4); warp(F.n[0], hx + 6, Math.round(lerp(hy - 1, hy + 9, k)), 5, 2, {}); open = true; }
  const chew = t >= 4.4 && t < 5.6, ck = (t - 4.4) / 0.4;
  if (chew) { const cst = Math.min(2, Math.floor(ck)); if (cst < 2 && (ck % 1) < 0.5) warp(F.n[cst + 1], hx + 7, hy + 10, 3, 2, { mid: true }); open = (ck % 1) < 0.5; }
  if (open) { set(hx + 7, hy + 11, 3); set(hx + 9, hy + 11, 3); set(hx + 8, hy + 12, 3); }
  if (t > 6.0 && Math.floor(t * 5) % 2 === 0) spark(Math.round(cx) + 14, 3);
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
