const { chromium } = require('playwright');
(async () => {
  const [id, out, ...ts] = process.argv.slice(2);
  const b = await chromium.launch(); const p = await b.newPage();
  await p.goto('file://' + process.cwd() + '/eating-lab.html');
  const data = await p.evaluate(({ id, ts }) => { const a = LAB.ANIMS.find(x => x.id === id); return ts.map(t => { LAB.render(a, +t); return { t, px: Array.from(LAB.buf) }; }); }, { id, ts });
  const S = 6, cols = 6, rows = Math.ceil(data.length / cols);
  await p.setContent(`<canvas id=c width="${cols * (40 * S + 8)}" height="${rows * (28 * S + 18)}"></canvas>`);
  await p.evaluate(({ data, S, cols }) => { const c = document.getElementById('c'), g = c.getContext('2d'); g.fillStyle = '#888'; g.fillRect(0, 0, c.width, c.height); g.font = '11px sans-serif'; const pal = ['#c6c9be', '#b4b8ac', '#70746a', '#1e211c'];
    data.forEach((f, k) => { const ox = (k % cols) * (40 * S + 8) + 4, oy = Math.floor(k / cols) * (28 * S + 18) + 14; g.fillStyle = '#fff'; g.fillText(f.t, ox, oy - 3); g.fillStyle = pal[0]; g.fillRect(ox, oy, 40 * S, 28 * S);
      for (let y = 0; y < 28; y++) for (let x = 0; x < 40; x++) { const v = f.px[y * 40 + x]; if (v) { g.fillStyle = v === 2 ? pal[2] : pal[3]; g.fillRect(ox + x * S, oy + y * S, S - 1, S - 1); } } }); }, { data, S, cols });
  await (await p.$('#c')).screenshot({ path: out }); await b.close();
})();
