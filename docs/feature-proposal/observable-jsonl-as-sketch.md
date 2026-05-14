## Introduction

Currently, mdCAD supports adding 3D sketches to the scene. It also allows the user to parse and import bare geometry data from a *.jsonl file and dump it top level into the root-level scene.

## Feature description

We want a new File menu item that imports *.jsonl data into a sketch. Some special handling for this type of import:

    1. Import Line3D as sketch line geometry (same API as Add Line from the active sketch workspace), Arc3D as arc geometry as arc geometry (same API as Add Arc from the active sketch workspace) and Circle3D as arc full circle arc geometry (same API as Add Circle from the active sketch workspace). following the same logic Point3D as point (add point API from sketch workspace).
    2. Geometry log entries are flattened into a single sketch.
    3. Import colors for each entity and set these as the color of the geometry component of the sketch geometry's ECS entity.
    4. Polyline3D and Polygon3D are converted to separate lines.
    5. Meshes are ignored completely.
    6. JSONL does not support constraints and this feature does not have to add any extra constraints - just leave geometries uncostrained as they are.

Another, optional (opt out) feature during import, when on:
    
    1. Keep the JSONL file observable until the sketch or the link is broken.
    2. The Sketch entity gains a new extra ECS component with observability controls, called JsonlObserverComponent.
    3. JsonlObserverComponent has the following controls:
        b. 'Observe file' toggle - when ON the component periodically checks when the JSONL file has been modified, and re-parses that if possible (the file is not blocked from read access). Defaults to ON
        c. 1-1000 ms integer slider to control the rate limiter. Defaults to 100ms
        d. A button to manually re-parse on user's demand.
        e. An small area (two lines) for error/warning messagging - showing if anything is wrong with reading the file (file no longer found, parse errors/warnings).
        f. some other relevant JSONL import settings, like the scale, rotation and whether to shift to center of mass (these are defaulted to whatever was chosen during the initial import).
    4. JSONL files can be hogged up by a writer process (usually a C# script routine) for a while - the observer should note that the file has changed, and if it's still locked - retry a couple of times with debounce until it is available. Let's hardcode the retries to 50 for now.

Important notes:

    This is a proper sketch feature it must have an accompanying script with live link. As if the user did a Add Entity -> Create Sketch from the menu. The sketch Label component's name is the file name of the jsonl (no file extension). The Label component's description is the full file path to the jsonl file.