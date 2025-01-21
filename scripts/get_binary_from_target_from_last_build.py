import sys
from pathlib import Path

import yaml


def get_cbuild(cbuild_idx):
    with open(cbuild_idx, "r") as file:
        data = yaml.safe_load(file)
    builds = data["build-idx"]["cbuilds"]
    return builds[0]["cbuild"]


def parse_cbuild(cbuild):
    cbuild_parent = Path(cbuild).parent
    with open(cbuild, "r") as file:
        data = yaml.safe_load(file)
    output_dir = data["build"]["output-dirs"]["outdir"]
    output_dir = (cbuild_parent / output_dir).absolute()
    outputs = {d["type"]: d["file"] for d in data["build"]["output"]}
    bin = str(Path(output_dir) / outputs["bin"])

    context = data["build"]["context"]
    target = context.split("+")[1].strip()
    return bin, target


if __name__ == "__main__":
    cbuild_idx = sys.argv[1]
    try:
        cbuild = get_cbuild(cbuild_idx)
        bin, target = parse_cbuild(cbuild)
        print(bin, target)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
