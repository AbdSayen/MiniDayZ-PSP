// c2dump.js <regex on decoded text> [maxdepthTop]  - decodes C2 event blocks via getObjectRefTable
const fs=require('fs');const D='C:/Users/abdur/Desktop/MiniDayZ/original_docs/';
const P=JSON.parse(fs.readFileSync(D+'data.js','utf8').replace(/^\uFEFF/,'')).project;
const rt=fs.readFileSync(D+'c2runtime.js','utf8');const s0=rt.indexOf('cr.getObjectRefTable = function');
const body=rt.slice(s0,rt.indexOf('];',s0));const refs=body.split('\n').slice(1).map(l=>l.trim().replace(/,$/,'').replace(/^cr\.(plugins_|behaviors|system_object)\.?/,'').replace('.prototype.cnds.','.C.').replace('.prototype.acts.','.A.').replace('.prototype.exps.','.E.'));
const T=JSON.parse(fs.readFileSync('C:/Users/abdur/agent-tools/typenames.json','utf8'));
const tn=i=>i===-1?'System':(T[i]||('t'+i))+'#'+i;
function prm(p){return JSON.stringify(p,(k,v)=>v).slice(0,400);}
function cnd(c){return (c[5]?'NOT ':'')+tn(c[0])+(c[2]?'['+c[2]+']':'')+' '+refs[c[1]]+(c[9]?' '+prm(c[9]):'');}
function act(a){return tn(a[0])+(a[2]?'['+a[2]+']':'')+' '+refs[a[1]]+(a[5]?' '+prm(a[5]):'');}
function dec(e,ind){const pad='  '.repeat(ind);if(e[0]===1)return pad+'VAR '+e[1]+' = '+JSON.stringify(e[3]);if(e[0]===2)return pad+'INCLUDE '+e[1];if(e[0]!==0)return pad+'?'+JSON.stringify(e).slice(0,100);
 let out=[];if(e[1])out.push(pad+'GROUP '+e[1][1]);const or=e[2];
 out.push(pad+'EVENT sid'+e[4]+(or?' [OR]':'')+':');for(const c of e[5])out.push(pad+'  ? '+cnd(c));for(const a of e[6])out.push(pad+'  > '+act(a));
 if(e[7])for(const s of e[7])out.push(dec(s,ind+1));return out.join('\n');}
const re=new RegExp(process.argv[2]);const mode=process.argv[3]||'leaf';
function walk(list,path){for(const e of list){if(!Array.isArray(e)||e[0]!==0)continue;const d=dec(e,0);if(!re.test(d))continue;
 const subHit=(e[7]||[]).some(s=>Array.isArray(s)&&s[0]===0&&re.test(dec(s,0)));
 if(mode==='leaf'&&subHit&&d.length>6000){walk(e[7],path);continue;}
 console.log('=== '+path+'\n'+d.slice(0,+process.env.MAXC||20000));}}
P[6].forEach(sh=>walk(sh[1],sh[0]));

