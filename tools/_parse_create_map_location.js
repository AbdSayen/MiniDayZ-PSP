const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

// find function def
const defs = [];
for (const name of ['create_map_location', 'generate_location', 'generate_array_locations']) {
  const needle = `[180,40,null,2,false,false,false,`;
  let pos = 0;
  while (true) {
    const i = raw.indexOf(`[[1,[2,"${name}"]]]]],[],[[`, pos);
    if (i < 0) break;
    defs.push({ name, at: i });
    pos = i + 1;
  }
}
console.log('defs', defs);

function bodyOf(at) {
  return raw.slice(at, at + 2_000_000);
}

// create_map_location - search compare on location name strings
const locDef = defs.find((d) => d.name === 'create_map_location');
if (locDef) {
  const body = bodyOf(locDef.at);
  console.log('\ncreate_map_location len', body.length);
  const strings = [...body.matchAll(/\[2,"([^"]{3,40})"\]/g)].map((m) => m[1]);
  const freq = {};
  for (const s of strings) {
    if (/village|city|military|gas|hospital|fire|police|church|heli|bunker|secret|hostel|school|super|barrack/i.test(s))
      freq[s] = (freq[s] || 0) + 1;
  }
  console.log('interesting strings', freq);

  // calls to create_map_element with literal type id: [0,[0,N]] before call
  for (const m of body.matchAll(/create_map_element[^\]]{0,200}/g)) {
    // skip
  }
  // Pattern: [0,[19,150]],[0,[0,4]] — element layout id?
  const layoutIds = {};
  for (const m of body.matchAll(/\[0,\[0,(\d+)\]\],\[7,\[20,501,151/g)) layoutIds[m[1]] = (layoutIds[m[1]] || 0) + 1;
  console.log('layout ids before create_map_element param', layoutIds);
}

// Search whole data.js for create_map_element calls with preceding type literals
const callSites = [];
let pos = 0;
const callNeedle = '[1,[2,"create_map_element"]]';
while (true) {
  const i = raw.indexOf(callNeedle, pos);
  if (i < 0) break;
  const ctx = raw.slice(Math.max(0, i - 400), i + 50);
  callSites.push(ctx);
  pos = i + 1;
}
console.log('\ncreate_map_element call sites', callSites.length);
for (let n = 0; n < callSites.length; n++) {
  const ctx = callSites[n];
  // extract [0,[0,X]] near end
  const nums = [...ctx.matchAll(/\[0,\[0,(\d+)\]\]/g)].map((m) => m[1]);
  const strs = [...ctx.matchAll(/\[2,"([^"]+)"\]/g)].map((m) => m[1]).slice(-5);
  console.log('call', n + 1, 'nums', nums.slice(-3), 'strs', strs);
}
