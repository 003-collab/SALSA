# Surveyor Workspace Validation Plan

This checklist validates the native Qt workspace against SALSA's existing project files. It tests UI integration separately from the least-squares engine. Do not change solver mathematics or input-file semantics to make a UI test pass.

## Current validation status

- [ ] Configure and compile the native application with the new workspace enabled.
- [ ] Launch the application and confirm the **View → Surveyor Workspace** toggle shows and hides the dock.
- [ ] Open a project, close it, and open a second project; confirm the tree, point table, observation table, summary, and network canvas refresh each time.
- [ ] Run an adjustment using the workspace button and confirm the existing save/confirmation, preprocessing, solver, and result workflow is preserved.
- [ ] Compare results for the same project before and after the UI change (adjusted coordinates, residuals, and reported statistics).
- [ ] Confirm the existing automated test suite passes.

## Candidate fixture matrix

These are candidate fixtures, not confirmed passing GUI tests. Verify project entry points and relative INCLUDE paths before testing.

| Fixture | What to inspect |
| --- | --- |
| `examples/example00/ex0.proj` and its included LSA files | Project loading, includes, tree traversal, Cartesian points, and GPS-related records. |
| `examples/example01/` LSA files: `ctrl_sites.lsa`, `conventional.lsa`, `conventional_TP.lsa`, `conventional_tgtPole.lsa`, and `gpsMeasurements.lsa` | Mixed survey observations and point/table population. This folder does not currently contain an `example01.proj` entry point in the repository tree; use a known project that includes these records if one is available. |
| `examples/example02/redundancy.proj` | Repeated/redundant observations and a larger observation table. |
| `examples/example03/` LSA files (`loop1.lsa`, `loop2.lsa`, and `hdiffs*.lsa`) | Loop observations and different measurement combinations. |
| `publicTest/gui/integration/example01/example01.proj` | Existing GUI integration fixture; use it when running the public GUI test suite. |
| `doc/manual/UserManual/examples/Ghilani_23.4/` | A documented worked example for checking that the existing adjustment workflow remains unchanged. |

Do not move individual LSA files out of their fixture folders because relative includes may depend on their layout.

## Network canvas acceptance checks

- [ ] Cartesian `POSC` points are visible and labelled.
- [ ] `DIST` links are drawn only when both endpoint labels exist in the plotted Cartesian point set.
- [ ] A one-point or coincident-point project remains visible without division-by-zero or non-finite drawing coordinates.
- [ ] A project containing only geodetic `POSG` points displays the explanatory empty state; do not silently treat latitude/longitude as planar X/Y.
- [ ] Resizing the dock causes the network to repaint and fit the current point extent.
- [ ] The displayed network is explicitly identified as **initial** coordinates until solver-result coordinates are wired in.

## Regression boundaries

- The UI must consume the live `GuiModel` and typed SALSA records; it must not parse formatted display strings to recover numeric data.
- The UI must not duplicate or replace adjustment computations.
- Do not implement Trimble JXL import until these checks are run and build/runtime defects are resolved.
- Record the operating system, compiler, Qt version, fixture path, and exact test result when validation is completed.
