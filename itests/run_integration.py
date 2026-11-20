#!/usr/bin/env python3
"""
Integration test runner for NIFAT32.

Formats a fresh image with the formatter tool, launches the
unix_nifat32 CLI on top of it and checks the output against
declarative scenario files.

Scenario file format (one item per line):
    # comment
    @<geometry-key> <value>     per-scenario geometry override
    <command>                   sent to the unix_nifat32 stdin
    expect: <text>              text must appear in the command output
    expect-not: <text>          text must NOT appear in the command output

Geometry keys: image-size, spc, bs-count, b-bs-count, fat-count, j-count, e-count

A command output is everything printed by the tool up to and
including the next prompt ("<path> > ").
"""

import argparse
import os
import select
import shutil
import subprocess
import sys
import time
from pathlib import Path

GEOMETRY_KEYS = {
    "image-size": "image_size",
    "spc": "spc",
    "bs-count": "bs_count",
    "b-bs-count": "b_bs_count",
    "fat-count": "fat_count",
    "j-count": "j_count",
    "e-count": "e_count",
}

DEFAULT_GEOMETRY = {
    "image_size": 64,
    "spc": 8,
    "bs_count": 5,
    "b_bs_count": 0,
    "fat_count": 5,
    "j_count": 0,
    "e_count": 0,
}

TOOL_TIMEOUT = 120


def parse_scenario(path: Path) -> tuple[dict, list[tuple[str, str]]]:
    geometry = dict(DEFAULT_GEOMETRY)
    steps: list[tuple[str, str]] = []

    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue

            if line.startswith("@"):
                key, _, value = line[1:].partition(" ")
                if key not in GEOMETRY_KEYS:
                    raise ValueError(f"Unknown geometry key: {key}")
                geometry[GEOMETRY_KEYS[key]] = int(value)
                continue

            if line.startswith("expect-not:"):
                steps.append(("expect-not", line[len("expect-not:"):].strip()))
            elif line.startswith("expect:"):
                steps.append(("expect", line[len("expect:"):].strip()))
            else:
                steps.append(("cmd", line))

    return geometry, steps


class ToolSession:
    # Works on the raw stdout fd (non-blocking os.read) on purpose:
    # select() sees the pipe, while a BufferedReader would swallow the
    # prompt bytes into its internal buffer where select can't see them.
    def __init__(self, cmd: list[str], timeout: int = TOOL_TIMEOUT):
        self.proc = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
        self.fd = self.proc.stdout.fileno()
        os.set_blocking(self.fd, False)
        self.timeout = timeout
        self.window = b""

    def _read_until_prompt(self, step: str) -> bool:
        self.window = b""
        deadline = time.time() + self.timeout
        while not self.window.endswith(b" > "):
            remaining = deadline - time.time()
            if remaining <= 0:
                self.close()
                raise TimeoutError(f"unix_nifat32 hung on step: {step!r}")
            ready, _, _ = select.select([self.fd], [], [], remaining)
            if not ready:
                continue
            try:
                chunk = os.read(self.fd, 4096)
            except BlockingIOError:
                chunk = b""
            if not chunk:  # EOF: the tool exited
                break
            self.window += chunk

        return self.window.endswith(b" > ")

    def read_initial_prompt(self) -> None:
        if not self._read_until_prompt("startup"):
            raise RuntimeError(f"unix_nifat32 exited without prompt, output: {self.window!r}")

    def send(self, command: str) -> str:
        self.proc.stdin.write((command + "\n").encode())
        self.proc.stdin.flush()
        if not self._read_until_prompt(command):
            raise RuntimeError(f"unix_nifat32 exited on command {command!r}, output: {self.window!r}")
        return self.window.decode(errors="replace")

    def close(self) -> int:
        try:
            self.proc.stdin.close()
        except Exception:
            pass
        try:
            return self.proc.wait(timeout=self.timeout)
        except subprocess.TimeoutExpired:
            self.proc.kill()
            return -1


def build_image(root: Path, image: Path, geometry: dict) -> None:
    formatter_bin = root / "formatter" / "formatter"
    cmd = [
        str(formatter_bin),
        "-o", image.name,
        "-s", "nifat32",
        "--volume-size", str(geometry["image_size"]),
        "--spc", str(geometry["spc"]),
        "--fc", str(geometry["fat_count"]),
        "--bsbc", str(geometry["bs_count"]),
        "--b-bsbc", str(geometry["b_bs_count"]),
        "--jc", str(geometry["j_count"]),
        "--ec", str(geometry["e_count"]),
    ]
    subprocess.run(cmd, cwd=str(root / "formatter"), check=True, capture_output=True)
    shutil.move(root / "formatter" / image.name, image)


def run_scenario(root: Path, scenario: Path, results_dir: Path, keep: bool) -> tuple[bool, str]:
    geometry, steps = parse_scenario(scenario)
    workdir = root / "itests" / "work" / scenario.stem
    image = workdir / "nifat32.img"

    log_path = results_dir / f"{scenario.stem}.log"
    log: list[str] = []

    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True)
    results_dir.mkdir(parents=True, exist_ok=True)

    log.append(f"scenario: {scenario.name}")
    log.append(f"geometry: {geometry}")
    log.append("")

    try:
        build_image(root, image, geometry)
        log.append(f"image built: {image}")
    except Exception as e:
        log.append(f"IMAGE BUILD FAILED: {e}")
        log_path.write_text("\n".join(log), encoding="utf-8")
        return False, str(e)

    tool_cmd = [
        str(root / "builds" / "unix_nifat32"),
        str(image),
        str(geometry["image_size"]),
        "512",
        str(geometry["bs_count"]),
        str(geometry["j_count"]),
    ]

    # The tool stdout is block-buffered on a pipe and prompts have no
    # newlines, so run it unbuffered to observe the prompt after each command
    if shutil.which("stdbuf"):
        tool_cmd = ["stdbuf", "-o0", "-e2", *tool_cmd]

    failures: list[str] = []
    session = ToolSession(tool_cmd)
    try:
        session.read_initial_prompt()
        log.append("[prompt] ready")

        last_window = ""
        last_cmd = ""
        for kind, payload in steps:
            if kind == "cmd":
                last_cmd = payload
                last_window = session.send(payload)
                log.append(f"> {payload}")
                log.append(last_window.rstrip())
            elif kind == "expect":
                if payload not in last_window:
                    failures.append(f"expect: {payload!r} not found after command {last_cmd!r}")
            elif kind == "expect-not":
                if payload in last_window:
                    failures.append(f"expect-not: {payload!r} was found after command {last_cmd!r}")

        rc = session.close()
        log.append(f"[tool exit code] {rc}")
    except Exception as e:
        failures.append(f"runner error: {e}")
        log.append(f"RUNNER ERROR: {e}")
        session.close()
    finally:
        if not keep and workdir.exists():
            shutil.rmtree(workdir)

    log_path.write_text("\n".join(log), encoding="utf-8")

    if failures:
        log_text = "\n".join(f"  - {f}" for f in failures)
        return False, f"{len(failures)} check(s) failed:\n{log_text}"

    return True, f"{len(steps)} steps, log: {log_path}"


def main() -> int:
    parser = argparse.ArgumentParser(description="NIFAT32 integration tests")
    parser.add_argument("--root", type=str, default=None, help="Project root folder")
    parser.add_argument("--scenario", type=str, default=None, help="Scenario name or path (default: all)")
    parser.add_argument("--keep", action="store_true", help="Keep work directories with images")
    args = parser.parse_args()

    root = Path(args.root).resolve() if args.root else Path(__file__).resolve().parent.parent
    scenarios_dir = Path(__file__).resolve().parent / "scenarios"
    results_dir = Path(__file__).resolve().parent / "results"

    if args.scenario:
        scenario_path = Path(args.scenario)
        if not scenario_path.is_absolute():
            scenario_path = scenarios_dir / scenario_path
        if not scenario_path.exists() and not scenario_path.suffix:
            scenario_path = scenario_path.with_suffix(".scn")
        scenarios = [scenario_path]
    else:
        scenarios = sorted(scenarios_dir.glob("*.scn"))

    if not scenarios:
        print(f"No scenarios found in {scenarios_dir}", file=sys.stderr)
        return 2

    if not (root / "builds" / "unix_nifat32").exists():
        print("unix_nifat32 not found. Build the tools first: make tools", file=sys.stderr)
        return 2

    failed = 0
    for scenario in scenarios:
        ok, message = run_scenario(root, scenario, results_dir, keep=args.keep)
        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {scenario.name}: {message}")
        if not ok:
            failed += 1

    print(f"\nintegration: {len(scenarios) - failed}/{len(scenarios)} scenarios passed")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
