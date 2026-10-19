// c2chain.js <sid> : ancestor chain with conditions AND own actions, then direct children headers (cond+acts, no deeper)
const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
const pre=src.slice(0,src.indexOf('const re=new RegExp'));eval(pre+`
const sid=+process.argv[2];const chain=[];
function own(c,i){const p='  '.repeat(i);const o=[];if(c[1])o.push(p+'GROUP '+c[1][1]);o.push(p+'EVENT sid'+c[4]+(c[2]?' [OR]':'')+':');for(const x of c[5])o.push(p+'  ? '+cnd(x));for(const a of c[6])o.push(p+'  > '+act(a));return o.join('\\n');}
function w(list){for(const e of list){if(!Array.isArray(e)||e[0]!==0)continue;chain.push(e);if(e[4]===sid){chain.forEach((c,i)=>console.log(own(c,i)));(e[7]||[]).forEach(s=>{if(Array.isArray(s)&&s[0]===0)console.log(own(s,chain.length));});process.exit(0);}if(e[7])w(e[7]);chain.pop();}}
P[6].forEach(sh=>w(sh[1]));`);
