// gen_car_hitbox.js: car (create_map_element IV1=6) Solid bases from data.js, incl. SetAnimFrame + SetSize(c2-c1)
const fs=require('fs');const src=fs.readFileSync('C:/Users/abdur/agent-tools/c2dump.js','utf8');
eval(src.slice(0,src.indexOf('const re=new RegExp'))+`
let carEv=null;function find(l){for(const e of l){if(!Array.isArray(e)||e[0]!==0)continue;if(e[5].some(c=>refs[c[1]].endsWith('CompareInstanceVar')&&c[0]===501&&JSON.stringify(c[9])==='[[10,1],[8,0],[7,[0,6]]]'))if(e[4]===4174243855126541)carEv=e;if(e[7])find(e[7]);}}
P[6].forEach(s=>find(s[1]));
const fr=(o,f)=>P[3][o][7][0][7][f];
function ipOf(o,f,ip){const F=fr(o,f);if(ip[0]===0&&ip[1]===0)return [0,0];const pts=F[9];const q=ip[0]===0?pts[ip[1]-1]:pts.find(x=>x[0]===ip[1]);if(!q)throw 'ip '+ip;return [(q[1]-F[7])*F[4],(q[2]-F[8])*F[5]];}
const out=[];
for(const s of carEv[7]){const c=s[5][0];if(c[0]!==501)continue;const sub=c[9][2][1][1];let car=null,base=null;
 for(const a of s[6]){const an=refs[a[1]];
  if(an.endsWith('.Spawn')&&a[0]===501){car={sub,oid:a[5][0][1],name:T[a[5][0][1]]};}
  else if(an.endsWith('.Spawn')&&car&&a[0]===car.oid&&(a[5][0][1]===336||a[5][0][1]===339)){base={big:a[5][0][1]===339?1:0,ip:a[5][2][1],frame:0,size:null};car.base=base;}
  else if(base&&(a[0]===336||a[0]===339)&&an.endsWith('SetAnimFrame'))base.frame=a[5][0][1][1];
  else if(base&&(a[0]===336||a[0]===339)&&an.endsWith('SetSize')){const g=x=>x[1][2][0][1];base.size=[a[5][0][1][1][5][0][1], 'c2-c1'];}
 }
 if(!car)continue;const b=car.base;const off=ipOf(car.oid,0,b.ip);const F=fr(b.big?339:336,b.frame);let w=F[4],h=F[5];
 if(b.size){const c1=ipOf(car.oid,0,[2,'c1']),c2=ipOf(car.oid,0,[2,'c2']);w=c2[0]-c1[0];h=c2[1]-c1[1];}
 let poly=F[10];if(!poly||!poly.length)poly=[-F[7],-F[8],1-F[7],-F[8],1-F[7],1-F[8],-F[7],1-F[8]];
 const pts=[];for(let i=0;i<poly.length;i+=2)pts.push([+(off[0]+poly[i]*w).toFixed(2),+(off[1]+poly[i+1]*h).toFixed(2)]);
 out.push({sub,name:car.name,oid:car.oid,big:b.big,frame:b.frame,ip:b.ip,w,h,sized:!!b.size,pts});}
fs.writeFileSync('C:/Users/abdur/agent-tools/car_hitbox.json',JSON.stringify(out,null,1));
for(const o of out)console.log(o.sub,o.name,'big',o.big,'f',o.frame,'ip',JSON.stringify(o.ip),o.w+'x'+o.h,o.sized?'SIZED':'',JSON.stringify(o.pts));`);

