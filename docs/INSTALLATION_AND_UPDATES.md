# Installation and updates

Build from a supported clean FireRed ROM in the browser, then follow the
[installation guide](https://niceliketryce.github.io/FireRed-Evolved/install.html).
The HOME launcher uses title `000400000F3D0100` and starts
`/3ds/FireRed-Evolv3D/FireRed-Evolv3D.3dsx`. Saved progress remains in
`/3ds/FireRedEvolved/`. Installing a generic launcher does not install a game.

The guide compares a selected `.3dsx` with the build hashes in the current
patcher manifest. The browser reads the file locally. Before replacing the
game manually, close it and copy the entire `/3ds/FireRedEvolved/` folder
to a separate location. Leave that folder on the SD card and replace only
the executable in `/3ds/FireRed-Evolv3D/`.

The Windows wireless companion and included Python source verify the selected
generated game against release metadata, then share that single file over a
private LAN with an expiring random URL. Its management page runs on loopback;
the generated QR is local. No game is uploaded to an external service.

In-game **Options → General → Tools** provides update and recovery actions.
After a successful normal game save, the game closes its runtime and opens the
launcher. The launcher verifies HTTPS, installed-game SHA256, patch SHA256 and
BPS CRCs, then builds the new file with bounded streaming buffers. A synced
intent journal precedes the first rename. Original saves are copied into an
immutable snapshot; a previous game is retained by its hash. Recovery checks
actual file hashes and preserves current saved progress. A failed or interrupted
update never authorizes an unverified game file. Hold SELECT while opening the
HOME icon for the maintenance menu. First launcher adoption requires a fresh
launcher-compatible build; older supported outputs can use its update check.

Public releases include only approved patch data and generic installation tools.
`service.json` describes exact source/output/patch hashes; the Pages assembler
requires its version/output and patch inventory to agree with `release.json`.
Installation downloads have fixed basenames, size caps and SHA256 descriptors.
The launcher contains only its public CA bundle and licence notices in RomFS.

Forwarding is adapted from **Emerald 3DS by Zallax and contributors** at
`bcae4b70dac61419fc933084084469f6cfbe344b`:
https://github.com/ZallaxDev/emerald-3ds . Paths, identity, artwork and service
integration were changed. This adaptation retains CC BY-NC-SA4.0; see
`3ds_port/forwarder/LICENSE-PORT.md` and `UPSTREAM.json`. Independent MIT service
modules, original artwork, the separate MIT wireless companion and third-party
libraries retain their stated licences. The generic native restore integration
is included under `3ds_port/forwarder/integration/`.

Local qualification uses the actual ARM/native UI, a real full-game update and
rollback, host fault-interruption cases and exact save hashes. It does not claim
physical HOME Menu installation or physical SD power-cut validation.
