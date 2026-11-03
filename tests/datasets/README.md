# Auto-map datasets

Real target heads for measuring the auto-map landmark solver (Sprint 12). The
meshes stay out of git unless their licence allows sharing; keep a local copy
in `tests/datasets/local/` (ignored by git) or anywhere else on disk.

## A dataset case

Each case is a saved MetaVisage project (`.mmproj`):

1. Pick **MetaHuman Head (LOD0)** as the source in Stage 1.
2. Import the test head as the target and align it in Stage 1.
3. Optional reference answer: in Stage 2 choose Help > Show Profile Landmarks
   (Dev), then click the matching spot on the target for each landmark. Clicks
   pair with the landmarks in order (1–52). Symmetry can stay on.
4. Save the project. Mesh paths are stored relative to the project, so keep the
   project next to (or above) its mesh.

## Running the benchmark

```powershell
.\build\bin\Release\metavisage_automap_bench.exe "<dataset folder>" [--out "<folder>"]
```

For tuning, `--no-refit` and `--no-snap` turn off the feature refit and the
crease snap, `--regions eye,ear` picks which regions are refit, and
`--set name=value` overrides a float parameter (for example
`--set snapRadius=0.01`). Write tuning runs somewhere other than the default
results folder.

It finds every `.mmproj` under the folder and writes to `<dataset>/automap-results/`
unless `--out` is given:

- `report.md` / `report.csv`: per case, time, confidence counts and, with a
  reference answer, median / 90th-percentile / worst error as a percentage of
  the target's bounding-box diagonal, plus the worst landmarks.
- `landmarks.csv`: one row per landmark per case, with confidence and error.
- `<case>.automap.mmproj`: the project with the solved points paired, to open
  and inspect in Stage 2.

Errors include any difference between where a landmark is defined on the
source profile and where it was clicked on the target, so a landmark that is
off by the same amount in the same direction on every head usually means the
two placements disagree, not that the solver missed.
