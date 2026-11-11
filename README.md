# FireRed Evolv3D

FireRed Evolv3D brings FireRed Evolved to native Nintendo 3DS homebrew.

Current beta: **v0.2.46**, synchronized with FireRed Evolved’s shared encounter rules and event scripts.

## How to Play

1. Open the [3DS patcher](https://niceliketryce.github.io/FireRed-Evolv3D/).
2. Select your own clean US FireRed ROM (v1.0 or v1.1). The patcher detects the version.
3. Click **Build and download .3dsx**.
4. Open the downloaded file in **Azahar**, or follow the [Nintendo 3DS setup guide](https://niceliketryce.github.io/FireRed-Evolved/install.html).

## Major features

- **All 251 Pokémon** — Catch every Gen 1 and 2 species.
- **Bigger battle rewards** — Bonus items, TMs and unlocks.
- **RNG Fix** — Ruby and Sapphire’s clock-based seeding.
- **Voxel Kanto** — 3D overworld, interiors and battles.
- **Dual-screen controls** — Touch maps and menus.
- **Walking partners** — All 386 species, including shinies.
- **Battle Frontier** — Seven facilities, link battles and recordings.
- **Secret Bases** — 28 entrances, decorating, visits and battles.
- **Celadon Contests** — Five categories with ribbons and paintings.
- **Pokéblocks** — Blend berries with NPCs or friends.
- **Berry gardening** — Grow berries with the Goldeen Canteen.

## Minor features

- **HOME Menu launcher** — Install via FBI QR code.
- **Wireless setup** — Transfer the game over Wi-Fi.
- **In-game updates** — Verified patches with game and save backups.
- **Options help** — Press SELECT for setting details.
- **Update notes** — Preview changes in launcher 1.1.0.
- **Postgame Gen3 support** — Gen 3 trades, held items, breeding and Dex entries after Celio’s quest.
- **Stable menu handoffs** — Steady screens during menu switches.
- **Photo mode** — Free camera and PNG export.
- **Quest journal** — Track story, Celio, Mew, Celebi and Frontier.
- **Pokédex device** — Pokémon and habitats on the top screen.
- **Smooth field toggle** — Switch smoothly between voxel and classic views.
- **Separate speed controls** — Independent field and 1–4× battle speeds.
- **HM assist** — Use earned HMs without teaching moves.
- **Party EXP Share** — Optional full participant and half bench EXP.
- **Physical/special split** — Optional Gen 4 move categories and AI.
- **Reusable TMs** — Optional unlimited TM use.
- **Catch EXP** — Optional EXP from catching Pokémon.
- **Real-time pacing** — Reduces field slowdowns.
- **Classic Menus** — Use the original game menus.
- **Optional cheats** — Encounter, catch, item and other cheats.
- **Clock sync** — Restore real time and keep berry growth.
- **Soft reset** — Press L+R+START+SELECT.

Gameplay enhancements and cheats default **OFF**. Speeds start at **1×**; real-time pacing defaults **ON**.

- **Crisp battle HUD** — Sharp text and animated HP bars.
- **Visible wild Pokémon** — Optional encounters on the map.
- **Reward progress** — Mew and Celebi counts, missing Pokémon and habitats.
- **Save backups** — Rotating backups with trainer previews.
- **Offline patching** — Reuse the verified package offline.
- **Modern presentation** — Tabbed Options and red Poké Ball startup.
- **Encounter odds** — Selected rares 5%. Night grass and trees: Gastly 5%, Haunter 1%. Other odds split evenly.
- **A changing world** — Real-time day, night and weather.
- **Habitat Guide** — Locations, odds, levels and unlocks.
- **Training tools** — IV/EV checks, longer Repels and faster hatching.
- **Easier evolutions** — Trade-backs, extra stones and move relearning.
- **Tree Shaker** — Find Pokémon in trees.
- **Safari Zone** — Catch the three Johto starters.
- **Acro Bike** — Hop ledges both ways after the Champion.
- **Mirage Island** — New ferry stop after the Champion.
- **Camera controls** — Reset and presets, including Old 3DS Performance.
- **Optional challenges** — Seeded randomizer and Nuzlocke tracker.

`v0.1.0` is the first public platform source version. Future releases use
`vMAJOR.MINOR.PATCH`. The gameplay features above describe the integrated game.
This repository contains the platform and adapter source. The FireRed game
integration and player build are developed separately. The diagnostic target
below exercises the adapter without loading a game or writing saves.

## Move your save

Use the [save transfer tool](https://niceliketryce.github.io/FireRed-Evolved/save.html) to move between GBA and 3DS, or import an original US/English FireRed battery save. Each package includes an untouched backup. Read the [controls and save guide](https://niceliketryce.github.io/FireRed-Evolved/guide.html).

## Upstream reference

The existing `3ds_port/`, `builder/` and Emerald documentation are retained from
[Emerald 3DS](https://github.com/ZallaxDev/emerald-3ds). Its Emerald installer is
for Pokémon Emerald. The platform code is distributed under the scoped
[upstream licence](LICENSE-PORT.md), [notices](NOTICE.md) and the
[new adapter licence](firered_port/LICENSE).

## Gameplay gallery

![Pallet Town house with corrected voxel roof](web-patcher/gallery/pallet-house-roof.png)

![Native voxel battle](web-patcher/gallery/voxel-arena-hud.png)

![Native touch-screen party](web-patcher/gallery/native-party.png)
