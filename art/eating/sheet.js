const { chromium } = require('playwright');
(async () => {
  const id = process.argv[2], n = +process.argv[3] || 12, out = process.argv[4] || 'sheet.png', food = process.argv[5] || 'meal';
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1200, height: 900 } });
  const errs = []; p.on('pageerror', e => errs.push(e.message)); p.on('console', m => { if (m.type() === 'error') errs.push(m.text()); });
  await p.goto('file://' + process.cwd() + '/eating-lab.html');
  const data = await p.evaluate(({ id, n, food }) => {
    LAB.FOODK.kind = food;
    const ids = id === 'all' ? LAB.ANIMS.map(a => a.id) : id.split(',');
    const res = {};
    for (const i of ids) { const a = LAB.ANIMS.find(x => x.id === i); res[i] = []; for (let k = 0; k < n; k++) { const t = a.dur * k / (n - 1); LAB.render(a, t); res[i].push({ t, inv: LAB.invert, px: Array.from(LAB.buf) }); } }
    return res;
  }, { id, n, food });
  const S = 5, cols = Math.min(n, 6);
  const ids = Object.keys(data);
  const rows = ids.length * Math.ceil(n / cols);
  const html = `<canvas id=c width="${cols * (40 * S + 8)}" height="${rows * (28 * S + 18)}"></canvas>`;
  await p.setContent(html);
  await p.evaluate(({ data, S, cols }) => {
    const c = document.getElementById('c'), g = c.getContext('2d'); g.fillStyle = '#888'; g.fillRect(0, 0, c.width, c.height); g.font = '11px sans-serif';
    const pal = ['#c6c9be', '#b4b8ac', '#70746a', '#1e211c']; let r0 = 0;
    for (const id of Object.keys(data)) { const fr = data[id]; fr.forEach((f, k) => { const ox = (k % cols) * (40 * S + 8) + 4, oy = (r0 + Math.floor(k / cols)) * (28 * S + 18) + 14; g.fillStyle = '#fff'; g.fillText(id + ' ' + f.t.toFixed(1), ox, oy - 3);
      g.fillStyle = pal[0]; g.fillRect(ox, oy, 40 * S, 28 * S);
      for (let y = 0; y < 28; y++) for (let x = 0; x < 40; x++) { let v = f.px[y * 40 + x]; if (f.inv) v = v === 0 ? 3 : v === 3 ? 0 : v; if (v) { g.fillStyle = v === 2 ? pal[2] : pal[3]; g.fillRect(ox + x * S, oy + y * S, S - 1, S - 1); } } });
      r0 += Math.ceil(fr.length / cols); }
  }, { data, S, cols });
  await (await p.$('#c')).screenshot({ path: out });
  console.log('errors:', errs.length ? errs.join(' | ') : 'none');
  await b.close();
})();
