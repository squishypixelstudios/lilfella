const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage(); const errs = [];
  p.on('pageerror', e => errs.push(String(e)));
  await p.goto('file://' + process.cwd() + '/eating-lab.html');
  const r = await p.evaluate(() => { const bad = []; for (const a of LAB.ANIMS) for (const f of Object.keys(LAB.FOODK || {meal:1,snack:1})) { try { for (let t = 0; t < a.dur; t += 0.1) LAB.render(a, t); } catch (e) { bad.push(a.id + ':' + e.message); } } return bad; });
  console.log('render errors:', r, 'page errors:', errs, 'count', await p.evaluate(() => LAB.ANIMS.length));
  await b.close();
})();
