# Surveyor UI integration roadmap

The goal is to make SALSA more approachable for surveyors while preserving the existing least-squares engine and established project format.

## Principles

- The existing SALSA solver remains the source of truth for adjustment mathematics.
- New views read from SALSA's existing GuiModel rather than maintaining a second copy of survey data.
- UI actions that run an adjustment must call the established MainWindow::calculateAdjustment() workflow, including its save checks, preprocessing, solver execution, and result handling.
- Sample values in the standalone surveyor-ui browser prototype are illustrative and must never be presented as computed survey results.
- Trimble JXL import is deferred until the live project/model workflow is connected and regression-tested.

## Milestones

### 1. Visual prototype — started

The standalone HTML/CSS/JavaScript prototype establishes the target visual language and basic interaction patterns. It is not a solver and does not load .lsa projects.

### 2. Native Qt workspace bridge — implemented, validation pending

SurveyorWorkspace is a dockable Qt panel that attaches directly to the existing GuiModel and invokes the current adjustment workflow through a signal handled by MainWindow. This is the first native integration step; it deliberately does not duplicate or alter solver code. A successful compile and GUI runtime test are still required.

### 3. Live survey summaries and network view — initial implementation, validation pending

The native workspace has Network, Points, Observations, and Project records tabs. It walks the existing GuiModel tree and reads POSC/POSG coordinates and constraint state from typed records. The initial network canvas plots POSC X/Y coordinates and draws DIS links where both referenced points are available; it labels the view as initial coordinates, fits all plotted points, and does not pretend to project POSG geodetic coordinates. Observation rows are sourced from measurement records; DIS records expose structured value and sigma, while other measurement types currently show their original LSA record text and referenced point labels. These are input/apriori values, not adjusted results. Selection linking, explicit CRS/unit handling, POSG projection, and adjusted-position rendering remain to be implemented. Do not infer point types or units from display strings when structured record APIs are available.

### Existing sample projects for validation

Use repository fixtures before beginning JXL support:

- `examples/example00/ex0.proj` and its included `.lsa` records for project/include handling.
- `examples/example01/` (`ctrl_sites.lsa`, `conventional.lsa`, `conventional_TP.lsa`, `gpsMeasurements.lsa`) for a mixed control/conventional/GNSS project.
- `examples/example02/redundancy.proj` and its companion records for redundancy-oriented behavior.
- `examples/example03/` (`loop1.lsa`, `loop2.lsa`, `hdiffsAB.lsa`, `hdiffsBA.lsa`, `hdiffsBC.lsa`, `hdiffsCB.lsa`) for height-difference records and directional variants.
- `doc/manual/UserManual/examples/Ghilani_23.4/` for a documented textbook example split across record files.

These are candidate fixtures identified in the repository; each must be opened and checked in the built GUI before being called a passing regression case.

### 4. Adjustment diagnostics

Bind result cards and tables to actual output data: convergence/termination status, degrees of freedom, variance factor, residuals, redundancy, adjusted coordinate precision, and confidence/error ellipses where available. Preserve the distinction between unavailable values and zero values.

### 5. Import workflow

Design a JXL importer as a separate conversion/validation layer. Map supported Trimble observations and station metadata into documented SALSA records, report unsupported records explicitly, and test against representative JXL fixtures before enabling the workflow in the main application.

## Validation gates

- Build the Qt GUI on supported platforms.
- Open, edit, save, and reload an existing .lsa project.
- Confirm the new workspace observes model replacement when projects are opened or reloaded.
- Confirm its Run Adjustment button follows the same save confirmation and solver path as the existing menu command.
- Compare solver outputs before and after UI changes on established test fixtures.
- Keep JXL parsing and any data normalization separate from the least-squares engine.
