// Simulate C2 generate_array_roads_1 on one row (Array.At(Col, Row)).
function roads1Row(row, grid, size) {
  let anotherX = -1;
  let anotherX2 = -1;
  for (let col = 0; col < size; col++) {
    const v = grid[row][col];
    if (v === 9 || v === 6 || v === 11 || v === 15) {
      if (v === 15) {
        if (anotherX === -1) anotherX = col;
        else anotherX2 = col;
      }
    }
  }
  if (anotherX !== -1 && anotherX2 !== -1 && anotherX2 > anotherX) {
    if (grid[row][anotherX] === 0) grid[row][anotherX] = 13;
  }
}

const size = 16;
const grid = Array.from({ length: size }, () => Array(size).fill(0));
grid[5][3] = 15;
grid[5][10] = 15;
roads1Row(5, grid, size);
console.log("row5", grid[5].join(","));
