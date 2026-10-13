# nice_view_gem simulator

Renders the vendored `nice_view_gem` status screen **without flashing
hardware**: it compiles the actual shield widget sources (unmodified) against
the exact LVGL revision Zephyr pins, with small stub headers standing in for
Zephyr/ZMK, and writes each frame as a PNG.

Built while developing the runtime light/dark theme toggle (`&nvgem`); use it
any time you touch the widgets' drawing code or the theme machinery.

## Run

```sh
tools/simulator/run.sh            # central half (text status screen)
tools/simulator/run.sh peripheral # peripheral half (crystal animation)
SANITIZE=1 tools/simulator/run.sh central  # with AddressSanitizer
```

Frames land in `tools/simulator/out/`:

- `*_1_boot.png` — boot frame with the compiled-in default theme
  (light unless built with `-DCONFIG_NICE_VIEW_WIDGET_INVERTED=1`)
- `*_2_after_toggle.png` — after simulating a `&nvgem` toggle
- `*_3_after_toggle_back.png` — after toggling back (pixel-identical to frame 1;
  the printed `fbsum` checksums make this easy to confirm)

Each frame also gets a `<name>_view.png` companion: the panel's framebuffer is
160×68 landscape, but the module is mounted rotated on the keyboard, so the
widgets author each 68×68 tile portrait and rotate it into the framebuffer.
`_view.png` rotates the raw frame 90° CCW to the portrait 68×160 image the user
actually sees — use it to judge the visual result (e.g. clipping at the panel
edges is much easier to spot there).

First run clones LVGL into `tools/simulator/lvgl` at Zephyr's pinned revision
(`build/` and `out/` are gitignored).

The sim runs LVGL on a **virtual clock** (`sim_tick_ms` in the sim main, read by
the `k_uptime_get_32()` stub) instead of wall time: each dump advances 32 × 30 ms
= 960 ms, exactly one crystal animation cycle, so frames always render at the
same animation phase and stay byte-comparable. `lv_task_handler()` runs inside
each 30 ms slice, driving the paused-until-invalidated refresh timer the same
way the hardware tick does. Set `SIM_VERBOSE=1` for extra output.

## Simulate other themes / boot defaults

`sim_config.h` mirrors the Kconfig the firmware build would provide. To test
the boot-dark setup, build with:

```sh
tools/simulator/run.sh central -DCONFIG_NICE_VIEW_WIDGET_INVERTED=1
```

## Faking device state

Battery, BLE profile/connection, layer, and WPM values shown on screen come
from the `sim_dev` struct in `sim_main.c` (peripheral: `sim_main_peripheral.c`).
Edit it there if you need to exercise a specific visual state.

## Sim-only shims (do not copy into firmware code)

- `_lv_refr_set_disp_refreshing()` is called before the first widget draw:
  `lv_canvas_transform` reads the refreshing display's chroma key, which is
  only set up inside a refresh — on hardware a refresh has always run by then.
- `k_uptime_get_32()` returns the virtual clock instead of wall time (on
  hardware Zephyr provides the real tick; see the virtual-clock note above).
