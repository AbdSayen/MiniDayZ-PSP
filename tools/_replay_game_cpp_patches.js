/**
 * Rebuild game.cpp by replaying Write + StrReplace tool calls from agent transcripts.
 */
const fs = require('fs');
const path = require('path');

const root = path.join(
  process.env.USERPROFILE || '',
  '.cursor/projects/c-Users-abdur-Desktop-MiniDayZ/agent-transcripts'
);
const targetNorm = 'cpp/src/core/game.cpp';

function walk(d, acc) {
  if (!fs.existsSync(d)) return acc;
  for (const f of fs.readdirSync(d)) {
    const p = path.join(d, f);
    const st = fs.statSync(p);
    if (st.isDirectory()) walk(p, acc);
    else if (f.endsWith('.jsonl')) acc.push({ p, mtime: st.mtimeMs });
  }
  return acc;
}

function isGamePath(p) {
  if (typeof p !== 'string') return false;
  const n = p.replace(/\\/g, '/').toLowerCase();
  return n.endsWith(targetNorm);
}

function visit(obj, ops, file, lineNo) {
  if (!obj || typeof obj !== 'object') return;
  if (Array.isArray(obj)) {
    for (const x of obj) visit(x, ops, file, lineNo);
    return;
  }
  if ((obj.name === 'Write' || obj.name === 'StrReplace') && obj.input && isGamePath(obj.input.path)) {
    ops.push({
      file,
      lineNo,
      name: obj.name,
      input: obj.input,
    });
  }
  for (const k of Object.keys(obj)) visit(obj[k], ops, file, lineNo);
}

const files = walk(root, []).sort((a, b) => a.mtime - b.mtime);
const ops = [];
for (const { p } of files) {
  const lines = fs.readFileSync(p, 'utf8').split(/\n/);
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    if (!line.includes('game.cpp')) continue;
    let msg;
    try {
      msg = JSON.parse(line);
    } catch (e) {
      continue;
    }
    visit(msg, ops, p, i + 1);
  }
}

console.log('ops found', ops.length);
let text = '';
let writes = 0;
let replaces = 0;
let replaceFail = 0;

for (const op of ops) {
  if (op.name === 'Write') {
    text = op.input.contents || '';
    writes++;
    console.log('WRITE', text.length, 'from', path.basename(path.dirname(op.file)), 'L' + op.lineNo);
  } else if (op.name === 'StrReplace') {
    const oldS = op.input.old_string;
    const newS = op.input.new_string;
    const all = !!op.input.replace_all;
    if (typeof oldS !== 'string' || typeof newS !== 'string') {
      replaceFail++;
      continue;
    }
    if (!text) {
      replaceFail++;
      continue;
    }
    if (all) {
      if (!text.includes(oldS)) {
        replaceFail++;
        continue;
      }
      text = text.split(oldS).join(newS);
      replaces++;
    } else {
      const idx = text.indexOf(oldS);
      if (idx < 0) {
        replaceFail++;
        continue;
      }
      text = text.slice(0, idx) + newS + text.slice(idx + oldS.length);
      replaces++;
    }
  }
}

const outPath = path.join(__dirname, '../cpp/src/core/game.cpp');
fs.writeFileSync(outPath, text);
console.log('result bytes', text.length, 'writes', writes, 'replaces', replaces, 'replaceFail', replaceFail);
console.log('lines', text.split(/\n/).length);
console.log('has dmg_hints_', text.includes('dmg_hints_'));
console.log('has hurt_zombie_bullet', text.includes('hurt_zombie_bullet'));
console.log('has wetness_ = 0', text.includes('wetness_ = 0'));
