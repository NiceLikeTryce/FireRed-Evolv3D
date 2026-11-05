# FireRed Evolv3D browser patcher source

This directory contains the browser page, worker, reconstruction modules and
component notices. The worker accepts clean US/English FireRed v1.0 and v1.1,
detects the revision, verifies every input component and verifies the finished
3DSX before offering the download.

The site requires a separately generated `release.json` and the matching files
in `assets/`. Those files are excluded from this source checkout. The current
qualified delivery candidate uses schema 10. Source paths in its reconstruction
recipes are bounded, and the complete output is bound to its SHA-256 digest.

Serve the prepared site over HTTP so the browser can create its worker and load
the reconstruction modules. The BPS runtime is loaded by the classic worker
before it imports the reconstruction module. Keep the original runtime notices
and every file linked by `NOTICE.txt` with the deployed page.

The public download address is added to the main README after deployment is
qualified. The current local candidate and distribution records are maintained
separately from this public source tree.
