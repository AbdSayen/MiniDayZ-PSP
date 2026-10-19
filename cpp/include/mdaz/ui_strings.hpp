#pragma once

namespace mdaz {

class IPlatform;

// l_eng_ui.xml Dictionary lines (C2 export). Loads at runtime; embeds quest fallbacks.
bool ui_strings_load(IPlatform* platform, const char* path);
const char* ui_string(int id);

} // namespace mdaz
