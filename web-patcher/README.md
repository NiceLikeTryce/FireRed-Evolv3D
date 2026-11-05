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

The public patcher is at https://niceliketryce.github.io/FireRed-Evolv3D/.
Local build candidates and distribution records are maintained separately from
this public source tree.

## Prepare a deployment

After an approved patch-data release is published, run
`python tools/prepare_web_patcher_site.py --release v0.1.1 --output pages-out`.
The script verifies the owner repository, release author, exact asset names,
declared sizes and SHA-256 hashes, then validates the manifest with the browser
module. It creates the completed output atomically in a fresh directory.

The Pages workflow accepts a published release tag through a manual dispatch
from the owner's default branch. Its jobs prepare and deploy static files.
Publication and Pages activation follow the project's distribution approval.
