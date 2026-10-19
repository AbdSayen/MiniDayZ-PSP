// c2acts.js <type> : distinct actions applied to object type <type> (first event of each) with its conditions
const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
eval(src.slice(0,src.indexOf('const re=new RegExp'))+`
const ty=+process.argv[2];const seen={};function w(l){for(const e of l){if(!Array.isArray(e)||e[0]!==0)continue;for(const a of e[6]){if(a[0]===ty){const k=refs[a[1]]+(a[2]?'['+a[2]+']':'')+String(JSON.stringify(a[5])).slice(0,60);if(!seen[k]){seen[k]=1;console.log(act(a).slice(0,160),' | sid'+e[4],' | '+e[5].map(cnd).join(' && ').slice(0,220))}}}if(e[7])w(e[7]);}}
P[6].forEach(s=>w(s[1]));`);

