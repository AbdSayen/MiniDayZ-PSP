const fs = require('fs');
const analysis = JSON.parse(
  fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/cpp/docs_gen/c2_analysis.json', 'utf8')
);
const raw = fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js', 'utf8');
const idxToPrefix = {};
for (const o of analysis.objects || []) {
  if (o.prefixes && o.prefixes[0]) idxToPrefix[o.idx] = o.prefixes[0];
}

const starts = [];
let pos = 0;
while (true) {
  const i = raw.indexOf('[[1,[2,"create_map_location"]]]]],[],[[', pos);
  if (i < 0) break;
  starts.push(i);
  pos = i + 1;
}
console.log('starts', starts);

for (const defAt of starts) {
  const body = raw.slice(defAt, defAt + 500000);
  for (const name of ['city', 'city2', 'city3', 'barrack', 'church', 'gas', 'firestation', 'police', 'hospital']) {
    const i = body.indexOf(`[2,"${name}"]`);
    if (i >= 0) console.log('def', defAt, 'has', name, 'at', i);
  }
}

// Use larger body from first start - search generate_array_locations
const genAt = raw.indexOf('[[1,[2,"generate_array_locations"]]]]],[],[[');
console.log('generate_array_locations', genAt);
const gbody = raw.slice(genAt, genAt + 800000);
for (const name of ['village', 'city', 'barrack', 'military', 'gas', 'hospital', 'fire', 'police', 'church']) {
  const re = new RegExp('\\[2,"' + name + '"\\]', 'g');
  let c = 0;
  while (re.exec(gbody)) c++;
  if (c) console.log('gen_array', name, c);
}

// generate_location def
const glAt = raw.indexOf('[[1,[2,"generate_location"]]]]],[],[[');
console.log('generate_location', glAt);
const glbody = raw.slice(glAt, glAt + 600000);
console.log('gl len', glbody.length);
// find create_map_location calls inside
let c2 = 0;
for (const m of glbody.matchAll(/create_map_location/g)) c2++;
console.log('create_map_location mentions in generate_location', c2);
