# Testing Patterns

**Analysis Date:** 2026-03-24

## Test Framework

**Runner:**
- There is no formal C/C++ unit test runner configured in the repository.
- `CMakeLists.txt` and `src/CMakeLists.txt` define application builds only; there is no `enable_testing()` or `add_test()` usage.
- The only scripted browser automation currently checked in is Node + Puppeteer in `scripts/debug-wasm.mjs` and `scripts/test-imgui.mjs`.

**Assertion Library:**
- No dedicated assertion library is configured for native code or Node scripts.
- Browser scripts rely on direct DOM/page interaction, console output, and ad hoc checks rather than a full expect/expectation stack.

**Run Commands:**
```bash
cmake -B build -G Ninja && ninja -C build
./build/bin/mdCAD

cd scripts && npm install
node scripts/debug-wasm.mjs build-web/bin/mdCAD.html 5000 --screenshot
node scripts/test-imgui.mjs --visible

emcmake cmake -B build-web && cmake --build build-web
cd android && ./gradlew assembleDebug
```

## Test File Organization

**Location:**
- There is no `tests/` tree or `*.test.*` pattern in the repo.
- Test-like automation lives in `scripts/`, with artifacts written to `scripts/test-output/`.
- Native verification is build-and-run driven, with the current machine path centered on Ninja builds in `build`.

**Naming:**
- Web automation is named by purpose, for example `scripts/debug-wasm.mjs` and `scripts/test-imgui.mjs`.
- Input/data generation helpers also live in `scripts/`, such as `generate_ply_grid.py`, `generate_ply_text.py`, and `obj_to_colored_ply.py`.

**Structure:**
```text
src/
  app.c
  ui/
    ui_scene_hierarchy.h
scripts/
  debug-wasm.mjs
  test-imgui.mjs
  test-output/
```

## Test Structure

**Suite Organization:**
```javascript
// Representative pattern from scripts/test-imgui.mjs
const browser = await puppeteer.launch({ headless: true, args: [...] });
const page = await browser.newPage();
page.on('console', msg => console.log(msg.text()));
page.on('pageerror', err => console.log(err.message));

await page.goto(`file://${htmlFile}`, { waitUntil: 'networkidle0' });
await page.mouse.click(270, 47);
await page.screenshot({ path: 'scripts/test-output/test-5-color-picker.png' });
```

**Patterns:**
- Tests and debug flows are mostly end-to-end browser runs against the wasm build.
- Setup is ad hoc and inline, usually with helper functions for drag, screenshot, or pixel sampling.
- Cleanup is explicit with `await browser.close()`, and scripts create output directories on demand.

## Mocking

**Framework:**
- No mocking framework is configured.
- Browser scripts drive the real compiled app instead of substituting mocked subsystems.

**Patterns:**
```javascript
// Representative helper pattern from scripts/debug-wasm.mjs
page.on('console', msg => {
    console.log(`[${msg.type().toUpperCase()}] ${msg.text()}`);
});

page.on('requestfailed', req => {
    console.log(`${req.url()} - ${req.failure()?.errorText}`);
});
```

**What to Mock:**
- Nothing is formally mocked today.
- If coverage expands, external browser/runtime dependencies are the main candidates, not core app modules.

**What NOT to Mock:**
- Do not mock core scene, ECS, or GPU paths unless a future narrow unit test requires isolation.
- For current verification, prefer the real wasm/native build path and keep the interaction as close to user behavior as possible.

## Fixtures and Factories

**Test Data:**
```text
scripts/
  generate_ply_grid.py
  generate_ply_text.py
  obj_to_colored_ply.py
  embed_gcode.py
```

**Location:**
- Fixture generation is script-based rather than fixture-directory based.
- Browser artifacts and screenshots are saved under `scripts/test-output/`, which is gitignored.

## Coverage

**Requirements:**
- No coverage threshold or enforcement is configured.
- Formal unit/integration coverage is sparse compared with manual and scripted verification.

**Configuration:**
- There is no coverage tool or report target checked in for C/C++, Node, or Python.
- Verification is mostly compile-time, runtime smoke testing, and visual inspection.

**View Coverage:**
```bash
# Not currently available as a repo command
cmake -B build -G Ninja && ninja -C build
node scripts/debug-wasm.mjs --screenshot
```

## Test Types

**Unit Tests:**
- Not currently present as checked-in tests.
- Pure helpers in `src/math3d.h` and small component headers are good candidates for future unit coverage.

**Integration Tests:**
- Build validation is integration-oriented: configure with CMake, compile with Ninja or Gradle, and run the app.
- The native validation path for this machine is Ninja builds in `build`; use that path as the baseline when checking regressions locally.

**E2E Tests:**
- Puppeteer scripts in `scripts/` provide the closest thing to end-to-end automation today.
- `scripts/test-imgui.mjs` exercises ImGui interaction in the wasm build, while `scripts/debug-wasm.mjs` captures runtime behavior and screenshots.

## Common Patterns

**Async Testing:**
```javascript
await page.goto(`file://${htmlFile}`, { waitUntil: 'networkidle0' });
await new Promise(r => setTimeout(r, 500));
await page.screenshot({ path: screenshotPath });
```

**Error Testing:**
```javascript
try {
    await page.goto(fileUrl, { waitUntil: 'networkidle0', timeout: 30000 });
} catch (err) {
    console.error(`[FATAL] ${err.message}`);
}
```

**Snapshot Testing:**
- Not used as a formal framework.
- Screenshots in `scripts/test-output/` act as the practical visual artifact for regression inspection.

*Testing analysis: 2026-03-24*
*Update when test patterns change*
