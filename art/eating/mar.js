const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  await p.goto('file://' + process.cwd() + '/eating-lab.html');
  const out = process.argv[2];
  const data = await p.evaluate(() => { const a = LAB.ANIMS.find(x => x.id === 'marshy'); const r = []; for (const v of [0, 1]) { LAB.MARSHY.v = v; for (const t of [0.5, 0.95, 1.3, 1.7, 2.1, 2.5, 3.2, 5.2]) { LAB.render(a, t); r.push({ t: v + ':' + t, px: Array.from(LAB.buf) }); } } return r; });
  const S = 6, cols = 8, rows = 2;
  await p.setContent(`<canvas id=c width="${cols * (40 * S + 8)}" height="${rows * (28 * S + 18)}"></canvas>`);
  await p.evaluate(({ data, S, cols }) => { const c = document.getElementById('c'), g = c.getContext('2d'); g.fillStyle = '#888'; g.fillRect(0, 0, c.width, c.height); g.font = '11px sans-serif'; const pal = ['#c6c9be', '#b4b8ac', '#70746a', '#1e211c'];
    data.forEach((f, k) => { const ox = (k % cols) * (40 * S + 8) + 4, oy = Math.floor(k / cols) * (28 * S + 18) + 14; g.fillStyle = '#fff'; g.fillText(f.t, ox, oy - 3); g.fillStyle = pal[0]; g.fillRect(ox, oy, 40 * S, 28 * S);
      for (let y = 0; y < 28; y++) for (let x = 0; x < 40; x++) { const v = f.px[y * 40 + x]; if (v) { g.fillStyle = v === 2 ? pal[2] : pal[3]; g.fillRect(ox + x * S, oy + y * S, S - 1, S - 1); } } }); }, { data, S, cols });
  await (await p.$('#c')).screenshot({ path: out }); await b.close();
})();
