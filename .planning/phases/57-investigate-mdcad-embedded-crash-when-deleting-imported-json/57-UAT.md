---
status: complete
phase: 57-investigate-mdcad-embedded-crash-when-deleting-imported-json
source:
  - 57-01-SUMMARY.md
  - 57-02-SUMMARY.md
  - 57-03-SUMMARY.md
  - 57-04-SUMMARY.md
started: 2026-05-29T23:59:00Z
updated: 2026-05-29T23:59:00Z
---

## Current Test

[testing complete]

## Tests

### 1. Avalonia full host plain startup-import delete smoke
expected: Launch the full Avalonia host with the bundled proof JSONL, keep live refresh off, delete the startup-imported root anchor and then the first imported geometry child, and mdCAD stays attached with no unexpected-session-loss warning/detail.
result: pass

### 2. Avalonia full host live-refresh startup-import delete smoke
expected: Launch the full Avalonia host with the bundled proof JSONL, turn live refresh on, delete the startup-imported root anchor and then the first imported geometry child, and mdCAD stays attached with no unexpected-session-loss warning/detail.
result: pass

### 3. Avalonia minimal sealed startup-import delete smoke
expected: Launch the minimal Avalonia host on its configured startup JSONL, let sealed auto-start complete, delete the startup-imported root anchor and then the first imported geometry child, and the sealed warning surface stays hidden because no crash occurs.
result: pass

### 4. WPF full host plain startup-import delete smoke
expected: Launch the full WPF host with the bundled proof JSONL, keep live refresh off, delete the startup-imported root anchor and then the first imported geometry child, and mdCAD stays attached with no unexpected-session-loss warning/detail.
result: pass

### 5. WPF full host live-refresh startup-import delete smoke
expected: Launch the full WPF host with the bundled proof JSONL, turn live refresh on, delete the startup-imported root anchor and then the first imported geometry child, and mdCAD stays attached with no unexpected-session-loss warning/detail.
result: pass

### 6. WPF minimal sealed startup-import delete smoke
expected: Launch the minimal WPF host on its configured startup JSONL, let sealed auto-start complete, delete the startup-imported root anchor and then the first imported geometry child, and the sealed warning surface stays hidden because no crash occurs.
result: pass

## Summary

total: 6
passed: 6
issues: 0
pending: 0
skipped: 0
blocked: 0

## Gaps

None.
