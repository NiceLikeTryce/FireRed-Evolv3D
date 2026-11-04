# FireRed Evolv3D

Private native Nintendo3DS project combining FireRed Evolved gameplay, an adapted
Emerald3DS world renderer and Twin Embers touch-interface work. Current integration
is in qualification; this is not a fully qualified campaign release.

This repository holds the source-only platform/backend. The actual game engine,
Evolved data, native UI bindings, generated Kanto assets and tested game packages
stay in the private game repository on feature/3ds-twin-embers-voxel. The platform
branch is feature/twin-embers-voxel. Both main branches and public GBA delivery
remain unchanged. No original game assets, saves, ROMs or generated game packs
are added to this platform repository.

The private integration runs the actual FireRed engine as ARM11 code. It has
switchable3D/classic presentation, Kanto building/terrain depth, camera controls,
SD saves, stereo audio and lower-screen menu/battle controls. Full campaign and
complete native touch-screen replacement remain in progress. The standalone
port-check target below remains a diagnostic without game or save loading.

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
