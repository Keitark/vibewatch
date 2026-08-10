# Morse Vibe architecture

Status: `USER_REVIEW`. Native tests, firmware build, physical flash, StickS3
autodetection, PSRAM initialization, NimBLE startup, and the firmware readiness
banner pass. Visual, acoustic, microphone, power, macOS, and Windows acceptance
evidence remains open.

## Boundaries and actors

The hardware boundary is one unmodified M5StickS3. The operator keys Button A
or supplies a coherent 600-1000 Hz external Morse tone. Button B commits,
clears, or changes mode. A paired ChatGPT/Codex host owns the meaning assigned
to each Codex Micro action slot. The firmware owns input timing, decoding,
display state, BLE framing, and the 30-second host-microphone safety release.

No external circuit, connector, rail, enclosure modification, or companion
desktop bridge is introduced.

## Functional flow

1. M5Unified initializes display, buttons, power, speaker, microphone hardware,
   and battery telemetry.
2. KEY mode converts Button A edges to timed key-down/key-up events while the
   speaker produces the 880 Hz sidetone.
3. MIC mode disables the shared speaker path, calibrates one second of ambient
   noise, and converts band-qualified tone blocks into the same key edges.
4. The shared decoder classifies dots/dashes, adapts the time unit, and commits
   after a three-unit gap or Button B request.
5. Automatic commit uses the alphabet table. Button B force-commit selects the
   cut-number alias where available, preventing `A/1` and `N/9` ambiguity.
6. The mapper accepts only `F O N P A M 1 2 3 4 5 6`. Other decoded values are
   displayed without transmission.
7. BLE sends the upstream `v.oai.hid` JSON payload inside vendor report 6.
   Momentary controls receive DOWN and UP notifications 12 ms apart.
8. The display renders mode, connection, battery, pending pattern, decoded
   value, action label, microphone level, and recent history.

## Resource ownership and modes

| Mode | Audio owner | Button A | Button B | BLE |
|---|---|---|---|---|
| Startup | Speaker | Pair-reset chord detection | Pair-reset chord detection | Not started |
| KEY | Speaker | Straight key + sidetone | Commit / clear / mode | Advertising or connected |
| MIC calibration | Microphone | Restart calibration | Commit / clear / mode | Advertising or connected |
| MIC listening | Microphone | Restart calibration | Commit / clear / mode | Advertising or connected |
| Host MIC ON | Current input mode | Unchanged | Unchanged | ACT10+ACT11 held until M or safety release |

The StickS3 speaker and microphone are never intentionally active together.
Changing input mode first releases the host microphone, clears the pending
symbol, and hands the codec from one owner to the other.

## Interfaces and failure behavior

- BLE retains the upstream VID/PID, HID map, manufacturer/model strings, and
  advertised name `Vibe Watch #1` for host compatibility. Firmware version is
  `morse-v1.0`.
- Invalid Morse, unmapped characters, and disconnected operation never queue or
  send an event. The display states the reason.
- A failed BLE notification reports `SEND FAILED`. Disconnect clears local
  host-microphone state and restarts advertising.
- Host push-to-talk releases on the next `M`, after 30 seconds, on input-mode
  change, or before a controlled restart. A link loss prevents an explicit UP
  notification, so the host must also clear held controls on disconnect.
- A+B held for three seconds at boot clears all bonds. A shorter chord does not.

## Power and verification gates

The project adds no rail or load beyond the StickS3's built-in peripherals.
Firmware build size is host-verifiable, but current draw and battery runtime
are `TBD_MEASURE` in KEY idle, active sidetone, MIC listening, BLE connected,
and charging modes.

Promotion from `USER_REVIEW` requires readable 135x240 rendering, crisp keying
with measured latency, controlled tone/noise trials, safe repeated audio-mode
handoff, and the full command matrix on both macOS and Windows hosts. Physical
flash and boot are recorded in `docs/flash-verification.md`.
