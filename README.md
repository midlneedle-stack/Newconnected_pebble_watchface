diff --git a/README.md b/README.md
index 8178c76d627cade75005b40711b92f4177bc6cfc..eb9f43551753a1a40b16c68d7f9f3e0056f264ee 100644
--- a/README.md
+++ b/README.md
@@ -1 +1,322 @@
-readme
+# Pebble Watchface Creation Handbook
+
+This handbook condenses the official Re-Pebble guides, tutorials, and C SDK documentation into a single reference you can rely on while designing, implementing, and shipping a Pebble watchface. Work through the sections in order when you are new to the platform, then keep it nearby as a checklist for future projects.
+
+## Table of Contents
+1. [Platform Overview](#platform-overview)
+2. [Set Up the SDK Environment](#set-up-the-sdk-environment)
+3. [Scaffold a Watchface Project](#scaffold-a-watchface-project)
+4. [Understand Project Structure](#understand-project-structure)
+5. [Event-Driven App Lifecycle](#event-driven-app-lifecycle)
+6. [Step-by-Step: Build the First Watchface](#step-by-step-build-the-first-watchface)
+7. [Designing for Every Pebble](#designing-for-every-pebble)
+8. [Working with Graphics and Resources](#working-with-graphics-and-resources)
+9. [Animating Layers and Vector Paths](#animating-layers-and-vector-paths)
+10. [Connecting to Device and Companion Services](#connecting-to-device-and-companion-services)
+11. [Configuration, Persistence, and Localization](#configuration-persistence-and-localization)
+12. [Debugging, Testing, and Optimization](#debugging-testing-and-optimization)
+13. [Package, Publish, and Maintain](#package-publish-and-maintain)
+14. [Reference Cheat Sheet](#reference-cheat-sheet)
+
+---
+
+## Platform Overview
+- **Hardware families**: Pebble Classic (Aplite), Pebble Time (Basalt), Pebble Time Round (Chalk), Pebble 2 (Diorite), Pebble Time 2/Rebble compatible models (Emery). Display resolutions and color capabilities differ, so your UI must adapt.
+- **Operating model**: Apps and watchfaces are native C programs executed by the Pebble firmware. The Pebble SDK provides the toolchain, device emulators, and C libraries.
+- **Build artifacts**: Source is compiled into a `.pbw` bundle containing your binaries and resources. Install to a device/emulator with the `pebble` CLI.
+
+---
+
+## Set Up the SDK Environment
+1. **Install prerequisites**
+   - macOS/Linux: Python 2.7+, Node.js, Git, CMake, and GCC/Clang as described by the SDK installer.
+   - Windows: use WSL with Ubuntu for compatibility, or a dedicated VM.
+2. **Install the Pebble SDK**
+   ```bash
+   curl https://bootstrap.rebble.io/pebble-tool.py | python
+   pebble sdk install latest
+   ```
+   The bootstrapper installs the `pebble` CLI and downloads the latest SDK.
+3. **Install device platform packages**
+   ```bash
+   pebble sdk install basalt
+   pebble sdk install chalk
+   pebble sdk install diorite
+   pebble sdk install emery
+   ```
+   Always test on every platform you intend to support.
+4. **Log into the Rebble services** (optional but recommended) to enable timeline, weather, and app-store features: `pebble login`.
+5. **Set up emulators**: `pebble install --emulator <platform>` automatically starts the selected emulator.
+6. **Connect hardware** (when available) via USB/Bluetooth and ensure Developer Mode is enabled on the watch.
+
+---
+
+## Scaffold a Watchface Project
+1. Navigate to your workspace and run:
+   ```bash
+   pebble new-project my-watchface --template watchface
+   cd my-watchface
+   ```
+2. The template includes starter code in `src/main.c` that opens a window and updates the time every minute.
+3. Initialize Git to track iterations and collaborate: `git init && git add . && git commit -m "Initial"`.
+
+---
+
+## Understand Project Structure
+| Path | Description |
+|------|-------------|
+| `src/` | C source files. `main.c` contains `main`, initialization, and event handlers. Split additional features into modules when the file grows. |
+| `include/` | Optional header files for shared declarations. |
+| `resources/` | Fonts, PNG bitmaps, and raw files described in `resources/src/resource_map.json`. |
+| `package.json` *(SDK 4.x)* / `appinfo.json` *(SDK 3.x)* | Metadata (UUID, app name, version, capabilities, target platforms, watchface flag). |
+| `wscript` | Build configuration. Override to add compiler flags or custom tools if needed. |
+| `src/c` *(newer templates)* | Contains additional modules created by the template. |
+
+Key metadata fields:
+- `"watchface": true` ensures the app appears under Watchfaces.
+- `"targetPlatforms": ["aplite", "basalt", "chalk", "diorite", "emery"]` declares support.
+- `"resources"` entries define IDs used with `gbitmap_create_with_resource` and `fonts_load_custom_font`.
+
+---
+
+## Event-Driven App Lifecycle
+Pebble apps follow an event-driven pattern provided by the Foundation module:
+1. **Initialization**: In `init()`, create windows/layers and subscribe to services.
+2. **Event loop**: `app_event_loop()` dispatches system events (time ticks, battery changes, messages).
+3. **Deinitialization**: In `deinit()`, destroy layers, unsubscribe services, and release resources.
+4. **App Services**: Use `AppTimer`, `AppMessage`, `HealthService`, etc., to schedule work or exchange data.
+
+Use `ApplicationHandlers` only when porting legacy SDK 2.x code—modern projects rely on explicit init/deinit functions.
+
+---
+
+## Step-by-Step: Build the First Watchface
+Follow the official watchface tutorial parts 1–3 to create a functional face.
+
+### 1. Window and Time Layer
+```c
+static Window *s_main_window;
+static TextLayer *s_time_layer;
+
+static void update_time(void) {
+  time_t temp = time(NULL);
+  struct tm *tick_time = localtime(&temp);
+
+  static char buffer[] = "00:00";
+  strftime(buffer, sizeof(buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
+
+  text_layer_set_text(s_time_layer, buffer);
+}
+
+static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
+  update_time();
+}
+
+static void main_window_load(Window *window) {
+  Layer *window_layer = window_get_root_layer(window);
+  GRect bounds = layer_get_bounds(window_layer);
+
+  s_time_layer = text_layer_create(
+      GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
+  text_layer_set_background_color(s_time_layer, GColorBlack);
+  text_layer_set_text_color(s_time_layer, GColorWhite);
+  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
+  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
+
+  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
+}
+
+static void main_window_unload(Window *window) {
+  text_layer_destroy(s_time_layer);
+}
+```
+
+### 2. App Skeleton
+```c
+static void init(void) {
+  s_main_window = window_create();
+  window_set_window_handlers(s_main_window, (WindowHandlers) {
+    .load = main_window_load,
+    .unload = main_window_unload,
+  });
+  window_stack_push(s_main_window, true);
+
+  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
+  update_time();
+}
+
+static void deinit(void) {
+  window_destroy(s_main_window);
+}
+
+int main(void) {
+  init();
+  app_event_loop();
+  deinit();
+}
+```
+
+### 3. Add Date, Battery, and Bluetooth Indicators
+- **Date Layer**: Create another `TextLayer`, place it below/above the time, and update inside `update_time()` using `strftime` with `%a %d %b`.
+- **Battery Icon/Text**: Subscribe via `battery_state_service_subscribe`. Update a text layer or change a bitmap when `BatteryChargeState` changes.
+- **Bluetooth Status**: Use `connection_service_subscribe` to toggle a bitmap or text. Optionally trigger `vibes_double_pulse()` when disconnected.
+
+### 4. Custom Backgrounds and Fonts
+- Declare assets in `resources/src/resource_map.json`:
+  ```json
+  {
+    "resources": {
+      "media": [
+        {"type": "bitmap", "defName": "BACKGROUND", "file": "images/background.png"},
+        {"type": "font", "defName": "FONT_TIME_48", "file": "fonts/Roboto-48.ttf"}
+      ]
+    }
+  }
+  ```
+- Load them:
+  ```c
+  static GBitmap *s_background_bitmap;
+  static BitmapLayer *s_background_layer;
+
+  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND);
+  s_background_layer = bitmap_layer_create(bounds);
+  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
+  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
+
+  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_TIME_48));
+  text_layer_set_font(s_time_layer, s_time_font);
+  ```
+- Destroy in `main_window_unload` with `gbitmap_destroy` and `fonts_unload_custom_font`.
+
+### 5. Handle Round vs. Rectangular Layouts
+Use macros for per-platform geometry:
+```c
+#ifdef PBL_ROUND
+  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
+#endif
+```
+Or rely on `PBL_IF_ROUND_ELSE(round_value, rect_value)` for coordinates, padding, and font choices. Test on both `basalt` and `chalk` emulators.
+
+---
+
+## Designing for Every Pebble
+Adopt the platform guidelines from the "Building for Every Pebble" best practices doc:
+- **Responsive Layouts**: Derive positions from `layer_get_bounds(window_layer)` and avoid hard-coded values. Provide safe margins (`content inset`) on round displays so text stays within the visible area.
+- **Color Usage**: Basalt and newer watches support 64 colors; earlier models are monochrome. Offer fallback palettes by wrapping color code in `#ifdef PBL_COLOR` checks and choose high-contrast combinations.
+- **Performance**: Keep frame rate low—update the UI only when necessary. Avoid continuous timers; rely on system ticks (minute/hour) and event callbacks. When animating, reuse buffers.
+- **Memory**: Release `GBitmap` and `GFont` objects when not displayed. Combine multiple small icons into one sprite sheet to reduce overhead.
+- **Power Efficiency**: Limit vibrations, keep background updates minimal, and honor Quiet Time (check `quiet_time_is_active()`).
+- **Accessibility**: Use large fonts, maintain contrast, and ensure critical information is available in both round and rectangular layouts.
+
+---
+
+## Working with Graphics and Resources
+The Graphics module provides primitives for rich visuals:
+- **Drawing in `LayerUpdateProc`**:
+  ```c
+  static void canvas_update_proc(Layer *layer, GContext *ctx) {
+    GRect bounds = layer_get_bounds(layer);
+    graphics_context_set_fill_color(ctx, GColorBlack);
+    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
+
+    graphics_context_set_stroke_color(ctx, GColorJaegerGreen);
+    graphics_draw_circle(ctx, grect_center_point(&bounds), 60);
+  }
+  ```
+- **Text Rendering**: `graphics_draw_text` for custom layout, or `TextLayer` convenience functions. System fonts are available via `fonts_get_system_font`.
+- **Images**: Use `gbitmap_create_with_resource` and `bitmap_layer_set_compositing_mode` (`GCompOpSet`, `GCompOpAnd`, etc.) to control blending.
+- **Paths (`GPath`)**: Describe vectors with `GPathInfo` vertices, then use `gpath_draw_outline` and `gpath_draw_filled`. Rotate with `gpath_rotate_to`.
+- **Palettes**: For color bitmaps, you can change palette entries at runtime with `gbitmap_set_palette`. Keep palette size <= 64 colors.
+- **Resource loading**: Prefer `resource_get_handle` with `fonts_load_custom_font` or `resource_load` for raw data.
+
+---
+
+## Animating Layers and Vector Paths
+### Property Animations
+- Animate positions, sizes, and offsets with `PropertyAnimation`:
+  ```c
+  GRect start = GRect(0, 168, 144, 20);
+  GRect finish = GRect(0, 120, 144, 20);
+  PropertyAnimation *anim = property_animation_create_layer_frame(text_layer_get_layer(s_time_layer), &start, &finish);
+  animation_set_duration((Animation *)anim, 600);
+  animation_schedule((Animation *)anim);
+  ```
+
+### Frame/Timer-Based Animations
+- Use `AppTimer` to cycle through frames for sprite animations. Keep intervals coarse (100–200 ms) to conserve battery.
+
+### Vector Animations (Advanced Tutorial)
+1. Define multiple `GPathInfo` frames representing each key shape.
+2. Store them in an array and create a `GPath` to mutate.
+3. On each animation step (via `Animation` or timer), interpolate vertices or swap the active path.
+4. Call `layer_mark_dirty(canvas_layer)` to redraw.
+5. Clean up with `gpath_destroy` during unload.
+
+Keep vector complexity low to maintain smooth performance. Prefer filled shapes over numerous thin lines.
+
+---
+
+## Connecting to Device and Companion Services
+- **TickTimer Service**: `tick_timer_service_subscribe` for minute/hour/day ticks. Always unsubscribe in `deinit`.
+- **Battery Service**: `battery_state_service_peek()` for synchronous reads; subscribe for live updates.
+- **Connection Service**: `connection_service_peek_pebble_app_connection()` checks Bluetooth at startup.
+- **Health Service**: Query step counts, sleep, and heart rate (on supported models). Call `health_service_events_subscribe` and guard with `#ifdef PBL_HEALTH`.
+- **AppMessage**: Exchange data with a phone companion app or web endpoint. Configure inbox/outbox sizes in `app_message_open`. Use dictionaries (`TupletInteger`, etc.) to encode messages.
+- **Timeline Pins**: For dynamic content, send timeline pins from a server using Rebble Web APIs. Watchfaces can react to timeline events to refresh data.
+
+Always debounce updates; for example, throttle weather requests to every 30 minutes.
+
+---
+
+## Configuration, Persistence, and Localization
+- **Configuration pages**: Host an HTML/JS page that posts JSON data back to the watchface. Use the `pebble-kit.js` bridge or `Clay` library to simplify.
+- **Persisting data**: Store settings or cached weather with `persist_write_*` and retrieve with `persist_read_*`. Use defined keys in an enum.
+- **Localization**: Wrap user-visible strings in translation tables. Use separate resource IDs or compile-time flags for alternate languages. Respect 12/24-hour formats automatically via `clock_is_24h_style()`.
+- **User preferences**: Provide toggles for color themes, temperature units, or complications. Validate configuration payloads in the inbox callback.
+
+---
+
+## Debugging, Testing, and Optimization
+- **Logging**: `APP_LOG(level, "message %d", value);` supports `DEBUG`, `INFO`, `WARNING`, `ERROR`. Logs appear in the terminal while running `pebble install --logs`.
+- **Crash diagnosis**: Use the backtrace printed in logs. Check for null pointers, invalid layer bounds, or resource leaks.
+- **Unit testing**: The SDK does not provide built-in unit tests, but you can isolate logic into pure C functions and test externally.
+- **Profiling**: Watch for dropped frames or sluggish animations. Simplify drawing or pre-render assets when possible.
+- **Testing matrix**:
+  - Emulators: `pebble install --emulator basalt/chalk/diorite/emery`.
+  - Hardware: verify brightness, ambient light, and button behavior (if interactive).
+  - Modes: 12h vs 24h, Bluetooth disconnects, low battery, Quiet Time, airplane mode.
+  - Edge cases: long month names in localized languages, leap years, daylight saving transitions.
+
+---
+
+## Package, Publish, and Maintain
+1. **Prepare metadata**: Update `package.json` with meaningful `name`, `shortName`, `longName`, `versionLabel`, `capabilities`, and `releaseNotes`.
+2. **Build the bundle**: `pebble build` outputs `build/my-watchface.pbw`.
+3. **Test the PBW**: Install on all targets (`pebble install --emulator basalt --pbw build/my-watchface.pbw`).
+4. **Create store assets**: Screenshots for rectangular (144×168) and round (180×180) devices, plus a short looping GIF demonstrating animations.
+5. **Publish**: Submit via the Rebble appstore portal, attaching release notes and configuration instructions.
+6. **Support users**: Monitor crash reports, respond to reviews, and ship updates. Increment `versionLabel` for every release.
+7. **Open source**: Include licensing, changelog, and contribution guidelines if sharing publicly.
+
+---
+
+## Reference Cheat Sheet
+- **Core headers**: `<pebble.h>` includes all modules. For finer control import from `pebble_fonts.h`, `gpath.h`, etc.
+- **Useful macros**:
+  - `PBL_IF_COLOR_ELSE(color_value, fallback)`
+  - `PBL_IF_RECT_ELSE(rect_value, round_value)`
+  - `STATUS_BAR_LAYER_HEIGHT`
+- **Service APIs**:
+  - Time: `TickTimerService`
+  - Sensors: `HealthService`, `AccelerometerService`
+  - Communication: `AppMessage`, `DictionaryIterator`
+- **Lifecycle hooks**:
+  - `window_set_window_handlers`
+  - `window_stack_push`
+  - `layer_mark_dirty`
+- **Resource helpers**:
+  - `resource_get_handle`
+  - `gbitmap_sequence_create_with_resource` for animated PNG sequences
+  - `gdraw_command_sequence_create_with_resource` for vector commands
+
+Keep iterating on design, testing across hardware, and referencing the official guides for deeper dives into each topic. With this handbook, you have a consolidated roadmap from first compile to a polished, store-ready Pebble watchface.
