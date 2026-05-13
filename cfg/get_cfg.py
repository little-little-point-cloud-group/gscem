import yaml 
from typing import Any, Dict


def load_yaml_config(cfg_path) -> Dict[str, Any]:
    with open(cfg_path, "r", encoding="utf-8") as f:
        return yaml.safe_load(f)