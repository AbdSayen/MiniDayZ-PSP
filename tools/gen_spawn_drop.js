// gen_spawn_drop.js: decode C2 Function "Spawn_drop" (Game_events) -> per-param count rule table.
const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
const pre=src.slice(0,src.indexOf('const re=new RegExp'));eval(pre+`
let fn=null;function w(l){for(const e of l){if(!Array.isArray(e)||e[0]!==0)continue;if(e[5].length&&refs[e[5][0][1]]&&refs[e[5][0][1]].endsWith('OnFunction')&&JSON.stringify(e[5][0][9])==='[[1,[2,"Spawn_drop"]]]'){fn=e;return;}if(e[7])w(e[7]);if(fn)return;}}
P[6].forEach(sh=>{if(!fn)w(sh[1]);});
const rows=[];const other=[];
for(const s of fn[7]){if(!Array.isArray(s)||s[0]!==0)continue;const c=s[5][0];if(!c||!refs[c[1]].endsWith('CompareParam')){other.push(dec(s,0));continue;}
 const pv=c[9][2][1][1];let type=-1,cnt=null,farmer=0,extra=[];
 for(const a of s[6]){const r=refs[a[1]];if(r.endsWith('CreateObject'))type=a[5][0][1];else if(r.endsWith('SetInstanceVar')&&a[5][0][1]===1)cnt=a[5][1][1];else extra.push(act(a));}
 for(const ss of (s[7]||[])){const t=dec(ss,0);if(/perk_farmer/.test(t)&&/AddInstanceVar \\[\\[10,1\\],\\[7,\\[0,(\\d+)\\]\\]\\]/.test(t))farmer=+RegExp.$1;else extra.push('SUB '+t.replace(/\\n/g,' | ').slice(0,300));}
 if(s[5].length>1)extra.push('MORECOND '+s[5].slice(1).map(cnd).join(' && '));
 rows.push({pv,type:T[type]||type,cnt,farmer,extra});}
fs.writeFileSync('C:/Users/abdur/agent-tools/sd.json',JSON.stringify(rows));if(other.length)console.error('OTHER',other.join('\\n').slice(0,3000));`);
