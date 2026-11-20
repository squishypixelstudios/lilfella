import json
t=open('template.html').read()
t=t.replace('__SPRITES__',open('sprites.json').read()).replace('__ENGINE__',open('engine.js').read())
open('eating-lab.html','w').write(t)
print(len(t))
