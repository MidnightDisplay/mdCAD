# Deferred Items (Out of Scope During 35-01 Execution)

1. **Windows linker lock during full Release build**
   - **Observed in:** `cmake --build build-vulkan --config Release`
   - **Symptoms:** `LNK1104` on `build-vulkan/bin/Release/mdCAD.exe` and `build-vulkan/bin/Release/scene_solver_diagnostics.exe`
   - **Why deferred:** Not introduced by this plan's test-scaffold changes; existing runtime file-lock/environment issue outside scoped task files.
   - **Follow-up:** Clear locking processes/handles in the environment, then rerun full Release build gate.

