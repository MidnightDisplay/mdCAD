# Math Backend Decision

## Summary

- Selected backend: cglm 0.9.6
- Reopen triggers: build-system blocker or license blocker only
- Fallback posture: no active fallback candidate
- Adoption model: direct cglm use with a thin project-owned convention/config entrypoint
- API posture: struct or array cglm API may be chosen per subsystem

## Scope

This record locks the Phase 1 backend decision for the current milestone. It exists to keep roadmap, requirements, and implementation work aligned with the direct-adoption path described in the phase context.

## Consequences

- Phase planning and execution should assume direct cglm adoption rather than a wrapper-first migration path.
- Any future reconsideration requires a hard build-system blocker or a license blocker, not a preference change or an untested fallback idea.
- Subsystems may choose the cglm struct or array API based on local code shape as long as they follow the shared project math contract.
