# SALSA Surveyor UI prototype

This folder contains an isolated, dependency-free UI prototype for a more approachable survey-network adjustment workflow.

## Run it
Open `index.html` in a modern desktop browser. No build step or network connection is required.

## Current scope
- Native Qt dockable workspace in the SALSA GUI, backed by the existing `GuiModel` and wired to the established adjustment command.
- Survey-workspace layout with project navigation, tool ribbon, data tables, network canvas, and adjustment summary.
- Sample points and observations for visual and interaction testing.
- Search/filter, workspace navigation, point selection, canvas zoom controls, and a clearly labeled demonstration adjustment state.

## Important
This is a front-end prototype using illustrative sample data. It does **not** call the SALSA solver or calculate a real least-squares adjustment. The existing SALSA mathematical engine is untouched. The next integration step is to build typed survey-point and observation views from SALSA's record model and actual adjustment outputs, with regression tests around solver inputs and outputs.

Trimble JXL import is intentionally out of scope for this first UI milestone.
