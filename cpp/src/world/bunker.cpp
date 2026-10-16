#include "mdaz/bunker.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace mdaz {

BunkerMap::BunkerMap() : width(0), height(0) {}

static int parse_int_at(const char*& p) {
  while (*p == ' ' || *p == '\t' || *p == ',') ++p;
  char* end = 0;
  long v = std::strtol(p, &end, 10);
  p = end ? end : p;
  return static_cast<int>(v);
}

// C2 RLE: "NxV" means N copies of V, bare V means one copy. Comma-separated.
static bool decode_rle(const char* data, std::vector<int>& out) {
  out.clear();
  const char* p = data;
  while (*p) {
    while (*p == ',' || *p == ' ') ++p;
    if (!*p) break;
    const char* start = p;
    while (*p && *p != ',' && *p != 'x') ++p;
    if (*p == 'x') {
      std::string nstr(start, p);
      int n = std::atoi(nstr.c_str());
      ++p;
      int v = parse_int_at(p);
      if (n < 0) n = 0;
      for (int i = 0; i < n; ++i) out.push_back(v);
    } else {
      std::string vstr(start, p);
      out.push_back(std::atoi(vstr.c_str()));
    }
  }
  return !out.empty();
}

bool BunkerMap::load_from_c2_json(const char* json) {
  if (!json) return false;
  const char* wkey = std::strstr(json, "\"width\"");
  const char* hkey = std::strstr(json, "\"height\"");
  const char* dkey = std::strstr(json, "\"data\"");
  if (!wkey || !hkey || !dkey) return false;

  width = std::atoi(std::strchr(wkey, ':') + 1);
  height = std::atoi(std::strchr(hkey, ':') + 1);

  const char* q1 = std::strchr(dkey, '\"');
  if (!q1) return false;
  q1 = std::strchr(q1 + 1, '\"');
  if (!q1) return false;
  ++q1;
  const char* q2 = std::strchr(q1, '\"');
  if (!q2) return false;
  std::string rle(q1, q2);

  if (!decode_rle(rle.c_str(), tiles)) return false;
  if (static_cast<int>(tiles.size()) < width * height) {
    tiles.resize(static_cast<std::size_t>(width * height), 0);
  }
  return width > 0 && height > 0;
}

bool BunkerMap::load_from_file(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  return load_from_c2_json(ss.str().c_str());
}

int BunkerMap::tile_at(int x, int y) const {
  if (x < 0 || y < 0 || x >= width || y >= height) return -1;
  return tiles[static_cast<std::size_t>(y * width + x)];
}

} // namespace mdaz
