# PLY Loader Progress Bar - Design Document

**STATUS: IMPLEMENTED (2026-02-04)**

## Problem Statement

Loading large PLY files (50k+ points) as Editable Subtree causes the application to freeze for several seconds with no user feedback. Users need visual progress indication and the ability to understand that work is happening.

## Analysis: Where Time Is Spent

### Point Cloud Node Mode (Fast)
1. **File I/O + Parsing**: ~90% of time - Reading and parsing vertex data
2. **Entity Creation**: ~10% - Single entity with point cloud geometry
3. **GPU Upload**: Negligible - Single batch allocation

### Editable Subtree Mode (Slow)
1. **File I/O + Parsing**: ~10% of time
2. **Entity Creation**: ~80% - Creating N individual ECS entities with components
3. **Parent-Child Setup**: ~10% - Establishing hierarchy relationships

**Key Insight**: For Editable Subtree, the bottleneck is ECS entity creation which MUST happen on the main thread (Flecs/ImGui/Sokol constraints).

## Available Infrastructure

### Threading Primitives (via Flecs)
```c
// Cross-platform threads
ecs_os_thread_t thread = ecs_os_thread_new(worker_fn, arg);
ecs_os_thread_join(thread);

// Mutexes
ecs_os_mutex_t mutex = ecs_os_mutex_new();
ecs_os_mutex_lock(mutex);
ecs_os_mutex_unlock(mutex);
ecs_os_mutex_free(mutex);

// Condition variables
ecs_os_cond_t cond = ecs_os_cond_new();
ecs_os_cond_signal(cond);
ecs_os_cond_wait(cond, mutex);
```

### Sokol Fetch (Async File I/O)
Already included in project. Provides:
- Background file loading with progress callbacks
- Cross-platform (native threads on desktop, async JS on web)
- Callbacks invoked on main thread via `sfetch_dowork()`

### Platform Support Matrix
| Platform | Threads | Sokol Fetch | Recommendation |
|----------|---------|-------------|----------------|
| macOS    | pthread | Yes         | Full support   |
| Windows  | Win32   | Yes         | Full support   |
| Linux    | pthread | Yes         | Full support   |
| iOS      | pthread | Yes         | Full support   |
| Android  | pthread | Yes         | Full support   |
| Web      | Limited | Async JS    | Chunked only   |

## Proposed Solutions

### Option A: Chunked Main-Thread Processing (Recommended)

**Concept**: Split entity creation across multiple frames, updating progress bar between chunks.

**Advantages**:
- No threading complexity
- Works on ALL platforms including Web
- ImGui-safe (all work on main thread)
- Can be cancelled mid-operation

**Disadvantages**:
- Doesn't parallelize file parsing
- UI still processes chunks (brief micro-stutters)

**Architecture**:
```
┌─────────────────────────────────────────────────────────┐
│                     Frame Loop                          │
├─────────────────────────────────────────────────────────┤
│  Frame N:   Parse PLY header, allocate arrays           │
│  Frame N+1: Parse vertices 0-999                        │
│  Frame N+2: Parse vertices 1000-1999                    │
│  ...                                                    │
│  Frame N+K: Parse complete, start entity creation       │
│  Frame N+K+1: Create entities 0-499                     │
│  Frame N+K+2: Create entities 500-999                   │
│  ...                                                    │
│  Frame N+M: Complete, close progress popup              │
└─────────────────────────────────────────────────────────┘
```

**Implementation Outline**:

```c
// New file: src/ply_import_job.h

typedef enum {
    PLY_JOB_IDLE,
    PLY_JOB_PARSING_HEADER,
    PLY_JOB_PARSING_VERTICES,
    PLY_JOB_CREATING_ENTITIES,
    PLY_JOB_COMPLETE,
    PLY_JOB_ERROR
} ply_job_state_t;

typedef struct {
    ply_job_state_t state;

    // File state
    FILE *file;
    char filepath[512];
    ply_header_t header;

    // Parsed data (grows during parsing)
    vec3_t *points;
    vec4_t *colors;
    int parsed_count;
    int total_count;

    // Entity creation state
    int created_count;
    ecs_entity_t root_entity;

    // Import options
    int import_mode;      // 0 = Point Cloud, 1 = Editable
    float scale;
    float point_size;
    vec4_t default_color;
    bool use_ply_colors;

    // Progress (0.0 - 1.0)
    float progress;
    char status_message[128];

    // Error state
    ply_error_t error;
} ply_import_job_t;

// Called each frame - processes one chunk of work
// Returns true when job is complete (success or error)
bool ply_import_job_tick(ply_import_job_t *job, ecs_scene_t *scene);

// Start a new import job
void ply_import_job_start(ply_import_job_t *job, const char *filepath, ...options...);

// Cancel an in-progress job
void ply_import_job_cancel(ply_import_job_t *job);
```

**Chunk Sizes** (tunable):
- Vertex parsing: 5000 vertices/frame (~16ms budget at 60fps)
- Entity creation: 500 entities/frame (heavier due to ECS overhead)

---

### Option B: Background Thread + Main Thread Finalization

**Concept**: Parse file in background thread, create entities on main thread in chunks.

**Advantages**:
- File parsing doesn't block UI at all
- Better perceived performance
- Can show "Loading file..." then "Creating entities..." phases

**Disadvantages**:
- More complex synchronization
- Doesn't work on Web (Emscripten)
- Thread safety concerns with error handling

**Architecture**:
```
┌──────────────────┐     ┌──────────────────┐
│  Worker Thread   │     │   Main Thread    │
├──────────────────┤     ├──────────────────┤
│ Open file        │     │ Show "Loading"   │
│ Parse header     │────▶│ Update progress  │
│ Parse vertices   │     │ Update progress  │
│ Signal complete  │────▶│ Start entities   │
│ (thread exits)   │     │ Create batch     │
│                  │     │ Create batch     │
│                  │     │ ... (chunked)    │
│                  │     │ Complete         │
└──────────────────┘     └──────────────────┘
```

**Synchronization**:
```c
typedef struct {
    ecs_os_mutex_t mutex;
    ecs_os_thread_t thread;

    // Protected by mutex
    float progress;
    bool parsing_complete;
    bool cancelled;
    ply_error_t error;

    // Owned by worker until parsing_complete
    ply_data_t data;
} ply_threaded_job_t;
```

---

### Option C: Sokol Fetch for File I/O

**Concept**: Use sokol_fetch for async file reading, then process on main thread.

**Advantages**:
- Clean async API
- Handles platform differences automatically
- Progress via response callbacks

**Disadvantages**:
- Sokol fetch is for streaming chunks, not progress tracking
- Still need chunked processing for entity creation
- Adds dependency on sokol_fetch initialization

**Note**: This could be combined with Option A for best results.

---

## Recommended Implementation: Option A (Chunked Processing)

### Phase 1: Core Job System

**New Files**:
- `src/ply_import_job.h` - Job state machine and tick function

**Modified Files**:
- `src/ui/ui_scene_hierarchy.h` - Replace synchronous import with job system
- `src/ply_loader.h` - Add incremental parsing functions

### Phase 2: UI Integration

**Progress Popup Design**:
```
┌─────────────────────────────────────────┐
│ Importing PLY Point Cloud               │
├─────────────────────────────────────────┤
│ File: scan_50k.ply                      │
│                                         │
│ [██████████████░░░░░░░░░░░░░] 47%       │
│                                         │
│ Parsing vertices: 23,500 / 50,000       │
│                                         │
│              [Cancel]                   │
└─────────────────────────────────────────┘
```

**Progress Bar Phases**:
1. **0-30%**: Parsing vertices
2. **30-100%**: Creating entities (Editable mode only)

For Point Cloud Node mode, entity creation is instant, so:
1. **0-95%**: Parsing vertices
2. **95-100%**: Creating point cloud entity

### Phase 3: Cancellation Support

- User can click "Cancel" to abort
- Job cleans up partial state (free parsed data, delete partial entities)
- Scene remains in consistent state

## API Design

### Starting an Import
```c
// In ui_scene_hierarchy.h, replace:
ply_data_t ply_data;
ply_error_t err = ply_load_file(path, &ply_data);
// ... synchronous entity creation ...

// With:
ply_import_job_start(&state->import_job, path, options);
state->import_in_progress = true;
```

### Per-Frame Processing
```c
// In ui_scene_hierarchy_draw():
if (state->import_in_progress) {
    // Draw progress popup
    ui_draw_import_progress_popup(state);

    // Process one chunk
    if (ply_import_job_tick(&state->import_job, state->scene)) {
        // Job complete
        state->import_in_progress = false;
        if (state->import_job.error == PLY_OK) {
            state->cache_dirty = true;
            snprintf(state->last_status, ...);
        } else {
            snprintf(state->last_status, "Error: %s", ...);
        }
    }
}
```

### Incremental PLY Parser
```c
// New functions in ply_loader.h:

// Open file and parse header only
ply_error_t ply_open(const char *filepath, ply_parse_state_t *state);

// Parse up to max_vertices, returns number parsed
// Call repeatedly until returns 0 or error
int ply_parse_vertices_chunk(ply_parse_state_t *state, int max_vertices);

// Get current progress (0.0 - 1.0)
float ply_get_progress(ply_parse_state_t *state);

// Close and cleanup
void ply_close(ply_parse_state_t *state);
```

## Performance Targets

| File Size | Point Cloud Mode | Editable Mode |
|-----------|------------------|---------------|
| 1k points | <1 frame         | <1 frame      |
| 10k points| <5 frames        | ~20 frames    |
| 50k points| ~20 frames       | ~100 frames   |
| 100k points| ~40 frames      | ~200 frames   |

At 60fps:
- 50k Point Cloud: ~0.3 seconds with smooth progress
- 50k Editable: ~1.7 seconds with smooth progress

## Edge Cases

1. **User closes app during import**: Job cleanup in `ui_scene_hierarchy_shutdown()`
2. **User starts new import while one is running**: Cancel current, start new
3. **File disappears mid-parse**: Error state with cleanup
4. **Out of memory during parsing**: Error with partial cleanup
5. **Very small files (<1000 points)**: Skip progress UI, import synchronously

## Testing Plan

1. **Unit tests**: `ply_parse_vertices_chunk()` with various chunk sizes
2. **Integration tests**: Full import with progress verification
3. **Stress tests**: 1M point file, verify no memory leaks on cancel
4. **Platform tests**: Verify on macOS, Windows, Web

## Future Enhancements

1. **Background thread parsing** (Option B) for native platforms
2. **Streaming large files** that don't fit in memory
3. **Binary PLY support** with same chunked approach
4. **Import queue** for multiple files

## Files to Create/Modify

### New Files
- `src/ply_import_job.h` - Import job state machine

### Modified Files
- `src/ply_loader.h` - Add incremental parsing API
- `src/ui/ui_scene_hierarchy.h` - Add job state, progress UI, chunked processing

## Implementation Order

1. Add `ply_parse_state_t` and incremental parsing to `ply_loader.h`
2. Create `ply_import_job.h` with state machine
3. Add progress popup UI to `ui_scene_hierarchy.h`
4. Replace synchronous import with job-based import
5. Add cancellation support
6. Test and tune chunk sizes

## Open Questions (Updated with final decisions)

1. Should we show a modal popup (blocking other UI) or a non-modal progress indicator?
   - **Recommendation**: Modal popup with Cancel button - prevents user from starting conflicting operations
   - **Final Decision**: Modal popup is the best approach for us

2. Should small files (<1000 points) skip the progress UI entirely?
   - **Recommendation**: Yes - use a threshold, import synchronously if below
   - **Final Decision**: Yes, having threshold is reasonable. Let's have that as a constant int defined in the source code

3. Should we remember the last used import options?
   - **Recommendation**: Yes - already storing most options in hierarchy state
   - **Final Decision**: Agreed. Yes, please!
