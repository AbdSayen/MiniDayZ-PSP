const fs = require('fs');
const path = require('path');
const files = [
  'cpp/src/core/game.cpp',
  'cpp/src/ai/zombie.cpp',
  'cpp/src/ai/animal.cpp',
  'cpp/src/ai/bandit.cpp',
  'cpp/src/world/loot.cpp',
  'cpp/src/player/survival.cpp',
];
for (const f of files) {
  const lines = fs.readFileSync(path.join(__dirname, '..', f), 'utf8').split(/\n/);
  for (let i = 0; i < lines.length; i++) {
    const L = lines[i];
    const hit =
      /stub|invent|approx|PLACEHOLDER|guess|kGunDmgPlaceholder/i.test(L) ||
      /next_float\(\) < 0\.\d/.test(L);
    if (!hit) continue;
    if (/draw_rect|vignette|alpha|0\.1f \+|lightning|hit_effect/.test(L)) continue;
    console.log(f + ':' + (i + 1) + '  ' + L.trim().slice(0, 140));
  }
}
