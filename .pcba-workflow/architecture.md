# Morse Vibe architecture

Status: `BLOCKED`. Twelve native tests and the `morse-v1.2` firmware build pass;
the earlier `morse-v1.1` flash, post-flash USB presence, and Windows
`Vibe Watch #1` BLE HID presence also pass. The user reports successful macOS
Codex Micro recognition, while ChatGPT desktop 26.805.11740 on Windows did not
expose Codex Micro settings. The `morse-v1.2` flash, Grove-key continuity, full
command matrix, and visual, acoustic, microphone, and power evidence remain open.

## Boundaries and actors

The hardware boundary is one unmodified M5StickS3 plus a passive normally-open
straight key and short Grove pigtail. The operator keys Button A, shorts Grove
GPIO10 to GND with the external key, or supplies a coherent 600-1000 Hz external
Morse tone. Button B commits,
clears, or changes mode. A paired ChatGPT/Codex host owns the meaning assigned
to each Codex Micro action slot. The firmware owns input timing, decoding,
display state, BLE framing, and the 30-second host-microphone safety release.

No powered external circuit, added rail, enclosure modification, or companion
desktop bridge is introduced. Grove red 5 V and yellow GPIO9 are unused.

## Functional flow

1. M5Unified initializes display, buttons, power, speaker, microphone hardware,
   and battery telemetry.
2. KEY mode merges already-debounced Button A with an 8 ms debounced active-low
   GPIO10 dry contact. The logical key stays down while either input is pressed,
   and the speaker produces the 880 Hz sidetone.
3. MIC mode disables the shared speaker path, calibrates one second of ambient
   noise, and converts band-qualified tone blocks into the same key edges.
4. The shared decoder classifies dots/dashes, adapts the time unit, and commits
   after a three-unit gap or Button B request.
5. Automatic commit selects the simplified-number alias where available.
   Button B force-commit explicitly selects the alphabet interpretation.
6. The mapper accepts only `F O X P C M 1 2 3 4 5 6`. `C` replaces the
   conflicting `A` command and `X` replaces the conflicting `N` command. Other decoded values are
   displayed without transmission.
7. BLE sends the upstream `v.oai.hid` JSON payload inside vendor report 6.
   Momentary controls receive DOWN and UP notifications 12 ms apart.
8. The display renders mode, connection, battery, pending pattern, decoded
   value, action label, microphone level, and recent history.

## Resource ownership and modes

| Mode | Audio owner | Button A | Button B | BLE |
|---|---|---|---|---|
| Startup | Speaker | Pair-reset chord detection | Pair-reset chord detection | Not started |
| KEY | Speaker | Straight key + sidetone; OR-combined with Grove GPIO10 | Commit / clear / mode | Advertising or connected |
| MIC calibration | Microphone | Restart calibration | Commit / clear / mode | Advertising or connected |
| MIC listening | Microphone | Restart calibration | Commit / clear / mode | Advertising or connected |
| Host MIC ON | Current input mode | Unchanged | Unchanged | ACT10+ACT11 held until M or safety release |

The StickS3 speaker and microphone are never intentionally active together.
Changing input mode first releases the host microphone, clears the pending
symbol, and hands the codec from one owner to the other.

## Interfaces and failure behavior

- BLE retains the upstream VID/PID, HID map, manufacturer/model strings, and
  advertised name `Vibe Watch #1` for host compatibility. Firmware version is
  `morse-v1.2`.
- Invalid Morse, unmapped characters, and disconnected operation never queue or
  send an event. The display states the reason.
- A failed BLE notification reports `SEND FAILED`. Disconnect clears local
  host-microphone state and restarts advertising.
- Host push-to-talk releases on the next `M`, after 30 seconds, on input-mode
  change, or before a controlled restart. A link loss prevents an explicit UP
  notification, so the host must also clear held controls on disconnect.
- A+B held for three seconds at boot clears all bonds. A shorter chord does not.
- GPIO10 is `INPUT_PULLUP`; the external key may only short it to GND. GPIO9 and
  Grove 5 V are not part of the key circuit. Firmware explicitly disables the
  StickS3 external 5 V output.

## Power and verification gates

The passive key adds no powered load. The weak internal GPIO pull-up and a short
indoor cable are acceptable for prototype evaluation; long/exposed wiring needs
3.3 V input conditioning, ESD protection, and a separate evidence gate.
Firmware build size is host-verifiable, but current draw and battery runtime
are `TBD_MEASURE` in KEY idle, active sidetone, MIC listening, BLE connected,
and charging modes.

Clearing `BLOCKED` requires resolving ChatGPT Codex Micro detection, then
completing the readable 135x240 rendering, crisp keying with measured latency,
controlled tone/noise trials, safe repeated audio-mode handoff, and the full
command matrix on both macOS and Windows hosts. Physical flash evidence is
recorded in `docs/flash-verification.md`.

The Windows host gate is currently `BLOCKED`, not `USER_REVIEW`: the operating
system accepts the BLE HID device, but ChatGPT does not identify it as a
supported Codex Micro. No command-matrix claim is valid until detection is
resolved and re-tested.
