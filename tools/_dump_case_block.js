const fs = require('fs');
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');

function dump(layoutName) {
  const tag = `[1,[2,"${layoutName}"]],[3,1]`;
  const tagAt = raw.indexOf(tag);
  const after = raw.slice(tagAt, tagAt + 20000);
  const caseM = after.match(/\[501,71,null,[^\]]+\[\[10,2\],\[8,0\],\[7,\[0,(\d+)\]\]\]\]/);
  const caseStart = tagAt + after.indexOf(caseM[0]) + caseM[0].length;
  let block = raw.slice(caseStart, caseStart + 3500);
  const end = block.indexOf(']]]],[0,null,false,null,');
  if (end > 0) block = block.slice(0, end);
  console.log('\n===', layoutName, 'case', caseM[1], '===');
  // extract literal offsets
  const nums = [...block.matchAll(/\[0,\[0,(-?\d+)\]\]/g)].map((m) => +m[1]);
  console.log('literals [0,[0,n]]:', nums);
  const nums2 = [...block.matchAll(/\[0,\[4,\[20,\d+,65,false,null\],\[0,(-?\d+)\]\]\]/g)].map((m) => +m[1]);
  console.log('hotspot x deltas:', nums2);
  const nums3 = [...block.matchAll(/\[0,\[4,\[20,\d+,66,false,null\],\[0,(-?\d+)\]\]\]/g)].map((m) => +m[1]);
  console.log('hotspot y deltas:', nums3);
  console.log(block.slice(0, 1200));
}

for (const n of ['city3', 'city2', 'brown2', 'white', 'barrack', 'church', 'firestation', 'ga_brown']) dump(n);
