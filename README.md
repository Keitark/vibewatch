# Morse Vibe

**English** | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

Morse Vibe turns an M5StickS3 into a two-mode Morse command controller for the
Codex Micro/Vibewatch BLE protocol. Key commands with Button A or play an
audible Morse tone near the built-in microphone; the display shows the live
pattern, decoded character, mapped action, connection, and battery state.

This is an MIT-licensed derivative of
[GOROman/vibewatch](https://github.com/GOROman/vibewatch), pinned from upstream
commit `f2520875a61fe087cb7e5b63a2a61ddcc2e79cb2`.

## Controls

| Control | Behavior |
|---|---|
| Button A in KEY mode | Straight Morse key with a crisp 880 Hz ton/tsu sidetone |
| Button A in MIC mode | Restart the one-second ambient calibration |
| Button B single press | Force commit; choose a cut-number alias when one exists |
| Button B double press | Clear the pending symbol |
| Button B hold for 800 ms | Switch KEY/MIC mode |
| Hold A+B for 3 seconds during boot | Clear BLE bonds and advertise for pairing |

Automatic three-unit gaps always use the alphabet interpretation. This keeps
ambiguous command letters deterministic. To enter a shortened numeral, key the
same pattern and press Button B before the automatic gap commits it.

Examples:

- `.-` + automatic gap -> `A` -> AI
- `.-` + Button B -> cut `1` -> Agent 1
- `-.` + automatic gap -> `N` -> NG
- `-.` + Button B -> cut `9` -> displayed as unmapped

## Command map

| Decoded value | Host event | Intended host assignment |
|---|---|---|
| F | ACT06 | Fast |
| O | ACT07 | Approve / OK |
| N | ACT08 | Decline / NG |
| P | ACT09 | Toggle Plan mode |
| A | ACT12 | AI / Codex action |
| M | ACT10 + ACT11 | Toggle push-to-talk, with 30-second safety release |
| 1-6 | AG00-AG05 | Select agent/chat 1-6 |

Codex Micro action slots are host-configurable. Assign ACT09 to Plan and ACT12
to the desired AI/Codex action in the desktop app on each computer. Unmapped
letters and numbers are displayed but never transmitted.

## Cut numbers

Button B explicitly selects these common radio abbreviations:

| Number | Short pattern | Letter with the same pattern |
|---:|---|---|
| 0 | - | T |
| 1 | .- | A |
| 2 | ..- | U |
| 3 | ...- | V |
| 4 | ....- | standard 4 |
| 5 | . | E |
| 6 | -.... | standard 6 |
| 7 | -... | B |
| 8 | -.. | D |
| 9 | -. | N |

## Microphone mode

MIC mode listens for a coherent 600-1000 Hz beeper tone at 16 ksample/s. It
calibrates the ambient noise floor for one second, then applies band-energy,
concentration, threshold, and hysteresis checks. Spoken “dit” and “dah” syllables
are not a supported input.

The StickS3 microphone and speaker share the audio path, so the sidetone is
silent in MIC mode.

## Build, test, and upload

Install PlatformIO Core, then run:

```sh
python -m platformio test -e native
python -m platformio run -e m5stack-sticks3
```

On Windows without `gcc/g++`, the native tests need a MinGW compiler on PATH.
The embedded firmware build does not use that host compiler.

Connect the StickS3 by USB and upload only when you intend to write the device:

```sh
python -m platformio run -e m5stack-sticks3 --target upload
```

The device advertises as `Vibe Watch #1` to preserve the upstream host
compatibility path. Pair it in the computer's Bluetooth settings, then verify
that the ChatGPT/Codex desktop app exposes the Codex Micro controls.

## Verification status

- Native decoder/tone/mapper tests: passing
- StickS3 firmware build: passing
- Physical StickS3 flash, board autodetection, PSRAM, NimBLE startup, and
  `morse-v1.0` readiness banner: passing
- Physical StickS3 display, key sound, microphone, and power: USER_REVIEW
- macOS and Windows paired-host command matrix: USER_REVIEW

See [.pcba-workflow/architecture.md](.pcba-workflow/architecture.md) for system
boundaries and outstanding evidence. See
[docs/flash-verification.md](docs/flash-verification.md) for the sanitized
physical-write receipt.

## License

MIT. The upstream copyright and permission notice remain in [LICENSE](LICENSE).
