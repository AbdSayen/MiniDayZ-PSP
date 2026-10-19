const j = require("../cpp/docs_gen/functions/generate_array_locations.json");
const s = JSON.stringify(j);
for (const v of [0, 2, 3, 4, 5, 6, 7, 9, 11, 12, 13, 14, 15]) {
  const re = new RegExp("\\[7,\\[0," + v + "\\]\\]", "g");
  console.log(v, (s.match(re) || []).length);
}
