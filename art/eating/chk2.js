const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage(); const errs = [];
  p.on('pageerror', e => errs.push(String(e)));
  await p.goto('file://' + process.cwd() + '/eating-lab.html');
  await p.waitForTimeout(1500);
  const r = await p.evaluate(() => { const bad = []; for (const k of ['meal', 'snack']) { LAB.FOODK.kind = k; for (const a of LAB.ANIMS) { try { for (let t = 0; t < a.dur; t += 0.05) LAB.render(a, t); } catch (e) { bad.push(k + ':' + a.id + ':' + e.message); } } } return bad; });
  console.log(r, errs, await p.evaluate(() => [document.querySelectorAll('#grid .card').length, document.querySelectorAll('#grid2 .card').length]));
  await p.setViewportSize({ width: 400, height: 900 }); console.log('overflow', await p.evaluate(() => document.documentElement.scrollWidth));
  await b.close();
})();
