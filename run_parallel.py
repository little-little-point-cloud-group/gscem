#!/usr/bin/env python3
"""
run_parallel.py
~~~~~~~~~~~~~~~~
Read a list of shell commands from a text file and run them all in parallel.
Each command is started in its own subprocess and its output is captured.

Usage:
    python3 run_parallel.py commands.txt
"""

import sys
import subprocess
import concurrent.futures
from pathlib import Path
from typing import List, Tuple

def read_commands(file: Path) -> List[str]:
    """Return a list of non-empty, non-comment lines from the file."""
    lines = []
    with file.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue          # skip blanks / comments
            lines.append(line)
    return lines


def run_one(cmd: str) -> Tuple[int, str, str]:
    """
    Execute a single command in a shell.
    Return (exit_code, stdout, stderr).
    """
    try:
        result = subprocess.run(
            cmd,
            shell=True,                     # run via the shell
            capture_output=True,            # capture stdout & stderr
            text=True,                      # decode to str
            timeout=36000,                   # optional: kill after 1 h
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Command timed out"


def main(commands_file: Path, max_workers: int = None):
    commands = read_commands(commands_file)
    if not commands:
        print("No commands to run.")
        return

    print(f"Launching {len(commands)} command(s) in parallel…")

    # `max_workers` defaults to min(32, os.cpu_count() + 4)
    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
        # Submit all commands
        future_to_cmd = {executor.submit(run_one, cmd): cmd for cmd in commands}

        # As they finish, print the results
        total = len(commands)
        done = 1
        for future in concurrent.futures.as_completed(future_to_cmd):
            cmd = future_to_cmd[future]
            try:
                exit_code, stdout, stderr = future.result()
            except Exception as exc:
                exit_code, stdout, stderr = -1, "", f"Exception: {exc!s}"

            print("\n" + "="*80)
            print(f"Command: {cmd}")
            print(f"Exit code: {exit_code}")
            print(f"done: {done}/{total}")
            if stdout:
                print("\n--- stdout ---")
                print(stdout.rstrip())
            if stderr:
                print("\n--- stderr ---")
                print(stderr.rstrip())
            print("="*80 + "\n")
            done += 1


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    file_path = Path(sys.argv[1])
    if not file_path.is_file():
        print(f"{file_path} is not a file")
        sys.exit(1)

    # Optional: accept a second arg for max workers
    workers = int(sys.argv[2]) if len(sys.argv) > 2 else None
    main(file_path, max_workers=workers)
