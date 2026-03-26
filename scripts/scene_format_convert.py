#!/usr/bin/env python3
"""
Convert legacy mdCAD scene JSON (version 1) into the Phase 6 cleaned schema.

Input schema accepted:
- Legacy scene schema: {"version": 1, "entities": [...]}
- Already-converted schema: {"format": "mdcad-scene", "version": 2, "entities": [...]}

Output schema emitted:
- {"format": "mdcad-scene", "version": 2, "entities": [...]}

Non-goals:
- No importer (PLY/JSONL) conversion logic.
- No undo/editor data migration.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any


SCENE_FORMAT = "mdcad-scene"
SCENE_VERSION = 2


def fail(message: str) -> int:
    print(f"error: {message}", file=sys.stderr)
    return 1


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(float(value))


def require_number(value: Any, field: str) -> float:
    if not is_number(value):
        raise ValueError(f"{field} must be a finite number")
    return float(value)


def require_vec(values: Any, count: int, field: str) -> list[float]:
    if not isinstance(values, list) or len(values) != count:
        raise ValueError(f"{field} must be an array of {count} finite numbers")
    return [require_number(v, f"{field}[{i}]") for i, v in enumerate(values)]


def normalize_transform(transform: dict[str, Any]) -> dict[str, list[float]]:
    if not isinstance(transform, dict):
        raise ValueError("components.transform must be an object")
    return {
        "position": require_vec(transform.get("position", [0.0, 0.0, 0.0]), 3, "components.transform.position"),
        "rotation": require_vec(transform.get("rotation", [0.0, 0.0, 0.0]), 3, "components.transform.rotation"),
        "scale": require_vec(transform.get("scale", [1.0, 1.0, 1.0]), 3, "components.transform.scale"),
    }


def normalize_light(light: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(light, dict):
        raise ValueError("components.light must be an object")
    light_type = light.get("type", "directional")
    if not isinstance(light_type, str):
        raise ValueError("components.light.type must be a string")
    return {
        "type": light_type,
        "color": require_vec(light.get("color", [1.0, 1.0, 1.0, 1.0]), 4, "components.light.color"),
        "intensity": require_number(light.get("intensity", 1.0), "components.light.intensity"),
    }


def normalize_entity(entity: Any, index: int) -> dict[str, Any]:
    if not isinstance(entity, dict):
        raise ValueError(f"entities[{index}] must be an object")

    if "id" not in entity:
        raise ValueError(f"entities[{index}].id is required")
    entity_id = int(require_number(entity["id"], f"entities[{index}].id"))

    parent_raw = entity.get("parent", None)
    parent_value: int | None
    if parent_raw is None:
        parent_value = None
    else:
        parent_value = int(require_number(parent_raw, f"entities[{index}].parent"))

    components = entity.get("components")
    if not isinstance(components, dict):
        raise ValueError(f"entities[{index}].components must be an object")

    normalized_components: dict[str, Any] = {}

    if "transform" in components:
        normalized_components["transform"] = normalize_transform(components["transform"])

    if "geometry" in components:
        geometry = components["geometry"]
        if not isinstance(geometry, dict):
            raise ValueError(f"entities[{index}].components.geometry must be an object")
        geom_type = geometry.get("type")
        if not isinstance(geom_type, str) or not geom_type:
            raise ValueError(f"entities[{index}].components.geometry.type must be a non-empty string")
        normalized_components["geometry"] = geometry

    if "renderable" in components:
        renderable = components["renderable"]
        if not isinstance(renderable, dict):
            raise ValueError(f"entities[{index}].components.renderable must be an object")
        normalized_components["renderable"] = renderable

    if "light" in components:
        normalized_components["light"] = normalize_light(components["light"])

    if not normalized_components:
        raise ValueError(f"entities[{index}].components has no supported entries")

    return {
        "id": entity_id,
        "parent": parent_value,
        "components": normalized_components,
    }


def convert_scene(payload: Any) -> dict[str, Any]:
    if not isinstance(payload, dict):
        raise ValueError("top-level JSON must be an object")

    if "entities" not in payload:
        raise ValueError("top-level 'entities' field is required")
    entities = payload["entities"]
    if not isinstance(entities, list):
        raise ValueError("top-level 'entities' must be an array")

    source_format = payload.get("format")
    source_version = payload.get("version")
    is_legacy_v1 = source_format is None and source_version == 1
    is_current_v2 = source_format == SCENE_FORMAT and source_version == SCENE_VERSION

    if not (is_legacy_v1 or is_current_v2):
        raise ValueError(
            "unsupported scene schema: expected legacy {version:1} or current "
            "{format:'mdcad-scene', version:2}"
        )

    normalized_entities = [normalize_entity(entity, i) for i, entity in enumerate(entities)]

    return {
        "format": SCENE_FORMAT,
        "version": SCENE_VERSION,
        "entities": normalized_entities,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert legacy mdCAD scene JSON into cleaned Phase 6 schema (format+version v2)."
    )
    parser.add_argument("--input", required=True, help="Path to input scene JSON file")
    parser.add_argument("--output", required=True, help="Path to output converted scene JSON file")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_path = Path(args.input)
    output_path = Path(args.output)

    if not input_path.is_file():
        return fail(f"input file not found: {input_path}")

    try:
        raw = input_path.read_text(encoding="utf-8")
    except OSError as exc:
        return fail(f"failed to read input file: {exc}")

    try:
        payload = json.loads(raw)
    except json.JSONDecodeError as exc:
        return fail(f"invalid JSON in input file: {exc}")

    try:
        converted = convert_scene(payload)
    except ValueError as exc:
        return fail(str(exc))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        output_path.write_text(json.dumps(converted, indent=2) + "\n", encoding="utf-8")
    except OSError as exc:
        return fail(f"failed to write output file: {exc}")

    print(f"converted: {input_path} -> {output_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
