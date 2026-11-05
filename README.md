# FireRed Evolv3D

FireRed Evolv3D brings FireRed Evolved to native Nintendo 3DS homebrew.

## Major features

- Catch and collect all 251 Gen 1 and Gen 2 Pokémon on one save.
- Earn increased rewards from Gyms and major battles.
- Challenge the Battle Frontier and bring your Pokémon along as walking partners.
- Explore Kanto in 3D with voxel scenery and use touch controls on the bottom screen.

## Minor features

- Explore a Kanto that changes with real-time day and night and weather.
- Use the Pokédex Habitat Guide to find Pokémon and see when they become available.
- Train with an IV and EV viewer, longer Repels, and faster egg hatching.

`v0.1.0` is the first public platform source version. Future releases use
`vMAJOR.MINOR.PATCH`. The gameplay features above describe the integrated game.
This repository contains the platform and adapter source. The FireRed game
integration and player build are developed separately. The diagnostic target
below exercises the adapter without loading a game or writing saves.

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
