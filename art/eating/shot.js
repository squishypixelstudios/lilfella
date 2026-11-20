const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch();
  for (const [name, vw, vh, scheme] of [['desk', 1200, 900, 'light'], ['mob', 390, 800, 'dark']]) {
    const p = await b.newPage({ viewport: { width: vw, height: vh }, colorScheme: scheme });
    await p.goto('file://' + process.cwd() + '/eating-lab.html'); await p.waitForTimeout(2200);
    await p.screenshot({ path: process.argv[2] + '/page-' + name + '.png', fullPage: name === 'mob' ? false : true });
    const sw = await p.evaluate(() => document.documentElement.scrollWidth); console.log(name, 'scrollWidth', sw, 'vw', vw);
  }
  await b.close();
})();
