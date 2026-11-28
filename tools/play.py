#!/usr/bin/env python3
"""Launch Model III Space Invaders with a persistent high score.

The game prints "HSnnnn\r" to the printer whenever a new high score is
set at game over. This script:
  1. reads hiscore.dat (repo root; plain 4-digit text),
  2. patches those digits into HISCORE_D inside a temp copy of the
     freshly assembled CMD (matching its zmac .bds symbol file),
  3. runs trs80gp with the printer captured to a file,
  4. harvests the highest HSnnnn from the capture back into hiscore.dat.

Usage: python tools/play.py [--edition march|title|sc55] [extra trs80gp args]
"""
import argparse
import os
import pathlib
import re
import shutil
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
EDITIONS = {'march': 'space_invaders', 'title': 'space_invaders_emt_title',
            'sc55': 'space_invaders_sc55'}
HISCORE = ROOT / 'hiscore.dat'


def emulator():
    configured = os.environ.get('TRS80GP')
    if configured:
        return configured
    found = shutil.which('trs80gp') or shutil.which('trs80gp.exe')
    if found:
        return found
    mac = pathlib.Path('/Applications/trs80gp.app/Contents/MacOS/trs80gp')
    if mac.is_file():
        return str(mac)
    raise SystemExit('Install trs80gp on PATH or set TRS80GP to its executable.')


def read_hiscore():
    try:
        text = HISCORE.read_text().strip()
        if re.fullmatch(r'\d{1,4}', text):
            return int(text)
    except FileNotFoundError:
        pass
    return 0


def symbol_addr(name, bds):
    pat = re.compile(rf'^([0-9a-f]{{4}}) [0-9a-f]{{4}} s {re.escape(name)}:')
    for line in bds.read_text().splitlines():
        m = pat.match(line)
        if m:
            return int(m.group(1), 16)
    raise SystemExit(f'symbol {name} not found in {bds}')


def patch_cmd(data: bytes, addr: int, values: bytes) -> bytes:
    """Patch bytes at Z80 address addr inside a TRS-80 /CMD image."""
    out = bytearray(data)
    i = 0
    patched = set()
    while i < len(out):
        if i + 2 > len(out):
            raise ValueError('Truncated CMD record header')
        rtype = out[i]
        length = out[i + 1]
        if rtype == 0x01:                      # load record
            count = length if length >= 3 else length + 256
            load = out[i + 2] | (out[i + 3] << 8)
            body = i + 4                       # file offset of first data byte
            ndata = count - 2
            if body + ndata > len(out):
                raise ValueError('Truncated CMD load record')
            for k, v in enumerate(values):
                off = addr + k - load
                if 0 <= off < ndata:
                    out[body + off] = v
                    patched.add(k)
            i = body + ndata
        elif rtype == 0x02:                    # entry point: last record
            break
        else:                                  # named/other: skip payload
            i += 2 + (length if length else 256)
    if patched != set(range(len(values))):
        raise ValueError('High-score address not fully present in CMD; rebuild matching symbols')
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument('--edition', choices=EDITIONS, default='march')
    args, extra = parser.parse_known_args()
    stem = EDITIONS[args.edition]
    cmd = ROOT / 'zout' / (stem + '.cmd')
    bds = ROOT / 'zout' / (stem + '.bds')
    if any(re.fullmatch(r'-m(?:[0-9]+)?', arg, re.I) and arg.lower() != '-m3'
           for arg in extra):
        raise SystemExit('Only the TRS-80 Model III (-m3) is supported.')
    if not cmd.is_file() or not bds.is_file():
        raise SystemExit(f'Build the selected Model III source first: zmac {stem}.asm')
    hiscore = read_hiscore()
    digits = bytes(int(d) for d in f'{hiscore:04d}')
    addr = symbol_addr('HISCORE_D', bds)
    patched = patch_cmd(cmd.read_bytes(), addr, digits)

    with tempfile.TemporaryDirectory(prefix='si_hiscore_') as tmp:
        tmp = pathlib.Path(tmp)
        game = tmp / (stem + '.cmd')
        game.write_bytes(patched)
        capture = tmp / 'printer.out'
        argv = [emulator(), '-m3', '-p', f'>{capture}', str(game)] + extra
        subprocess.run(argv)
        try:
            raw = capture.read_bytes()
        except FileNotFoundError:
            raw = b''

    best = max((int(m.group(1)) for m in re.finditer(rb'HS(\d{4})', raw)),
               default=0)
    if best > hiscore:
        HISCORE.write_text(f'{best:04d}\n')
        print(f'new high score saved: {best:04d}')
    elif hiscore:
        print(f'high score stays at {hiscore:04d}')


if __name__ == '__main__':
    main()
