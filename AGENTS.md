# FireRed Evolved 3DS source fork

This is the public platform fork. Preserve upstream licences and source history.
The active FireRed adapter is firered_port/. The original3ds_port/ remains the
Emerald 3DS backend reference; its unmodified builder produces Emerald, not FireRed.
Never run that bootstrap/reset script on the FireRed game checkout.

Keep FireRed game source, graphics/audio/text, ROMs, saves, generated packs,
compiler outputs and private integration files out of this repository and all
public artifacts. Author generic clean platform adapters here. Actual FireRed
Evolved game integration stays on feature/3ds-port in the private game repository.
Keep the public BPS patcher and stable game unchanged.

The current milestone is native-core integration, NOT a playable game. Never
label a renamed Emerald binary or the diagnostic port-check3DSX as FireRed
Evolved gameplay. Do not publish game binaries or assets from CI. Use pinned
upstream/SDK revisions. Keep fixed-width plain-C SDK/game boundaries; game ABI
flags must never be applied to native libctru translation units.

Current-game constraints:251 visible species;640 primary tiles/metatiles on
FireRed layouts and512 on imported Emerald layouts. FireRed Evolved uses 32-bit
metatile attributes in both cases. Do not assume Emerald map/save structs.
Keep FEATURES.txt and consolidated docs/CHANGELOG.txt current in every commit.
Commit/push authorized completed platform work as NiceLikeTryce. No upstream
pushes, no auto PRs, no private game imports into this public Git history.
