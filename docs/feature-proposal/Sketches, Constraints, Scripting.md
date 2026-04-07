# Sketches, Constraints, Scripting

This document describes a sizeable new feature pack for mdCAD.

## Sketches

ECS based entity that represents an aggregation of geometry entities solved to a state that satisfies all applied geometric constraints. Composed of existing Label, Transform components and new, to be added, components:

1. SketchManager - keeps track of the overall sketch solve status (solved, loose, fixed, solver errors with reasons), provides a control to set sketch color - applies to all geometries in the sketch (geometry entities override this color by having their color set to any RGB color except 0,0,0 - black).

2. GeometryManager - keeps track of the child geometry entitites that belong to the sketch, shows full count and a list of geometries along with their status (fixed, loose, constrained) in the UI. Allows fixing geometries (setting the geometric entity and its sub-entities to 'fixed' (immovable)) and deleting geometries, both as a select/multiselect + action buttons. Works in tandem with the next component.

3. ConstraintManager - same as geometry manager but keeps track and lits out the sketch constraints. When a constraint selected - shows which geometry entities participate in that constraint.

4. Solver - sketch solver control - auto-solve on/off, re-calculate by button press, show log with INFO, ERROR, WARNING. Solver type selection (implement just one type for the first milestone).

Geometries allowed in the sketch will be limited to points, lines and arcs(+circles) for the first milestone.

We want to see all useful 3D constraints - FIXED, COINCIDENT, COLLINEAR, PARALLEL, PERPENDICULAR, ALONG X, ALONG Y, ALONG Z, CORADIAL, CONCENTRIC, LENGTH, ANGLE, TANGENTIAL. These constraints are only legal for entity types that do make sense geometrically.

## UI

The component UI panels will integrate into the imgui-based Entity Inspector already present in mdCAD.

The constraints will have to be rendered in the viewport as small glyphs composed of lines and arcs and rendered flat to the screen, at a constant zoom level (akin to the 3d manipulation gizmo) located next to the entity (or sub-entity) it's applied to. These must be hoverable and selectable. To add a constraints we will need a new in-context UI menu, that can be shown/hidden by pressing Tab key, the menu will list out applicable constraints as clickable buttons (mirroring the 3D viewport glyphs), once applied the menu will auto-hide and solver will do it's thing (unless auto-solve is off).

Length and Angle contraints should render a glyph with a value and CAD-style dimension with arrows and leaders. Double-clicking the glyph reveals and in-context menu to modify and accept the value. The value must be also mirrored in the ConstraintManager when the constraint is selected. Length and Angle can be set to driven - meaning they do not contribute to the solver, just serve as a label for the actual in-sketch value.

## Scripting

Along with the sketches we want to further develop and feature-enrich our scene API. All of the geometry, sketch and contraint related activities must have an API. We also want a nice functional way of scripting that will work in a dynamic bi-directional manner:

1. In sketch manager have a button that reveals the script editor window for the sketch (standalone, not tied to Enitity Inspector). The script represents the sketch in a functional way. Entities are declared. Entities are added to the sketch node. Contraints and and their values are applied to entities. Scripting language choice is up for discussion - however its runtime must be easily packageable into the compiled C output of the app.

2. Any changes via the UI must update the script.

3. Sketch Sub-Scene must be serializable to the script and reconstructable from parsing the script.

4. Script can define some constants and variables - Input and Output. Script will have it's frontend for Input and Output variables. Input and Output variables will be dynamically parsed into UI numeric inputs on the frontend panel. Optionally the parameters can be attributed with (Min, Max, Step) values - these will inform the parser to render them as numeric input plus a slider, both bound to min/max constraints. Frontend and script have a bidirectional link (imgui should be able to handle this by default, being an immediate mode UI). Script allows math calculations and equations for intermediate values. Outputs will just show the read-out values, such as the length/angle of driven elements, or computed variables that are calculated from them or their various mathematical combinations.

## Some additional notes

- Entt for ECS - building on whats already there. Agressive refactoring/restructuring is permitted.
- Cimgui for flat UI - building on whats already there.
- Solver - to be researched and locked down but preferrably something off the shelf, C and single header library.
- Scripting engine - not sure about this one, but hopefully something lightweight, off the shelf, and perhaps functional, but Lua is also good. Since we target MacOS and Windows now, and will expand to iOS and Web (emscripten) in the future - ability to run on these platforms is a must.
- All off the shelf libraries must be open-source with a commercially-permissive non-viral license (MIT - good, GPL - bad). If no good library candidate is found - we need to create our own.

## Example case study

To help with development and debugging I propose we have a sketch test case that draws something geometrically cool along with the script that produces that. Should be something matematically elegant and showcasing a set of constraints (not necessarily all of them). Maybe even a list of examples that focus on one constraint type each, and come with sensible inputs and outputs for the frontend.

## Improvements as we go along

- Any sensible refactors to existing code to facilitate the new systems are fully welcomed. Undo system will likely need to be expanded, refactored and unified to support the sketch, the script and the solver. Manipulation gizmo and transforms will have to be imporoved to respect the constraints. Currently the project is in early stages and we do not have any legacy to support and be backwards-compatible with. Full mutations are allowed as long as they do not degrade the visual experience.

## Platform directions

- Target Windows MSVC + Vulkan during the development and testing run. MacOS parity will be tested in one swoop after all the features are implemented and pass on Windows.





