## Windows Vulkan Gate Provenance

- TimestampUTC: 2026-04-02T14:58:25.3823280Z
- SourceCommit: b5cc86df145b9dcbb358620f0f54ed0baf481a3c
- BuildCommand: cmake --build build-vulkan --config Release --target mdCAD
- TestCommand: ctest --test-dir build-vulkan -C Release --output-on-failure
- EvidenceFiles: gate-build-mdcad.txt, gate-ctest-full.txt
- Notes: Evidence captures raw command outputs and explicit exit codes for audit traceability.
