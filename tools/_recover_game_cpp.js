/**
 * Recover game.cpp from agent transcript tool Write payloads.
 */
const fs = require('fs');
const path = require('path');

const root = path.join(
  process.env.USERPROFILE || '',
  '.cursor/projects/c-Users-abdur-Desktop-MiniDayZ/agent-transcripts'
);

function walk(d, acc) {
  if (!fs.existsSync(d)) return acc;
  for (const f of fs.readdirSync(d)) {
    const p = path.join(d, f);
    if (fs.statSync(p).isDirectory()) walk(p, acc);
    else if (f.endsWith('.jsonl')) acc.push(p);
  }
  return acc;
}

function visit(obj, hits) {
  if (!obj || typeof obj !== 'object') return;
  if (Array.isArray(obj)) {
    for (const x of obj) visit(x, hits);
    return;
  }
  if (obj.name === 'Write' && obj.input && typeof obj.input.path === 'string') {
    const p = obj.input.path.replace(/\\/g, '/');
    if (p.endsWith('cpp/src/core/game.cpp') && typeof obj.input.contents === 'string') {
      hits.push(obj.input.contents);
    }
  }
  for (const k of Object.keys(obj)) visit(obj[k], hits);
}

const files = walk(root, []);
let best = { len: 0, file: '', text: '' };

for (const f of files) {
  const lines = fs.readFileSync(f, 'utf8').split(/\n/);
  for (const line of lines) {
    if (!line.includes('game.cpp') || !line.includes('contents')) continue;
    let msg;
    try {
      msg = JSON.parse(line);
    } catch (e) {
      continue;
    }
    const hits = [];
    visit(msg, hits);
    for (const text of hits) {
      if (text.length > best.len && text.includes('namespace mdaz')) {
        best = { len: text.length, file: f, text };
        console.log('candidate', text.length, path.basename(path.dirname(f)));
      }
    }
  }
}

if (!best.text) {
  console.error('No recoverable Write payload found');
  process.exit(1);
}

const outPath = path.join(__dirname, '../cpp/src/core/game.cpp');
fs.writeFileSync(outPath, best.text);
console.log('Wrote', outPath, 'bytes', best.len);
console.log('source', best.file);
