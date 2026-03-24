# External Integrations

**Analysis Date:** 2026-03-24

This repository does not communicate with remote runtime services. There are no payment, email/SMS, analytics, authentication, database, or webhook integrations.

## APIs & External Services

**Payment Processing:**
- None

**Email/SMS:**
- None

**External APIs:**
- None

**Build-time source fetches:**
- GitHub repositories for vendored dependencies
  - Integration method: CMake `FetchContent` in `vendors/libsokol/CMakeLists.txt` and `vendors/libcimgui/CMakeLists.txt`
  - Sources: `https://github.com/floooh/sokol.git` and `https://github.com/cimgui/cimgui.git`
  - Auth: none required for public source fetches

## Data Storage

**Databases:**
- None

**File Storage:**
- Local filesystem only on native desktop builds for scene save/load, import/export, and test artifacts
  - Paths used in code/docs include `build/`, `build-web/`, and local asset files such as `mdCAD.png`

**Caching:**
- None

## Authentication & Identity

**Auth Provider:**
- None

**OAuth Integrations:**
- None

## Monitoring & Observability

**Error Tracking:**
- None

**Analytics:**
- None

**Logs:**
- Local stdout/stderr only via the app and build/test scripts

## CI/CD & Deployment

**Hosting:**
- None

**CI Pipeline:**
- None defined in the repository

## Environment Configuration

**Development:**
- `VULKAN_SDK` for Windows Vulkan builds
- Android SDK/NDK for `android/`
- Emscripten toolchain for the web build
- Xcode and Apple SDKs for macOS/iOS builds
- Node.js/npm for `scripts/package.json` and Puppeteer-based local browser checks

**Staging:**
- None

**Production:**
- None

## Webhooks & Callbacks

**Incoming:**
- None

**Outgoing:**
- None

*Integration audit: 2026-03-24*
*Update when adding/removing external services*
