const fs = require('fs');
const path = require('path');
const block = fs.readFileSync(path.join(__dirname, '../cpp/docs_gen/river_event_block.json'), 'utf8');
const idx = block.indexOf('River_ready');
console.log('River_ready at', idx, 'of', block.length);
console.log(block.slice(idx - 200, idx + 8000));
