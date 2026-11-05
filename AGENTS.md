# FireRed Evolv3D source and browser patcher

November 5, 2026 owner approval: publish the verified FireRed Evolv3D native
patch-data package and deploy its public browser patcher through a Pages-only
workflow. This explicitly permits the qualified generated patch payloads and
release manifest in public releases and Pages artifacts. Keep generated data
out of source Git. Keep the separate game repository, retail ROMs, saves,
private game source, ELF files and complete prebuilt game images private.
Preserve existing refs. Native validation remains manually disabled. Pages
deployment may be enabled for the owner's approved public patcher.

The owner approved public visibility for NiceLikeTryce/FireRed-Evolv3D
(repository ID 1424368097). This overrides the older private-visibility
instructions below for this platform repository. Keep the separate game
repository, ROMs, saves, and game-build artifacts private. Preserve upstream
licence scopes and existing refs. Start public platform versions at v0.1.0
and use vMAJOR.MINOR.PATCH for later releases.

This is the PRIVATE platform fork. Preserve upstream licences and source history.
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
pushes, no auto PRs, no game imports into this platform Git history.

# Owner privacy requirement (October18,2026)
NiceLikeTryce/firered-evolved-3ds (repository id1424368097) is PRIVATE at the
owner's explicit request. Never make it public or publish its development
artifacts to public Pages/releases. Verify exact destination and private
visibility before pushing. Preserve the source-only platform/game separation
and licences. The separate public GBA BPS patcher is unaffected. Game CI reads
this private platform through a dedicated read-only deploy key; never print or
commit its private key. Current uncommitted port work remains preserved.
