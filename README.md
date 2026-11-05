# FireRed Evolv3D

FireRed Evolv3D brings FireRed Evolved to native Nintendo 3DS homebrew.

## Major features

- **All 251 Pokémon** — Catch and collect every Gen 1 and Gen 2 species on one save.
- **Bigger battle rewards** — Earn bonus items, TMs, and unlocks from Gyms and major battles.
- **RNG Fix** — Uses Ruby and Sapphire’s clock-based startup seeding.
- **Voxel Kanto** — Explore 3D routes, towns, interiors, and battle scenes.
- **Dual-screen controls** — Use the bottom screen for maps, menus, and touch controls.
- **Walking partners** — Bring any of the 251 Pokémon along, including shiny variants.
- **Battle Frontier** — Challenge seven facilities with link battles and battle recordings.
- **Secret Bases** — Find entrances, decorate your base, and visit and battle in other bases.
- **Celadon Contests** — Compete in five categories and earn ribbons and paintings.
- **Pokéblocks** — Blend berries with NPCs or link partners.
- **Berry gardening** — Plant, water, and harvest berries with the Goldeen Canteen.

## Minor features

- **A changing world** — Real-time day and night and weather change the scenery.
- **Habitat Guide** — Look up Pokémon locations, encounter odds, levels, and unlock conditions.
- **Training tools** — Check IVs and EVs, reuse longer Repels, and hatch eggs faster.
- **Easier evolutions** — Use Bill's trade-backs, extra evolution stones, and missed-move learning.
- **Tree Shaker** — Find encounters in trees across Kanto and the islands.
- **Safari Zone** — Catch all three Johto starters.
- **Acro Bike** — Hop ledges in either direction after the Champion.
- **Mirage Island** — Unlock a new ferry destination after the Champion.
- **Camera controls** — Reset the camera and switch between Performance and Quality presets.
- **Optional challenges** — Enable a seeded randomizer or track a Nuzlocke.

`v0.1.0` is the first public platform source version. Future releases use
`vMAJOR.MINOR.PATCH`. The gameplay features above describe the integrated game.
This repository contains the platform and adapter source. The FireRed game
integration and player build are developed separately. The diagnostic target
below exercises the adapter without loading a game or writing saves.

## How to play

Use the [FireRed Evolv3D web patcher](https://niceliketryce.github.io/FireRed-Evolv3D/) to build the Nintendo 3DS version.

1. Bring your own clean US/English FireRed v1.0 or v1.1 ROM.
2. Open the [3DS web patcher](https://niceliketryce.github.io/FireRed-Evolv3D/) and select your ROM. The patcher detects the revision automatically.
3. Choose **Build and download .3dsx**. Open the downloaded file in Azahar, or copy it to `/3ds/FireRedEvolved/` on your Nintendo 3DS SD card and open it from the Homebrew Launcher.

Both base revisions produce the same 3DS build. Your ROM stays on your device.

## Development

```sh
make -C firered_port check       # synthetic host tests; no game data
make -C firered_port port-check  # diagnostic 3DSX; requires devkitPro
```

See [the adapter guide](firered_port/README.md) and
[current development notes](docs/CHANGELOG.txt). The SDK image and upstream
revision are pinned in [port.lock.json](firered_port/port.lock.json).

## Upstream reference

The existing `3ds_port/`, `builder/` and Emerald documentation are retained from
[Emerald 3DS](https://github.com/ZallaxDev/emerald-3ds). Its Emerald installer is
for Pokémon Emerald. The platform code is distributed under the scoped
[upstream licence](LICENSE-PORT.md), [notices](NOTICE.md) and the
[new adapter licence](firered_port/LICENSE).
