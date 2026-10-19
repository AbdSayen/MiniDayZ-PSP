const fs=require('fs');const P=JSON.parse(fs.readFileSync('C:/Users/abdur/Desktop/MiniDayZ/original_docs/data.js','utf8').replace(/^\uFEFF/,'')).project;
const T=JSON.parse(fs.readFileSync('typenames.json','utf8'));
const cars=[204,480,203,476,205,478,479,229,230,231,232,233,249,251,263,857,858,859].filter(i=>/^car_(?!loot)/.test(T[i]||''));
const fam=P[3][920];console.log('t920 members?',JSON.stringify(P[3].length));
const cnt={};for(const L of P[5]){for(const ly of L[6]){for(const inst of ly[14]||[]){const t=inst[1];const nm=T[t]||'';if(/^(car_|obst_base|big_obstacle_base|ee_car|ee_fallcar|ee_bi_car)/.test(nm)&&!/loot/.test(nm)){const g=inst[0];const k=L[0]+'/'+ly[0]+'/'+nm+(g[3]<0?' MIRRORED':'')+(g[4]<0?' FLIPPED':'')+(g[5]?' ANG'+g[5].toFixed(2):'');cnt[k]=(cnt[k]||0)+1;}}}}
for(const k in cnt)console.log(cnt[k],k);
