# Testing

Four test systems run from the root Makefile. Each system builds the tools it needs and keeps images and logs under its own folder, which is git-ignored.

| Target | System | Runs |
| --- | --- | --- |
| `make unit` | `utests/` | C unit tests of library tools plus single-filesystem runs |
| `make integration` | `itests/` | Declarative scenarios driven through the `unix_nifat32` shell |
| `make bench` | `bench/` | C benchmark scenarios with parameter sweeps |
| `make robust` | `robust/` | SEU/MEU robustness smoke loops |
| `make robust-full` | `robust/` | Smoke loops plus bitflip scenario campaigns |

Aggregated targets:

```bash
make test      # unit + integration
make test-all  # unit + integration + bench + robust-full
```

## Unit

```bash
make unit
```

The target builds the unit test binary, formats a fresh 16 MB image (spc 8, fc 5, bsbc 5, jc 0, ec 0) into `builds/utests/`, and runs every test registered in `utests/`. Helper tests run against an in-memory fake disk, and the `fs_*` runs mount the pristine image through the public API. A new `utests/test_*.c` file is picked up by the build automatically.

```
utests: 175 assertions, 0 failed
```

## Integration

```bash
make integration
make integration INTEG_ARGS="--scenario basic_crud"
```

A scenario is a declarative `.scn` file in `itests/scenarios/`: `basic_crud`, `copy_move`, `error_paths`, `nested_dirs`, `rename_trunc`. The runner formats an image, starts `unix_nifat32`, replays the commands, and checks the output with `expect:` and `expect-not:` lines. Per-scenario logs go to `itests/results/`, work images to `itests/work/`, which `INTEG_ARGS="--keep"` preserves.

## Benchmarks

```bash
make bench
make bench BENCH_ARGS="--scenario cold_boot"
```

A scenario is a `.bench` file in `bench/scenarios/`: `cold_boot`, `different_bs`, `fat_ops`, `many_files`, `sweep_fat`, `throughput`. The `@test`, `@runs`, and `@sweep` directives pick the C benchmark, the run count, and the parameter sweep. The runner compiles the benchmark, runs it on a fresh image, and stores the measured values under `bench/results/<timestamp>/<name>/`. `bench/collector.sh` sweeps the standalone runner over `--fat-count`, `--bs-count`, or `--b-bs-count` and writes the series to `collected_results/`.

## Robustness

```bash
make robust
make robust ROBUST_ARGS="--scenario seu_basic"
make robust-full
```

Smoke scenarios in `robust/scenarios/` (`seu_basic`, `seu_stress`, `meu_mixed`, `meu_scratch`) set up an image, inject single-event or multiple-event upsets, and verify the filesystem, reporting the failure rate per loop. `robust-full` additionally runs the bitflip scenario campaigns from `robust/campaigns/` (`seu_campaign`, `meu_campaign`) through `test/nifat32_tests.py`. Campaign steps use the `T/I/S/C` grammar: `T` is a forward test, `I` is random single-bit flip injection, `S` is a scratch upset, and `C` restores the clean snapshot.

## Standalone C test runner

The test runner can build a fresh image and execute the C test suite:

```bash
python3 test/nifat32_tests.py \
  --new-image \
  --formatter formatter \
  --tests-folder test \
  --root-folder . \
  --clean \
  --test-type default
```

Fault-injection tests:

```bash
python3 test/nifat32_tests.py \
  --new-image \
  --formatter formatter \
  --tests-folder test \
  --root-folder . \
  --clean \
  --test-type bitflip \
  --injector-scenario test/injector_scenario.txt
```

See [`test/README.md`](../../test/README.md) for campaign and collector options.
