#!/usr/bin/env python3
"""
Robustness runner for NIFAT32 (SEU / MEU campaigns).

Two levels:
    smoke   (default) - fast setup -> inject -> test loops driven by
                        robust/bitflip_tester.py (SEU = random single-bit
                        flips, MEU = scratch emulation)
    full    (--full)  - smoke + scenario campaigns driven by
                        test/nifat32_tests.py --test-type bitflip with the
                        bitflips_1 + bitflips_2 tests (creation -> inject ->
                        verify, with periodic FS repair)

Smoke scenario files (robust/scenarios/*.scn):
    # comment
    @<key> <value>
    @sweep <param> <from> <to> <step>   optional bitflips/image-size sweep

Keys: strategy (random|scratch), bitflips, scratch-width, scratch-intensity,
      runs, setup-size, test-size, image-size, spc, bs-count, b-bs-count,
      fat-count, j-count, e-count, compiler

Campaign files (robust/campaigns/*.scn) use the shared runner grammar:
    T <size>   run the selected tests (bitflips_1 + bitflips_2)
    I <n>      inject n random single-bit flips (SEU)
    S <len>    inject a scratch of len bytes (MEU)
    C          restore the clean snapshot, back to creation mode
Plus optional settings: @campaign-runs, @scratch-width, @scratch-intensity.
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT_DEFAULT = HERE.parent
PY = sys.executable

SMOKE_KEYS: dict[str, tuple[str, type]] = {
    "strategy": ("bitflips_strategy", str),
    "bitflips": ("bitflips_count", int),
    "scratch-width": ("scratch_width", int),
    "scratch-intensity": ("scratch_intensity", float),
    "runs": ("runs", int),
    "setup-size": ("setup_size", int),
    "test-size": ("test_size", int),
    "image-size": ("image_size", int),
    "spc": ("spc", int),
    "bs-count": ("bs_count", int),
    "b-bs-count": ("b_bs_count", int),
    "fat-count": ("fat_count", int),
    "j-count": ("j_count", int),
    "e-count": ("e_count", int),
    "compiler": ("compiler", str),
}

SMOKE_DEFAULTS: dict[str, object] = {
    "bitflips_strategy": "random",
    "bitflips_count": 10,
    "scratch_width": 1,
    "scratch_intensity": 0.7,
    "runs": 5,
    "setup_size": 1000,
    "test_size": 1000,
    "image_size": 64,
    "spc": 8,
    "bs_count": 5,
    "b_bs_count": 0,
    "fat_count": 5,
    "j_count": 0,
    "e_count": 0,
    "compiler": "gcc",
}

CAMPAIGN_DEFAULTS: dict[str, object] = {
    "campaign_runs": 3,
    "scratch_width": 1,
    "scratch_intensity": 0.7,
    "image_size": 64,
    "spc": 8,
    "bs_count": 5,
    "fat_count": 5,
    "compiler": "gcc",
}

CAMPAIGN_KEYS: dict[str, tuple[str, type]] = {
    "campaign-runs": ("campaign_runs", int),
    "scratch-width": ("scratch_width", int),
    "scratch-intensity": ("scratch_intensity", float),
    "image-size": ("image_size", int),
    "spc": ("spc", int),
    "bs-count": ("bs_count", int),
    "fat-count": ("fat_count", int),
    "compiler": ("compiler", str),
}


def _parse_setting_lines(path: Path, keys: dict[str, tuple[str, type]], defaults: dict) -> dict:
    settings: dict[str, object] = dict(defaults)
    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("@"):
                parts = line[1:].split()
                if parts[0] not in keys:
                    raise ValueError(f"Unknown scenario key: {parts[0]!r} in {path.name}")
                attr, conv = keys[parts[0]]
                settings[attr] = conv(parts[1])
    return settings


def _run_logged(cmd: list[str], cwd: Path, log_path: Path, extra_path: str | None = None) -> int:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ)
    if extra_path:
        env["PYTHONPATH"] = extra_path + os.pathsep + env.get("PYTHONPATH", "")
    with open(log_path, "w", encoding="utf-8") as log:
        log.write(f"COMMAND: {' '.join(cmd)}\n\n")
        log.flush()
        return subprocess.call(cmd, cwd=str(cwd), env=env, stdout=log, stderr=subprocess.STDOUT)


def run_smoke_scenario(root: Path, scenario: Path, results_dir: Path) -> tuple[bool, str]:
    settings = _parse_setting_lines(scenario, SMOKE_KEYS, SMOKE_DEFAULTS)

    workdir = HERE / "work" / scenario.stem
    outdir = results_dir / scenario.stem
    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True)
    outdir.mkdir(parents=True)

    image = workdir / "nifat32.img"
    stats = outdir / "stats.txt"

    cmd = [
        PY, str(HERE / "bitflip_tester.py"),
        "--tests-folder", str(root / "test"),
        "--root-folder", str(root),
        "--formatter", str(root / "formatter"),
        "--image-path", str(image),
        "--new-image",
        "--compiler", str(settings["compiler"]),
        "--runs", str(settings["runs"]),
        "--setup-size", str(settings["setup_size"]),
        "--test-size", str(settings["test_size"]),
        "--bitflips-strategy", str(settings["bitflips_strategy"]),
        "--bitflips-count", str(settings["bitflips_count"]),
        "--scratch-width", str(settings["scratch_width"]),
        "--scratch-intensity", str(settings["scratch_intensity"]),
        "--image-size", str(settings["image_size"]),
        "--spc", str(settings["spc"]),
        "--bs-count", str(settings["bs_count"]),
        "--b-bs-count", str(settings["b_bs_count"]),
        "--fat-count", str(settings["fat_count"]),
        "--j-count", str(settings["j_count"]),
        "--e-count", str(settings["e_count"]),
        "--stats-output", str(stats),
        "--cleanup",
    ]

    # rc 0 = no failed runs, rc 1 = campaign finished with failed runs
    # (a measurement result, not an infrastructure error)
    rc = _run_logged(cmd, workdir, outdir / "run.log")
    if rc not in (0, 1):
        return False, f"bitflip_tester crashed with rc={rc}, see {outdir / 'run.log'}"

    summary = stats.read_text(encoding="utf-8").strip().splitlines()
    summary_line = summary[-1] if summary else "no stats"
    return True, summary_line


def run_campaign(root: Path, scenario: Path, results_dir: Path) -> tuple[bool, str]:
    settings = _parse_setting_lines(scenario, CAMPAIGN_KEYS, CAMPAIGN_DEFAULTS)

    workdir = HERE / "work" / scenario.stem
    outdir = results_dir / scenario.stem
    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True)
    outdir.mkdir(parents=True)

    stats = outdir / "stats.txt"

    # The shared bitflip campaign works on the CWD-relative "nifat32.img",
    # so run it inside the scenario workdir
    cmd = [
        PY, str(root / "test" / "nifat32_tests.py"),
        "--test-type", "bitflip",
        "--test-specific", "test_nifat32_bitflips",
        "--formatter", str(root / "formatter"),
        "--tests-folder", str(root / "test"),
        "--root-folder", str(root),
        "--new-image",
        "--clean",
        "--compiler", str(settings["compiler"]),
        "--image-size", str(settings["image_size"]),
        "--spc", str(settings["spc"]),
        "--bs-count", str(settings["bs_count"]),
        "--fat-count", str(settings["fat_count"]),
        "--bitflip-runs", str(settings["campaign_runs"]),
        "--scratch-width", str(settings["scratch_width"]),
        "--scratch-intensity", str(settings["scratch_intensity"]),
        "--injector-scenario", str(scenario),
        "--bitflip-stats-output", str(stats),
    ]

    # rc 0 = all runs passed, rc 1 = campaign finished with failed runs.
    # robust/ must be on PYTHONPATH: nifat32_tests.py imports injector from there
    rc = _run_logged(cmd, workdir, outdir / "run.log", extra_path=str(HERE))
    if rc > 1:
        return False, f"bitflip campaign crashed with rc={rc}, see {outdir / 'run.log'}"
    if not stats.exists():
        return False, "campaign exited with rc=1 without stats (early failure), see " + str(outdir / "run.log")

    summary = stats.read_text(encoding="utf-8").strip().splitlines()
    summary_line = summary[-1] if summary else "no stats"
    return True, summary_line


def main() -> int:
    parser = argparse.ArgumentParser(description="NIFAT32 robustness tests (SEU/MEU)")
    parser.add_argument("--root", type=str, default=None, help="Project root folder")
    parser.add_argument("--full", action="store_true", help="Also run the bitflip scenario campaigns")
    parser.add_argument("--scenario", type=str, default=None, help="Scenario name (default: all)")
    args = parser.parse_args()

    root = Path(args.root).resolve() if args.root else ROOT_DEFAULT

    if not (root / "builds" / "unix_nifat32").exists():
        print("Tools not found. Build them first: make tools", file=sys.stderr)
        return 2

    results_dir = HERE / "results" / time.strftime("%Y%m%d_%H%M%S")
    results_dir.mkdir(parents=True)

    failed = 0
    total = 0

    def resolve(name: str, folder: Path) -> Path:
        path = Path(name)
        return path if path.is_absolute() else folder / f"{name}.scn"

    scenarios = sorted((HERE / "scenarios").glob("*.scn"))
    if args.scenario:
        scenarios = [resolve(args.scenario, HERE / "scenarios")]

    for scenario in scenarios:
        if not scenario.exists():
            # Maybe it is a campaign name: let the campaigns section handle it
            print(f"[SKIP] smoke {scenario.name}: not found in scenarios/")
            continue
        ok, message = run_smoke_scenario(root, scenario, results_dir)
        status = "PASS" if ok else "FAIL"
        print(f"[{status}] smoke {scenario.name}: {message}")
        failed += 0 if ok else 1
        total += 1

    if args.full:
        campaigns = sorted((HERE / "campaigns").glob("*.scn"))
        if args.scenario:
            campaigns = [resolve(args.scenario, HERE / "campaigns")]
        for campaign in campaigns:
            if not campaign.exists():
                print(f"[SKIP] campaign {campaign.name}: not found")
                continue
            ok, message = run_campaign(root, campaign, results_dir)
            status = "PASS" if ok else "FAIL"
            print(f"[{status}] campaign {campaign.name}: {message}")
            failed += 0 if ok else 1
            total += 1

    print(f"\nrobust: {total - failed}/{total} scenarios passed")
    print(f"results: {results_dir}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
