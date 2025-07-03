# Salsa 1.15.0 Release Notes

February 22, 2022,

Salsa 1.15.0 adds an important new feature and fixes several bugs in the 1.14.1 and earlier releases.

Approved for public release by the National Geospatial-Intelligence Agency, case 22-592.

## New Developments

* Reliability Metrics

  * A new feature, reliability metrics, is introduced. The metrics provide the user with a sense of the survey's
  susceptibility to blunders. The three main metrics are:  
    1. Redundancy - previously calculated by SALSA.  Redundancy is a number between 0 and 1 that is an indicator
    of how immune the solution is to an error in the subject measurement.
    1. Internal Reliability - also known as the minimum detectable bias. This is the minimum error value for
    that particular measurement that can be detected on the basis of elevated standard residuals.
    1. External Reliability - A mapping of the internal reliability to state-space that provides a zone
    (reliability rectangle) within which it is impossible to detect blunders on the basis of standard residuals
    alone.
  * These metrics are implemented in the SALSA GUI and output files in the following ways:
    1. The minimum redundancy warning/error is now replaced with a maximum external reliability magnitude
    warning/error. The thresholds for the warning/error values are user configurable.
    1. There are two new columns in the Measurement Residuals table for internal and external reliability. No
    columns were removed from the measurement residuals table.
    1. The point confidence regions table now includes the geometry of the reliability rectangle in addition to
    the ellipses shown in previous releases. To create room for the new columns, and to help the user focus on
    the most important content, the columns displaying the ellipse azimuth and 2D minor axis length were removed.
    1. The map widget now displays reliability rectangles around the relevant points. The rectangle display
    is enabled by a checkbox.
    1. The *.out file captures reliability metrics.
    1. The output KML file now captures reliability rectangles as well as confidence ellipses.
    1. Example 2 in the user manual is updated to walk the user through the new reliability metrics.
  * For more information on the Reliability metrics, refer to the new Appendix B in the user manual, which is
  dedicated to this topic.

## Bug Fixes

The following bugs are resolved in SALSA 1.15.0. Our priority scale for bugs is P1=Highest, P5=Lowest

|*Bug or Issue ID*|*Priority*|*Short Description*|
|---|---|---|
| [1396](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1396) | P2 | Config dialog sizing error. Portions of the config dialog menu sometimes appear offscreen and are unreachable. |
| [1365](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1365) | P2 | Output full preprocessor warning messages. Rather than output the full preprocessor warnings in the SALSA log, a link to a separate display window will be output. |
| [1363](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1363) | P3 | Investigate odd behavior with SALSA produced GSDI macrobooks, particular with losing column functionality using the xlsm export script. |
| [1207](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1207) | P3 | Inconsistent decimals in station inverse data. Station inverse output would display 3-5 decimal places without following a set convention. The validation spec documents the number of decimals for linear measurements should be 5.  |
| [1355](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1355) | P4 | User manual build error for Windows. The user manual build would fail due to LaTex deprecated dependencies. Workarounds were developed and documented for other Windows users. |
| [1351](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1351) | P4 | Address large size of source archive file. The source of the bloat was a hidden ".git" file. The file was removed and the size dropped to 350 mb. |
| [1344](https://sgl-git.arlut.utexas.edu/spp/lsa/-/issues/1344) | P4 | XLSM export script modifications. The two changes were to make absolute precision persist through the entire project after the user sets it and to have Windows open the xlsm macrobook after the script performs write-out. |

## Downloading and Installing

The SALSA msi installer is being rebaselined to SALSA version 1.15.0, i.e., there is no patch file (*.msp) available
for SALSA 1.15.0.

### Download

For information on acquiring the NGA bundle (with potential FOUO content) of SALSA offline, contact the NGA Office
of Surveys at <surveys@nga.mil>
(314) 676-9152.

Other requests for SALSA should be directed to the public project page: <https://www.arlut.utexas.edu/salsa>

### Install

Installation is simple:

* For new installations, run the msi file and follow the prompts.
* For existing installations
  1. Uninstall SALSA.
  1. Run the msi and follow the prompts.
