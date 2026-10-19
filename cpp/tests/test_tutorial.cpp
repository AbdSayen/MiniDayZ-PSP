#include "mdaz/c2_session_globals.hpp"
#include "mdaz/tutorial.hpp"

#include "mdaz/tutorial_layout.hpp"

#include <cstdio>
#include <cstring>

int test_tutorial() {
  std::printf("--- test_tutorial / movement / tutor flags ---\n");
  int fail = 0;

  mdaz::touch_tutorial_c2_audit();

  mdaz::C2SessionGlobals g;
  mdaz::c2_init_desktop_controls(g);
  if (g.movement_stick_enabled != 1.0 || g.movement_tap_enabled != 0.0) {
    std::printf("FAIL desktop controls stick=%g tap=%g\n", g.movement_stick_enabled,
                g.movement_tap_enabled);
    ++fail;
  } else {
    std::printf("OK   CONTROLS STICK desktop defaults\n");
  }

  mdaz::c2_invoke_movement_tap(g, 1);
  if (g.run_touch_id != -1.0 || g.movement_tap_enabled != 1.0) {
    std::printf("FAIL Movement_tap(1) run_touch_id=%g\n", g.run_touch_id);
    ++fail;
  } else {
    std::printf("OK   Movement_tap AltMove/run_touch_id\n");
  }

  mdaz::c2_invoke_movement_stick(g, 0);
  mdaz::c2_invoke_movement_tap(g, 0);
  mdaz::c2_invoke_movement_stick(g, 1);
  if (g.movement_stick_enabled != 1.0) {
    std::printf("FAIL Movement_stick(1)\n");
    ++fail;
  } else {
    std::printf("OK   Movement_stick enable\n");
  }

  g.tutor_bleeding_step = 1.0;
  g.tutorBleedingStopped = 0.0;
  if (!mdaz::c2_bleeding_guide_would_show(g)) {
    std::printf("FAIL Bleeding_guide step 1 should show\n");
    ++fail;
  } else {
    std::printf("OK   Bleeding_guide gate step\n");
  }

  mdaz::c2_note_tutor_bleeding_stopped(g);
  if (g.tutorBleedingStopped != 1.0 || mdaz::c2_bleeding_guide_would_show(g)) {
    std::printf("FAIL tutorBleedingStopped suppress\n");
    ++fail;
  } else {
    std::printf("OK   tutorBleedingStopped\n");
  }

  mdaz::c2_note_tutor_movement_chosen(g, 0);
  if (g.tutorMoveChoosen != 1.0 || g.tutorMovementType != 0.0) {
    std::printf("FAIL tutorMoveChoosen/type\n");
    ++fail;
  } else {
    std::printf("OK   tutorMoveChoosen / tutorMovementType TAP\n");
  }

  // PSP hint wording (user-requested deviation): no shown tutorial line may name touch /
  // mouse / keyboard controls; every override keeps an original line to compare against.
  {
    const char* bad[] = {"tap", "Tap", "drag", "mouse", "click", "key", "this button", "the button"};
    int checked = 0, overrides = 0; const int fail0 = fail;
    for (int id = 0; id < 600; ++id) {
      const char* s = mdaz::tut_ui_text(id);
      if (!s || !s[0]) continue;
      ++checked;
      if (mdaz::tut_ui_text_is_psp_override(id)) {
        ++overrides;
        std::printf("     hint %d\n       C2 : %s\n       PSP: %s\n", id, mdaz::tut_ui_text_original(id), s);
      }
      for (unsigned b = 0; b < sizeof(bad) / sizeof(bad[0]); ++b)
        if (std::strstr(s, bad[b])) { std::printf("FAIL hint %d still says '%s': %s\n", id, bad[b], s); ++fail; }
    }
    if (overrides != 9) { std::printf("FAIL PSP hint overrides=%d (want 9)\n", overrides); ++fail; }
    else if (fail == fail0) std::printf("OK   PSP hint wording: %d tutorial lines checked, %d rewritten, none name tap/drag/mouse/key\n", checked, overrides);
  }

  return fail;
}
