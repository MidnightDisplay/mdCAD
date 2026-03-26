# Phase 6 Scene Converter Sample Report

## Converter Execution

- command: `python scripts/scene_format_convert.py --input C:\Users\RodionRadchenko\AppData\Local\Temp\mdcad_phase6_legacy.json --output C:\Users\RodionRadchenko\AppData\Local\Temp\mdcad_phase6_converted.json`
- input path: `C:\Users\RodionRadchenko\AppData\Local\Temp\mdcad_phase6_legacy.json`
- output path: `C:\Users\RodionRadchenko\AppData\Local\Temp\mdcad_phase6_converted.json`
- converter result: pass (`exit_code=0`)
- converted output schema: `format=mdcad-scene`, `version=2`

## Converted-Scene Targeted Validation (D-08)

- converted entity count: `2`
- converted parent links: `{"100":null,"101":100}`
- converted transform fields:
  - `100`: `position=[0.0,0.0,0.0] rotation=[0.0,0.0,0.0] scale=[1.0,1.0,1.0]`
  - `101`: `position=[1.0,2.0,3.0] rotation=[0.1,0.2,0.3] scale=[1.0,1.0,1.0]`
- converted geometry type set: `["point","line"]`

### Field Outcomes

- entity count: **pass**
- parent links: **pass**
- position: **pass**
- rotation: **pass**
- scale: **pass**
- geometry type: **pass**

## Notes

- Converted output satisfies the same D-08 field checks used for representative scene validation.
