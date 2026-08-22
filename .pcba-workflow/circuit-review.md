# Circuit review

- Project: Morse Vibe StickS3 Grove straight key
- Revision: `morse-v1.2` working tree
- Status: `USER_REVIEW`
- Electrical source of truth: M5Stack StickS3 official pin map plus
  `src/main.cpp` GPIO configuration; there is no custom PCB or native schematic
- Reviewed evidence: M5Stack StickS3 and Grove documentation, Espressif GPIO
  documentation, product brief, architecture, firmware, 12 native tests, and
  PlatformIO embedded build

## Operating principle

A passive normally-open Morse key shorts Grove white GPIO10 to Grove black GND.
The ESP32-S3 internal pull-up holds the open contact HIGH. Firmware debounces the
active-low input for 8 ms, OR-combines it with Button A, and passes only merged
press/release edges to the existing Morse decoder and 880 Hz sidetone.

## Functional blocks and component roles

| Block | Role |
|---|---|
| Passive straight key | Human-operated, unpowered normally-open contact |
| HY2.0-4P pigtail | Carries GND and GPIO10; 5 V and GPIO9 are isolated |
| ESP32-S3 GPIO10 | Digital input with internal approximately 45 kOhm pull-up |
| Firmware debounce/merge | Rejects contact bounce and prevents duplicate edges when Button A overlaps |
| Existing decoder/speaker/BLE | Converts timing, provides sidetone, and sends mapped commands |

## Power tree and budget

USB or the internal battery powers the StickS3. `[SPEC]` Grove external 5 V is
disabled. The passive key consumes only pull-up current while closed, on the
order of 3.3 V / 45 kOhm (approximately 73 uA, nominal documentation value).
`[TBD-MEASURE]` Actual closed-key current and full-device mode current.

## Voltage domains and interfaces

| Interface | Domain | Idle | Active | Constraint |
|---|---:|---:|---:|---|
| Grove black | 0 V | GND | GND | One key terminal only |
| Grove red | 5 V-capable | Disabled/input | Unused | Insulate; never connect to key or GPIO10 |
| Grove yellow / GPIO9 | 3.3 V GPIO | Unused | Unused | Insulate separately |
| Grove white / GPIO10 | 3.3 V GPIO | HIGH through internal pull-up | LOW through key to GND | Dry contact only |

## Operating states, reset, and ownership

- Boot: GPIO10 becomes `INPUT_PULLUP`; external 5 V remains disabled.
- KEY idle: Button A and debounced Grove contact are open.
- KEY down: either input owns the common decoder key-down state; both must open
  before a release edge is emitted.
- MIC: Grove transitions are ignored by the decoder; the built-in microphone
  owns Morse input.
- Mode change: decoder and merged key state are reset to prevent a stuck tone or
  phantom edge.

## Timing and signal integrity

`[TARGET]` 8 ms stable level rejects mechanical chatter while remaining much
shorter than the decoder's 50 ms minimum adaptive dot unit. `[TBD-MEASURE]`
Validate the purchased key and intended cable for bounce, capacitive pickup,
and false transitions.

## Protection, thermal, and mechanics

The direct prototype connection adds no external ESD, surge, series resistance,
or Schmitt buffer. It is restricted to short, indoor, dry-contact use. A powered
electronic keyer, exposed connector, or long cable requires a separate 3.3 V
conditioning/protection adapter. Insulate unused red and yellow conductors
individually so they cannot short together or to the key chassis.

## Layout handoff constraints

There is no custom layout in this revision. For a future adapter: `[TARGET]`
place ESD protection at the external connector, use a firm external pull-up to
3.3 V (not Grove 5 V), include modest series current limiting, and provide a
ground return adjacent to the signal.

## Findings

| ID | Severity | Evidence | Consequence | Action | Gate | Status |
|---|---|---|---|---|---|---|
| GK-01 | major | StickS3 Grove exposes 5 V beside GPIO10 | Miswiring red to the key/GPIO can damage the input | Use black+white only; separately insulate red+yellow | Wiring review | `USER_REVIEW` |
| GK-02 | minor | Internal pull-up is approximately 45 kOhm | Long/noisy cables may create false edges | Keep prototype cable short; bench-test actual key; add protected adapter if needed | Physical key test | `USER_REVIEW` |
| GK-03 | information | Native merger/debounce tests and firmware build pass | Software behavior is reproducible | Retain tests in CI | Software | `PASS` |

## Approved design decisions and waivers

- GPIO10 was selected at the user's request; the official StickS3 HY2.0-4P pin
  map assigns it to the white conductor.
- GPIO9 remains unused so the key needs only one signal and ground.
- Direct dry-contact wiring is accepted only for indoor prototype evaluation;
  it is not a productized external interface waiver.

## Verification and unresolved evidence

- `PASS`: 12/12 native tests, including 8 ms contact debounce and overlapping
  Button A/Grove press behavior.
- `PASS`: PlatformIO `m5stack-sticks3` firmware build, 820,205 bytes used.
- `USER_REVIEW`: continuity-test the actual key cable before connection.
- `USER_REVIEW`: flash `morse-v1.2`; verify GPIO10 source labels, crisp sidetone,
  dot/dash recognition, rapid repeated keying, and absence of false edges.
- `USER_REVIEW`: powered/electronic keyers and iambic paddles are unsupported.

Sources:

- https://docs.m5stack.com/en/core/StickS3
- https://docs.m5stack.com/en/learn/interface/grove
- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/gpio.html
