# INVEMT.CMD experimental build

- Source: `space_invaders.asm` at commit `5c7c36fbd1d89839a251a211824ce15029d3b002`
- Source Git blob: `1ddca6a4cb9c745c40f65e814f225129f07ba1fe`
- Binary Git blob: `bd3fa7c038e72139febc0e312e394c6f12c07ef2`
- CMD size: 3,856 bytes; contiguous payload 3,792 bytes
- Load: 5500h–63CFh; transfer address 5500h
- HISCORE_D: 6337h

Built using a custom two-pass assembler supporting the Z80 instructions and directives used by this file. Both passes agreed on labels and instruction sizes, relative branch ranges were checked, and the CMD load records and entry point were decoded and compared to the generated memory image. No zmac comparison, emulator execution or real-hardware test has been performed.

```sh
trs80gp -m3 INVEMT.CMD
```

On LDOS, transfer the CMD in binary mode as INVEMT/CMD and enter INVEMT. Hold M while pressing SPACE for EMT-10 audio; SPACE alone starts the cassette-only mode. Use the fast v0.3 Nano in RAW mode. The original release CMD has not been replaced.

For an independent assembly, run `zmac space_invaders.asm` and compare the decoded load image and transfer address, rather than relying on CMD record grouping alone.
