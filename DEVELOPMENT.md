# Development and graphics verification

The following architecture and verification notes describe the original Model III game engine developed by Tim Walter (TechPrototyper). The MIDI editions retain this foundation. These notes are preserved for maintenance; the newer audio editions still require emulator and real-hardware regression tests.

**Screenshots:** trs80gp defaults to `-va` ("authentic display") and bakes
scanlines and phosphor blur into `-ics` captures — 229 colours and ~50 KB of
CRT simulation. `-vi -v1` gives the raw pixel matrix the machine actually puts
out: 512×384 on the Model III, two colours, under 1 KB.

```bash
trs80gp -m3 -batch -vi -v1 -id 1700 -ics -ix space_invaders.cmd
```

## The interesting part: byte-exact verification

This game was developed against two reference screenshots of the original it
recreates (`oldproj/StartGame.png`, `oldproj/StartGameOneMoveRight.png`).
Instead of eyeballing the screen, every change is validated by a pipeline that
proves the video RAM matches the targets **byte for byte**:

1. `tools/extract_grid.py` deterministically converts the reference PNGs into
   a 128×48 pixel matrix and the corresponding 64×16 character codes.
2. trs80gp runs headless (`-batch -turbo`), injects key presses through the
   keyboard matrix (`-ik`), and dumps raw text VRAM (`-it`) at precise frame
   counts.
3. `tools/verify.py` compares the dumps against the targets cell by cell.
4. `tools/validate_full.py` goes further: it detects the formation position on
   screen, reconstructs the *expected* full frame, and flags any residue pixel
   the incremental renderer left behind.

The 18 invader sprite variants (3 types × 2 animation states × 3 vertical
sub-positions) are not hand-drawn: `tools/gen_sprites.py` generates them from
the reference screenshots, which is what makes byte-exactness attainable.

## Architecture notes

- **One formation origin** (`FORM_COL/ROW/SUB/YSUB`) positions all 55 invaders —
  no per-invader address bookkeeping.
- `FORM_SUB` doubles as horizontal 1-pixel shift *and* animation state;
  `FORM_YSUB` gives 1-pixel vertical descent with character-row rollover.
- Sprites are blitted 5 characters wide with blank margins, so movement is
  self-cleaning: no erase pass, no flicker.
- Collision is resolved by screen peek plus back-calculation onto the
  formation slot.
- March speed comes from an 8.8 fixed-point speed table indexed by the live
  invader count, accumulated per frame.
- High score persistence abuses the printer port: on a new record the game
  prints `HSnnnn`, `tools/play.py` captures it and patches it back into the
  binary on next launch (Model III printer port F8h).
- The credits ticker on row 0 of the first splash models an endless strip of
  three copies of the message, each followed by 32 blanks: column *c* shows
  strip index `s - 64 + c`, so the text enters at the right edge and the gap
  falls out of the arithmetic instead of being counted. The row is composed in
  a RAM buffer and blitted with one `LDIR` — a partial row is on screen for
  ~0.7 ms, far too briefly for the beam to catch a half-shifted line. Its state
  is never initialised: `MARQ_S` and `MARQ_DONE` come from the load image as
  zero, which is why it runs on the first splash and no later one.
- Row 0 sits outside the verification zone (rows 1–15, columns 5–63), so the
  ticker cost nothing in byte-exactness.
- `BREAK` is bit 2 of the same keyboard row the game already polls (`3840H`).
  Leaving to DOS is `EI` + `JP 402DH`, the standard Model III DOS re-entry.

## Audio regression checklist

- Build each source with zmac; run the CMD in trs80gp in Model III mode without `-turbo` for timing tests.
- Cassette-only: start with SPACE in the march edition; verify shooting, UFO, explosion and death effects.
- EMT-10 title edition: confirm automatic title music, M toggle, M+SPACE, SPACE handover, four-note march, wave transitions, game over and BREAK return to title; confirm no lingering notes on exit.
- SC-55: check all MIDI effects, simultaneous march and effects, cleanup on BREAK/game over, and absence of cassette audio.
- With the module connected, test the raw 9600-baud bridge; if UART transmit-ready fails, confirm bounded waits and appropriate audio fallback. A disconnected cable cannot be detected reliably by this one-way interface.

For firmware and electrical details, see [hardware/README.md](hardware/README.md).
