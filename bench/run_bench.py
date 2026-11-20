#!/usr/bin/env python3
"""
Benchmark runner for NIFAT32.

Drives the shared C test runner (test/nifat32_tests.py, imported as a
module) against declarative .bench scenario files. Each scenario builds
a fresh image in its own work directory (bench/work/<name>/), compiles
the selected C test(s) and runs them, saving raw records to
bench/results/<timestamp>/<name>/.

Scenario file format (one item per line):
    # comment
    @<key> <value>                    scenario setting
    @sweep <param> <from> <to> <step> optional parameter sweep

Setting keys:
    image-size, spc, bs-count, b-bs-count, fat-count, j-count, e-count
        - image geometry (passed to the formatter and the test build)
    test       - test name substring (selects test_*.c in test/)
    test-size  - size/count argument passed to the test binary
    runs       - how many times to repeat the whole scenario
    compiler   - C compiler (default: gcc)

Sweep parameters are the same as the shared runner's --collect-param
(fat-count, bs-count, ...). A sweep replaces the plain runs.
"""

import argparse
import os
import shutil
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT_DEFAULT = HERE.parent

# Reuse the shared C-test runner as a module (no import side effects)
sys.path.insert(0, str(ROOT_DEFAULT / "test"))
import nifat32_tests as runner  # noqa: E402

SCENARIO_KEYS: dict[str, tuple[str, type]] = {
    "image-size": ("image_size", int),
    "spc": ("spc", int),
    "bs-count": ("bs_count", int),
    "b-bs-count": ("b_bs_count", int),
    "fat-count": ("fat_count", int),
    "j-count": ("j_count", int),
    "e-count": ("e_count", int),
    "test": ("test_specific", str),
    "test-size": ("test_size", int),
    "runs": ("runs", int),
    "compiler": ("compiler", str),
}

DEFAULTS: dict[str, object] = {
    "image_size": 64,
    "spc": 8,
    "bs_count": 5,
    "b_bs_count": 0,
    "fat_count": 5,
    "j_count": 0,
    "e_count": 0,
    "test_specific": None,
    "test_size": 1000,
    "runs": 1,
    "compiler": "gcc",
}


def parse_scenario(path: Path) -> tuple[dict, tuple[str, int, int, int] | None]:
    settings: dict[str, object] = dict(DEFAULTS)
    sweep: tuple[str, int, int, int] | None = None

    with open(path, "r", encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            key = parts[0][1:]

            if key == "sweep":
                if len(parts) != 5 or parts[1] not in runner.COLLECTABLE_ARGS:
                    raise ValueError(f"Bad sweep line (need '@sweep <param> <from> <to> <step>'): {line}")
                sweep = (parts[1], int(parts[2]), int(parts[3]), int(parts[4]))
                continue

            if key not in SCENARIO_KEYS:
                raise ValueError(f"Unknown scenario key: {key!r} in {path.name}")
            attr, conv = SCENARIO_KEYS[key]
            settings[attr] = conv(parts[1])

    if settings["test_specific"] is None:
        raise ValueError(f"Scenario {path.name} must select a test via '@test <name>'")

    return settings, sweep


def build_namespace(settings: dict, root: Path) -> argparse.Namespace:
    return argparse.Namespace(
        test_specific=settings["test_specific"],
        test_type="default",
        new_image=True,
        clean=True,
        test_size=settings["test_size"],
        compiler=settings["compiler"],
        formatter=str(root / "formatter"),
        tests_folder=str(root / "test"),
        root_folder=str(root),
        image_size=settings["image_size"],
        spc=settings["spc"],
        bs_count=settings["bs_count"],
        b_bs_count=settings["b_bs_count"],
        j_count=settings["j_count"],
        e_count=settings["e_count"],
        fat_count=settings["fat_count"],
        debug=None,
        collect_param=None,
        collect_from=1,
        collect_to=1,
        collect_step=1,
        collect_output=None,
        collect_append=False,
        # bitflip fields (unused here, but the shared runner expects them)
        bitflips_strategy="random",
        scratch_width=1,
        scratch_intensity=0.7,
        bitflips_count=10,
        bitflip_runs=1,
        bitflip_stats_output=None,
        injector_scenario=None,
    )


def _append_records(path: Path, records: list[dict]) -> None:
    with open(path, "a", encoding="utf-8") as f:
        for record in records:
            line = (
                f"test={record['test']} "
                f"rc={record['returncode']} "
                f"elapsed_sec={record['elapsed_sec']:.6f} "
                f"stdout={runner._sanitize_output(record['stdout'])} "
                f"stderr={runner._sanitize_output(record['stderr'])}"
            )
            f.write(line + "\n")


def run_scenario(root: Path, scenario: Path, results_dir: Path) -> bool:
    settings, sweep = parse_scenario(scenario)
    ns = build_namespace(settings, root)

    workdir = HERE / "work" / scenario.stem
    outdir = results_dir / scenario.stem
    if workdir.exists():
        shutil.rmtree(workdir)
    workdir.mkdir(parents=True)
    outdir.mkdir(parents=True)

    summary = [f"scenario: {scenario.name}", f"settings: {settings}"]
    ok = True

    original_cwd = os.getcwd()
    os.chdir(workdir)
    try:
        if sweep is not None:
            param, from_v, to_v, step_v = sweep
            ns.collect_param = param
            ns.collect_from = from_v
            ns.collect_to = to_v
            ns.collect_step = step_v
            ns.collect_output = str(outdir / f"{scenario.stem}_sweep.txt")
            summary.append(f"sweep: {param} {from_v}..{to_v} step={step_v}")

            runner.collect_data(ns)
            ok = (outdir / f"{scenario.stem}_sweep.txt").exists()
        else:
            runs = int(settings["runs"])
            for run_index in range(1, runs + 1):
                print(f"=== {scenario.stem}: run {run_index}/{runs} ===")
                run_file = outdir / f"run_{run_index:02d}.txt"
                run_file.write_text(
                    f"# {scenario.name} run={run_index} size={ns.test_size}\n",
                    encoding="utf-8",
                )

                test_bins = []
                try:
                    runner._build_image_from_args(ns)
                    test_files, test_bins = runner._compile_selected_tests(ns)
                    records = runner.run_tests_collect(test_bins, ns.test_size)
                    _append_records(run_file, records)
                    run_ok = all(record["success"] for record in records)
                    summary.append(
                        f"run_{run_index:02d}: {'ok' if run_ok else 'FAILED'} "
                        f"({len(records)} test record(s))"
                    )
                    ok = ok and run_ok
                finally:
                    runner._cleanup_test_bins(test_bins)
    except Exception as e:
        summary.append(f"RUNNER ERROR: {e}")
        ok = False
    finally:
        os.chdir(original_cwd)

    (outdir / "summary.txt").write_text("\n".join(summary) + "\n", encoding="utf-8")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description="NIFAT32 benchmark runner")
    parser.add_argument("--root", type=str, default=None, help="Project root folder")
    parser.add_argument("--scenario", type=str, default=None, help="Scenario name or path (default: all)")
    args = parser.parse_args()

    root = Path(args.root).resolve() if args.root else ROOT_DEFAULT
    scenarios_dir = HERE / "scenarios"

    if args.scenario:
        scenario_path = Path(args.scenario)
        if not scenario_path.is_absolute():
            scenario_path = scenarios_dir / scenario_path
        if not scenario_path.exists() and not scenario_path.suffix:
            scenario_path = scenario_path.with_suffix(".bench")
        scenarios = [scenario_path]
    else:
        scenarios = sorted(scenarios_dir.glob("*.bench"))

    if not scenarios:
        print(f"No scenarios found in {scenarios_dir}", file=sys.stderr)
        return 2

    if not (root / "builds" / "unix_nifat32").exists():
        print("Tools not found. Build them first: make tools", file=sys.stderr)
        return 2

    results_dir = HERE / "results" / time.strftime("%Y%m%d_%H%M%S")
    results_dir.mkdir(parents=True)

    failed = 0
    for scenario in scenarios:
        ok = run_scenario(root, scenario, results_dir)
        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {scenario.name}")
        if not ok:
            failed += 1

    print(f"\nbench: {len(scenarios) - failed}/{len(scenarios)} scenarios passed")
    print(f"results: {results_dir}")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
