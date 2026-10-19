// gen_base_sizes.js: every create_map_element-spawned obst_base/big_obstacle_base that gets SetSize; evaluates size on spawner frame 0
const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
eval(src.slice(0,src.indexOf('const re=new RegExp'))+`
const fr=(o,f)=>P[3][o][7][0][7][f];
function ip(o,name){const F=fr(o,0);const q=F[9].find(x=>x[0]===name);if(!q)throw o+' '+name;return [(q[1]-F[7])*F[4],(q[2]-F[8])*F[5]];}
function ev(x){if(x[0]===0)return x[1];if(x[0]===5)return ev(x[1][0]||x[1])-ev(x[2]||x[1][1]);if(x[0]===20){const nm=x[5][0][1];return ip(x[1],nm)[x[2]===59?0:1];}throw JSON.stringify(x);}
function ev2(x){if(x[0]===0)return x[1];if(x[0]===5){return ev2(x[1])-ev2(x[2]);}if(x[0]===20){return ip(x[1],x[5][0][1])[x[2]===59?0:1];}throw JSON.stringify(x);}
const res=[];function w(l){for(const e of l){if(!Array.isArray(e)||e[0]!==0)continue;let sp=null;for(const a of e[6]){const an=refs[a[1]];
 if(an.endsWith('.Spawn')&&(a[5][0][1]===336||a[5][0][1]===339))sp={by:a[0],base:a[5][0][1],ip:a[5][2][1]};
 else if(sp&&a[0]===sp.base&&an.endsWith('SetSize')){let W,H;try{W=ev2(a[5][0][1]);H=ev2(a[5][1][1]);}catch(err){W='?'+err;H='?';}res.push({sid:e[4],by:sp.by,name:T[sp.by],base:sp.base,ip:sp.ip,w:W,h:H});}
 else if(sp&&a[0]===sp.base&&/SetWidth|SetHeight/.test(an))res.push({sid:e[4],by:sp.by,name:T[sp.by],base:sp.base,note:an+' '+JSON.stringify(a[5])});}
 if(e[7])w(e[7]);}}
P[6].forEach(s=>w(s[1]));fs.writeFileSync('C:/Users/abdur/agent-tools/base_sizes.json',JSON.stringify(res,null,1));
for(const r of res)console.log(r.sid,r.name,r.base,JSON.stringify(r.ip),typeof r.w==='number'?r.w.toFixed(2):r.w,typeof r.h==='number'?r.h.toFixed(2):r.h,r.note||'');`);
