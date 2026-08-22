# Morse Vibe

**English** | [日本語](README.ja.md) | [简体中文](README.zh-CN.md)

Morse Vibe turns an M5StickS3 into a two-mode Morse command controller for the
Codex Micro/Vibewatch BLE protocol. Key commands with Button A, a passive Morse
straight key connected through Grove, or play an
audible Morse tone near the built-in microphone; the display shows the live
pattern, decoded character, mapped action, connection, and battery state.

This is an MIT-licensed derivative of
[GOROman/vibewatch](https://github.com/GOROman/vibewatch), pinned from upstream
commit `f2520875a61fe087cb7e5b63a2a61ddcc2e79cb2`.

## Controls

| Control | Behavior |
|---|---|
| Button A in KEY mode | Straight Morse key with a crisp 880 Hz ton/tsu sidetone |
| Grove dry-contact key in KEY mode | Same Morse input and sidetone through GPIO10 |
| Hold Button A or the Grove key for 1.2 seconds | Switch SIMPLIFIED/NORMAL decoder mode without entering a Morse symbol |
| Button A in MIC mode | Restart the one-second ambient calibration |
| Button B single press | Force commit immediately using the active decoder mode |
| Button B double press | Clear the pending symbol |
| Button B hold for 800 ms | Switch KEY/MIC input mode |
| Hold Button A + Button B for 1.5 seconds | Switch Codex controller / Morse Trainer; the gesture is consumed |
| Hold A+B for 3 seconds during boot | Clear BLE bonds and advertise for pairing |

## Morse Trainer

Hold Button A and Button B together for 1.5 seconds at runtime to enter the
standalone trainer. The display changes to `MORSE TRAIN`. Hold Button A or the
Grove key for 1.2 seconds to switch its independent SIMPLE/NORMAL setting. In
SIMPLE, problems are digits 0-9 answered with shortened-number Morse. In NORMAL,
problems are standard-Morse A-Z/0-9. Enter the answer with Button A, the Grove
key, or microphone tone. Live pattern and tentative-answer previews remain
active; timeout or Button B commits the answer.

A correct answer advances after a short confirmation. Wrong and invalid
submissions show `RETRY 1/3` then `RETRY 2/3`. The third failure reveals the
correct Morse pattern for 2.2 seconds and then advances automatically. Leaving
the trainer restores the Codex decoder mode unchanged.

Trainer answers are checked locally and never mapped or sent as Codex Micro BLE
commands, so training works without a paired host. Hold A+B for 1.5 seconds
again to return to the Codex controller. The combination remains consumed until
both buttons are released, preventing accidental key, commit, clear, or input-
mode actions.

## Grove straight key

Connect a passive normally-open straight key between Grove **white (GPIO10)**
and **black (GND)**. Leave red (5 V) and yellow (GPIO9) disconnected and
insulated. See the illustrated [Grove connection chart](docs/grove-key-connection.md).
Button A and the Grove key can be used interchangeably; if both are held, the
logical key releases only after both are open.

The decoder starts in SIMPLIFIED mode on every boot. Hold Button A or the Grove
Morse key continuously for 1.2 seconds to switch to NORMAL alphabet mode; hold
either Morse input again to return to SIMPLIFIED. The long hold is consumed as
a mode gesture and is not entered as a dash. A short Button B press commits
immediately without changing the active mode.
Command letters use conflict-free patterns.

On the first press of a new Morse character, the previous decoded character and
action are cleared. The display then shows the pending pattern live (`_` marks
the currently held key). After every release it also shows the character
represented by that partial pattern in the active decoder mode as `PENDING`
(for example, `.-` -> `1` in SIMPLIFIED and `.-` -> `A` in NORMAL). No command
is sent until the character is fixed by the automatic three-unit gap or by a
Button B single-press commit.

While a key remains down, `_` marks the live contact. When its duration reaches
the same two-unit threshold used to classify a released dash, the tentative
pattern and active-mode character update immediately (for example, `-_` and
`0 PENDING` in SIMPLIFIED). The preview does not modify or send the decoder
result. Continuing the hold to 1.2 seconds cancels it and switches decoder mode.

Examples:

- SIMPLIFIED: `.-` -> cut `1` -> Agent 1
- NORMAL: `.-` -> `A` -> displayed as unmapped
- SIMPLIFIED: `-.` -> cut `9` -> displayed as unmapped
- NORMAL: `-.` -> `N` -> displayed as unmapped

## Command map

| Decoded value | Host event | Intended host assignment |
|---|---|---|
| F | ACT06 | Fast |
| O | ACT07 | Approve / OK |
| X | ACT08 | Decline / NG |
| P | ACT09 | Toggle Plan mode |
| C | ACT12 | AI / Codex action |
| M | ACT10 + ACT11 | Toggle push-to-talk, with 30-second safety release |
| 1-6 | AG00-AG05 | Select agent/chat 1-6 |

Codex Micro action slots are host-configurable. Assign ACT09 to Plan and ACT12
to the desired AI/Codex action in the desktop app on each computer. Unmapped
letters and numbers are displayed but never transmitted.

## Simplified numbers (default)

SIMPLIFIED mode selects these common radio abbreviations. Hold Button A or the
Grove Morse key for 1.2 seconds to use the alphabet character in the right-hand
column instead:

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

- Native decoder/trainer/tone/mapper/Grove-input tests: 16/16 passing
- `morse-v1.2` simplified-number-default StickS3 firmware build: passing
- `morse-v1.3.3` Morse-key 1.2-second SIMPLIFIED/NORMAL toggle build, physical
  COM3 flash, verified image writes, boot, BLE reconnect, and RPC exchange: passing
- `morse-v1.4.2` clear-on-first-key, mode-aware tentative preview, live pending
  display, and commit-gated send:
  14/14 tests, firmware build, COM3 flash, verified writes, and boot passing
- `morse-v1.4.3` held-dash realtime preview: 15/15 tests, build, COM3 flash,
  verified writes, and boot passing
- `morse-v1.5.0` standalone A-Z/0-9 trainer and latched A+B mode switch:
  16/16 tests, build, COM3 flash, verified writes, and boot passing
- `morse-v1.5.1` independent SIMPLE/NORMAL training sets and three-attempt
  answer reveal: 16/16 tests, build, COM3 flash, verified writes, and boot passing
- Physical 1.2-second mode-toggle gesture on Button A and Grove key: USER_REVIEW
- Earlier `morse-v1.0` physical flash, board autodetection, PSRAM, NimBLE
  startup, and readiness banner: passing
- Earlier `morse-v1.1` Grove firmware physical flash: passing
- `morse-v1.2` simplified-number key check: USER_REVIEW
- Physical StickS3 display, key sound, microphone, and power: USER_REVIEW
- macOS Codex Micro recognition: user-reported passing; full macOS and Windows
  command matrix: USER_REVIEW

See [.pcba-workflow/architecture.md](.pcba-workflow/architecture.md) for system
boundaries and outstanding evidence. See
[docs/flash-verification.md](docs/flash-verification.md) for the sanitized
physical-write receipt.

## License

MIT. The upstream copyright and permission notice remain in [LICENSE](LICENSE).
