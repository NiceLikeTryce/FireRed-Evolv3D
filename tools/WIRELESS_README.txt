FireRed Evolv3D wireless setup

Windows: extract the ZIP, open FireRed-Evolv3D-Wireless.exe, and choose
the .3dsx produced by the official patcher. Keep your computer and 3DS
on the same private home Wi-Fi. In FBI select Remote Install > Scan QR
Code, then scan the QR displayed on your computer. Wait for FBI to finish.
The resulting SD path is /3ds/FireRed-Evolv3D/FireRed-Evolv3D.3dsx.
Install the separate HOME launcher from the patcher installation page.

Mac/Linux (Python 3.10 or later):
  python3 -m venv .venv
  .venv/bin/python -m pip install -r wireless-requirements.txt
  .venv/bin/python wireless_install.py
The included Python sources run the same local browser interface.

The companion verifies the generated game against the current public
release before sharing it. Internet is needed to read release information.
Game transfer stays on your LAN. Your original ROM is never requested by
the companion. Its browser picker stages the chosen game on this computer;
nothing is sent to a third-party server. Stop sharing or close the companion
when finished. Sharing expires after 30 minutes. Keep the QR private.

If Windows asks about network access, allow this companion on your private
home network. Guest-network/client isolation can block a transfer; use the
same non-isolated network or copy the generated file to the SD card instead.

For later updates, use Options > General > Tools > Check for updates in the
game. Existing saves remain under /3ds/FireRedEvolved/. The launcher keeps
previous games and save snapshots under /3ds/FireRed-Evolv3D/backups/.
Recovery restores a game version while keeping your current save progress.

Project: https://github.com/NiceLikeTryce/FireRed-Evolv3D
Patcher: https://niceliketryce.github.io/FireRed-Evolv3D/

Source: wireless_install.py and wireless_web.py, MIT; see LICENSE.txt.
Runtime and dependency notices are included in licenses/. The ZIP contains
no FireRed ROM, generated game, save, or game assets.
