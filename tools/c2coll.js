const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
eval(src.slice(0,src.indexOf('const re=new RegExp'))+`
const ids=process.argv.slice(2).map(Number);const seen={};function w(l){for(const e of l){if(!Array.isArray(e)||e[0]!==0)continue;for(const c of e[5]){const p=JSON.stringify(c[9]||[]);if(/Overlap|Collision/.test(refs[c[1]])&&ids.some(i=>p.startsWith('[[4,'+i+']'))){const k=cnd(c);if(!seen[k]){seen[k]=1;console.log(k.slice(0,150),' sid'+e[4])}}}if(e[7])w(e[7]);}}
P[6].forEach(s=>w(s[1]));
for(const i of [204,336,339]){const t=P[3][i];console.log(i,t[0],'behaviors:',JSON.stringify((t[8]||[]).map(b=>b[0]+'/'+b[1])))}`);
