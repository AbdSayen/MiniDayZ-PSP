/** Heuristic: list likely-invented float chance literals in game.cpp comments+code. */
const fs = require('fs');
const path = require('path');
const files = [
  'cpp/src/core/game.cpp',
  'cpp/src/ai/zombie.cpp',
  'cpp/src/ai/animal.cpp',
  'cpp/src/ai/bandit.cpp',
  'cpp/src/world/loot.cpp',
  'cpp/src/world/vehicle.cpp',
];
for (const f of files) {
  const t = fs.readFileSync(path.join(__dirname, '..', f), 'utf8');
  const lines = t.split(/\n/);
  for (let i = 0; i < lines.length; i++) {
    const L = lines[i];
    if (/stub|invent|approx|PLACEHOLDER|guess|~\\d+%|0\\.0\\d+f \\*|next_float\\(\\) < 0\\./i.test(L) ||
        /next_float\\(\\) < 0\\./.test(L)) {
      if (/draw_rect|0\\.02f \\*|0\\.1f \\+|alpha|vignette|flash/.test(L)) continue;
      console.log(f + ':' + (i + 1) + '  ' + L.trim().slice(0, 120));
    }
  }
}
