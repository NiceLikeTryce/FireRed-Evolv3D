# FireRed native 3DS adapter

This is the first integration layer, not a playable FireRed build.
It reuses the Emerald 3DS input/video decoder interfaces and adds a native-game
hardware memory bridge, BIOS copy/DMA handling, and FireRed-specific map decoding.
FireRed layouts split at 640; imported Emerald layouts split at 512. Both use
FireRed Evolved's32-bit metatile attributes. Never interpret a FireRed map using
Emerald's unmodified structures or reinterpret a FireRed save as Emerald data.

`make -C firered_port check` runs synthetic host tests without game data.
`make -C firered_port port-check` builds a diagnostic 3DSX using devkitPro.
The diagnostic prints **NOT A PLAYABLE GAME BUILD**. It does not load/write saves,
pretend to run FireRed, or turn an Emerald build into a FireRed release.
A private integration may link real FireRed native core units through the plain-C
core-provider API. Full engine startup, audio, saves, dual-screen menus and Kanto
voxel models remain integration work. Special FIFO/audio DMA is rejected until
its dedicated audio bridge is implemented.

The complete existing `3ds_port/` backend remains available for the full port.
Its own game-side modules currently depend on Emerald; they must be adapted
against FireRed Evolved's actual engine and ABI before a playable build exists.


Native gameplay services are now available on feature/native-gameplay: fixed-width
runtime/input/video/audio, exact resident payload relocation and256-colour text.
The private game integration owns all game code/assets and whole-game tests.
No executable game or generated game pack is distributed from this source fork.
