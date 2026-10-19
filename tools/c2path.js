// c2path.js <sid> : prints ancestor chain (conditions only) + the block itself
const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
const pre=src.slice(0,src.indexOf('const re=new RegExp'));eval(pre+`
const sid=+process.argv[2];const chain=[];
function w(list){for(const e of list){if(!Array.isArray(e)||e[0]!==0)continue;chain.push(e);if(e[4]===sid){chain.forEach((c,i)=>{if(i<chain.length-1){if(c[1])console.log('  '.repeat(i)+'GROUP '+c[1][1]);console.log('  '.repeat(i)+'EVENT sid'+c[4]+(c[2]?' [OR]':'')+': '+c[5].map(cnd).join(' && '));}});console.log(dec(e,chain.length-1).slice(0,+process.env.MAXC||8000));process.exit(0);}if(e[7])w(e[7]);chain.pop();}}
P[6].forEach(sh=>{console.log('SHEET '+sh[0]);w(sh[1]);});`);
