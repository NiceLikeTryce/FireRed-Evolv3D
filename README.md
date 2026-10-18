# Pokémon FireRed Evolved — 3DS port

**Work in progress. There is no playable FireRed3DS build yet.**

This 3DS development repository is private. Development builds and integration
stay private; the existing GBA patcher is a separate project.

This fork starts a native Nintendo 3DS port of FireRed Evolved using the
Emerald 3DS platform backend. The previous native `.gba` voxel experiment is
retired. This project targets3DS homebrew, with dual-screen and voxel rendering
planned through the existing backend.

Implemented so far:

- Native 3DS build tools and a diagnostic 3DSX target.
- Input and video-memory bridge using the existing3DS decoder interfaces.
- BIOS memory copies and immediate/deferred/repeating DMA transfers.
- FireRed 640 and imported-Emerald 512 tileset-bank handling.
- Bounds-checked32-bit FireRed metatile attributes and map-block decoding.
- A plain-C provider interface for linking real FireRed native engine units.

The FireRed integration has compiled and exercised the game's actual RNG/task
core and checked its map, sprite and save layouts. Full game startup, assets,
scripts, audio, save storage, dual-screen menus and Kanto voxel scenes remain.
The `port-check` program clearly identifies itself as a diagnostic; it does not
load a game or write saves. Original game content is not included here.

## Development

```sh
make -C firered_port check       # synthetic host tests; no game data
make -C firered_port port-check  # diagnostic 3DSX; requires devkitPro
```

See [the adapter guide](firered_port/README.md) and
[current development notes](docs/CHANGELOG.txt). The SDK image and upstream
revision are pinned in [port.lock.json](firered_port/port.lock.json).
CI tests the platform adapter and builds only the diagnostic target.

## Upstream reference

The existing `3ds_port/`, `builder/` and Emerald documentation are retained from
[Emerald 3DS](https://github.com/ZallaxDev/emerald-3ds). Its Emerald installer is
not an installer for this FireRed port. Preserve the scoped
[upstream licence](LICENSE-PORT.md), [notices](NOTICE.md) and the
[new adapter licence](firered_port/LICENSE).
