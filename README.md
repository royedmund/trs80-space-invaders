# TRS-80 Model III Space Invaders

A faithful Z80 assembly recreation of Space Invaders for the **TRS-80 Model III**, with original cassette sound effects and optional external MIDI audio. This fork supports the Model III only.

![Space Invaders gameplay](assets/gameplay.gif)

## Editions and downloads

| Edition | Audio | Source | CMD / status |
| --- | --- | --- | --- |
| Cassette (original release) | Original cassette sounds | [Upstream project](https://github.com/TechPrototyper/trs80-space-invaders) | [space_invaders.cmd](space_invaders.cmd) — existing release |
| Yamaha EMT-10 march | MIDI four-note march; cassette effects; cassette-only option | [space_invaders.asm](space_invaders.asm) | [INVEMT.CMD](INVEMT.CMD) — experimental assembled build |
| **Yamaha EMT-10 title edition** | Automatic *Echoes from the Void* title music, MIDI march and cassette effects | [space_invaders_emt_title.asm](space_invaders_emt_title.asm) | Build from current source |
| Roland SC-55 | All music and effects over MIDI; no cassette audio | [space_invaders_sc55.asm](space_invaders_sc55.asm) | Experimental source; build from source |

The original cassette CMD and earlier EMT-10 march CMD are retained as separate releases. The title edition was previously assembled, but the latest source contains additional cleanup fixes and needs a fresh build. The SC-55 edition also needs assembly and hardware testing.

## Play

Download an available CMD or assemble the chosen source. On a real Model III, transfer the file in binary mode to your DOS environment (for example, `INVEMT/CMD` under LDOS) and enter its program name.

In [trs80gp](http://48k.ca/trs80gp.html):

```sh
trs80gp -m3 space_invaders.cmd
trs80gp -m3 INVEMT.CMD
```

| Key | Action |
| --- | --- |
| Left / right arrows | Move |
| SPACE | Start / fire |
| BREAK | Return from gameplay to the splash; press again on splash to exit |
| M + SPACE | Select EMT-10 MIDI gameplay in the march or title edition |
| M (title edition) | Toggle title music and select audio mode before starting |

In the EMT-10 march edition, SPACE alone selects cassette-only audio. In the title edition, SPACE starts with the current audio mode; music is enabled on the first splash. In the SC-55 edition, SPACE starts all-MIDI gameplay.

Do not use `-turbo` for normal play: the game and live music are paced by frame delays.

| Splash | Gameplay |
| --- | --- |
| ![Title screen](assets/splash.png) | ![Game screen](assets/gameplay.png) |

## Features

- 55 invaders with pixel-wise movement, flicker-free 64×16 graphics and four destructible shields.
- Mystery ship, increasing wave difficulty and accelerating formation movement.
- Arcade-style title screen, high score and original scrolling credits.
- Optional emulator high-score persistence across sessions using [tools/play.py](tools/play.py).
- Separate EMT-10 and SC-55 sound engines using the Model III RS-232 interface.

## Yamaha EMT-10 music

**Echoes from the Void** is an original dark sci-fi title theme, alternating native strings (program 6) and choir (program 8) with up to four simultaneous notes. The 643-byte embedded MIDI event table loops in approximately 38 seconds at a nominal 30 frames/sec, without a MIDI file parser. Actual timing depends on the Model III and serial overhead.

The title theme stops when gameplay begins and resumes on return to the splash if EMT-10 mode is selected. The latest source preserves that selection at game over and retries incomplete MIDI cleanup.

Both EMT-10 editions use a four-note march — G2, F2, E♭2, C2 — triggered by the existing formation movement. As invaders are eliminated, the march accelerates. The native EMT-10 program is 64 (Electric Bass + Piano 1 split); all four notes fall below the split. Shooting, explosions, UFO chirps and death retain the original cassette sounds.

## Roland SC-55 audio

The experimental SC-55 edition routes all music and sound effects to the Sound Canvas, including the accelerating march, shooting, UFO, explosions and player death. Independent MIDI channels allow effects to overlap. No cassette output or cassette fallback is used.

See [SC55.md](SC55.md) for channel assignments, sound design and testing instructions.

## RS-232 to MIDI interface

```text
TRS-80 Model III RS-232 (9600 baud)
  -> MAX3232
  -> Arduino Nano (D10 RX / D1 TX)
  -> MIDI OUT (31250 baud)
  -> Yamaha EMT-10 or Roland SC-55 MIDI IN
```

- [Hardware setup, wiring and parts list](hardware/README.md)
- [Circuit schematic](hardware/minimal-midi-out.svg)
- [Arduino Nano fast v0.3 firmware](firmware/serial_to_midi_fast/serial_to_midi_fast.ino)
- [MIDI interface license](hardware/MIDI-INTERFACE-LICENSE.txt)

Use the Nano firmware in **RAW mode**; power-cycle the Nano if it was previously used for timed MIDI playback or has faulted. EMT-10 users should connect the cassette and MIDI module audio outputs to separate amplifiers or a suitable audio mixer. SC-55 users only need the module audio output.

The [central Model III MIDI project](https://github.com/royedmund/TRS-80-Model-III-Simple-Midi-Out) remains the primary location for firmware and interface development.

## Build from source

Install [zmac](http://48k.ca/zmac.html), then choose the desired edition:

```sh
# EMT-10 march / cassette option
zmac space_invaders.asm
trs80gp -m3 zout/space_invaders.cmd

# EMT-10 title music
zmac space_invaders_emt_title.asm
trs80gp -m3 zout/space_invaders_emt_title.cmd

# Roland SC-55
zmac space_invaders_sc55.asm
trs80gp -m3 zout/space_invaders_sc55.cmd
```

The sources use `ORG 5500H`. For emulator high-score persistence, run the launcher after assembling the relevant edition:

```sh
python tools/play.py --edition title
```

Use `--edition march` or `--edition sc55` for the other editions. Set the `TRS80GP` environment variable if the executable is not on PATH.

See [INVEMT-BUILD.md](INVEMT-BUILD.md) for the existing experimental march binary's build provenance and [DEVELOPMENT.md](DEVELOPMENT.md) for architecture, graphics regression and audio testing notes.

## Original author and acknowledgements

This project is a fork of [TechPrototyper/trs80-space-invaders](https://github.com/TechPrototyper/trs80-space-invaders). Full credit and sincere thanks go to **Tim Walter (TechPrototyper)** for the original game. The upstream title-screen credits acknowledge Tim Walter and Claude (2024–2026), and inspiration from Space Invaders and Big Five Games.

The original Z80 game engine, formation movement, graphics, sprite generation, collision handling, shields, scoring, wave progression, cassette audio and byte-exact video RAM verification are inherited achievements. The original in-game credits and Git history are preserved.

This Model III fork is maintained by [Roy Edmund Antaw (royedmund)](https://github.com/royedmund). Its contributions include the EMT-10 march and original title soundtrack, SC-55 all-MIDI audio, RS-232/Nano integration and Model III-specific support.

Four-note march design reference: [Ron Jeffries — Space Invaders 37: Sounds](https://ronjeffries.com/articles/020-invaders-30ff/i-37/). This fork uses newly selected pitches and existing formation movement events.

The copied MIDI interface materials carry their own MIT license; it does not extend to the upstream game.
