import json
from pathlib import Path
from typing import Any, Dict


def load_config(cfg_path) -> Dict[str, Any]:
    with open(cfg_path, "r", encoding="utf-8") as f:
        return json.load(f)