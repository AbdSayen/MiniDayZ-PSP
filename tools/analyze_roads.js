const fs = require('fs');
const { execSync } = require('child_process');

function extract(uid) {
  execSync(`node tools/extract_event_block.js ${uid}`, { stdio: 'pipe' });
  return fs.readFileSync('cpp/docs_gen/river_event_block.json', 'utf8');
}

for (const [name, uid] of [
  ['roads_1', '921061167763666'],
  ['roads_2', '484111970403648'],
]) {
  const b = extract(uid);
  console.log('\n===', name, 'len', b.length);
  const vals = {};
  let p = 0;
  const needle = '[511,182,null';
  while ((p = b.indexOf(needle, p)) >= 0) {
    const m = b.slice(p, p + 280).match(/\[7,\[0,(\d+)\]\]\]/);
    if (m) vals[m[1]] = (vals[m[1]] || 0) + 1;
    p++;
  }
  console.log('511,182 sets:', vals);
  // compare 511,186 reads for type 4
  let c4 = 0;
  p = 0;
  while ((p = b.indexOf('[7,[0,4]]', p)) >= 0) {
    c4++;
    p++;
  }
  console.log('[7,[0,4]] count', c4);
}
