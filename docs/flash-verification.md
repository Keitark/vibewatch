# StickS3 flash verification

Date: 2026-08-10 (Asia/Tokyo)

This receipt records the local firmware build and physical-device write. The
device's unique MAC address is intentionally omitted from the public artifact.

## Target identification

- Local port: `COM3`
- USB VID:PID: `303A:1001`
- Chip: `ESP32-S3-PICO-1`, revision 0.2
- Embedded flash: 8 MB
- Embedded PSRAM: 8 MB
- USB mode: USB-Serial/JTAG

The target was identified with an esptool `chip_id` read before any write.

## Validation and write

| Check | Result |
|---|---|
| `python -m platformio test -e native` | PASS, 10/10 |
| `python -m platformio run -e m5stack-sticks3` | PASS |
| `python -m platformio run -e m5stack-sticks3 --target upload --upload-port COM3` | PASS |
| Bootloader, partition, boot-app, and application write hashes | Verified by esptool |

Final application artifact:

- Path: `.pio/build/m5stack-sticks3/firmware.bin`
- Size: 820,000 bytes
- SHA-256: `5A15307A447A6EA9C80859FBF14D84BB3DAC1FD53862304AFE545626EE3C454F`

## Post-flash runtime evidence

Opening the USB serial port at 115200 baud reset the target and captured:

```text
[M5GFX] [Autodetect] board_M5StickS3
I NimBLEDevice: NimBle host synced.
MORSE_VIBE_BLE name=Vibe Watch #1 version=morse-v1.0
MORSE_VIBE_READY board=M5StickS3 mode=KEY bonds_cleared=0
```

These lines prove that the flashed image booted, selected the StickS3 hardware,
initialized NimBLE, began the firmware BLE path, and reached the main KEY mode.

## Grove-key `morse-v1.1` update

Date: 2026-08-10 (Asia/Tokyo)

The Grove-key image was built and written to the same COM3 StickS3 target:

| Check | Result |
|---|---|
| Local firmware revision | `morse-v1.1` |
| Host-facing compatibility version | Previously `morse-v1.1`; see compatibility rebuild below |
| Grove key configuration | GPIO9 active LOW, internal pull-up, 8 ms debounce |
| PlatformIO StickS3 build | PASS, 820,205 bytes program usage |
| Application binary | 820,576 bytes |
| SHA-256 | `718F6E4F2EA84CB561AA3D2471C631B5323E0C1F94AF57C2A0D1D1BE0B86A215` |
| Upload to COM3 | PASS |
| Bootloader, partition, boot-app, and application write hashes | Verified by esptool |
| Post-flash USB device | PASS: ESP32-S3 USB composite, JTAG/serial, and COM3 present |
| Post-flash Windows BLE device | PASS: `Vibe Watch #1` present with BLE HID service |
| ChatGPT Codex Micro recognition | **BLOCKED: not recognized** |

The first post-flash serial capture saw PSRAM initialization, but the readiness
banner was not captured before the USB serial interface re-enumerated. USB and
BLE device presence prove the target is active, but this receipt does not claim
a `morse-v1.1` serial-banner pass.

ChatGPT desktop AppX version 26.803.5235.0 was inspected directly. Settings had no
Codex Micro section, and searching Settings for `Codex Micro` returned only the
unrelated keyboard shortcut `Switch to Codex`. OpenAI's documentation says the
Codex Micro settings remain available after ChatGPT detects a supported Micro,
so the current firmware is not accepted by ChatGPT as a supported Micro.

## `v1.0` compatibility rebuild

Date: 2026-08-10 (Asia/Tokyo)

The BLE Device Information characteristic and JSON-RPC responses now report the
upstream compatibility value `v1.0`. Serial diagnostics separately identify the
local firmware as `morse-v1.1`.

| Check | Result |
|---|---|
| Native tests | PASS, 12/12 |
| PlatformIO StickS3 build | PASS, 820,229 bytes program usage |
| Application binary | 820,592 bytes |
| SHA-256 | `106C6CB2FB98B90D3FAA74FE87D4FCAD9D964296FC2EB9F2EA33C3D562AE6CF8` |
| Binary string inspection | PASS: contains both `v1.0` and `morse-v1.1` |
| Device upload | PASS: COM3, all written-region hashes verified by esptool |
| Post-flash USB device | PASS: COM3 re-enumerated with status OK |
| Post-flash boot banner | PASS: `version=v1.0 local=morse-v1.1`, Grove GPIO9, READY |
| ChatGPT Codex Micro recognition | **BLOCKED: no Micro settings surface after live retest** |

## ChatGPT BLE/RPC diagnostic capture

Date: 2026-08-10 (Asia/Tokyo)

The firmware was temporarily instrumented first to print every complete
JSON-RPC request and response, then extended to queue and print every raw write
to the vendor output characteristic before validating its channel or framing.
The final diagnostic image was built, tested, and written to the physical
StickS3 before the decisive timed serial capture.

| Check | Result |
|---|---|
| Native tests | PASS, 12/12 |
| PlatformIO StickS3 build | PASS, 820,673 bytes program usage |
| Application binary | 821,040 bytes |
| SHA-256 | `90FE95BCF14F9E84ABD9FA4E8C8DCC0DE1FAA0A7B2D4AF0AAE06CB6A6C78E40E` |
| Device upload | PASS: COM3, all written-region hashes verified by esptool |
| BLE connection and report subscriptions | PASS in three successful captures |
| ChatGPT raw vendor-output write (`RPC RAW`) | **BLOCKED: none received** |
| ChatGPT JSON-RPC request (`RPC RX`) | **BLOCKED: none received** |
| ChatGPT JSON-RPC response (`RPC TX`) | **BLOCKED: none sent because no request arrived** |
| Controlled Windows HID `sys.version` query | PASS: raw write, parse, and `v1.0` response |

First capture:

```text
14.343s subscribe event; attr_handle=8, subscribed: true
14.343s subscribe event; attr_handle=58, subscribed: true
14.343s subscribe event; attr_handle=35, subscribed: true
14.343s subscribe event; attr_handle=39, subscribed: true
14.343s subscribe event; attr_handle=43, subscribed: true
14.343s subscribe event; attr_handle=47, subscribed: true
14.343s mtu update event; conn_handle=1 mtu=255
```

Second capture, while opening ChatGPT Settings and searching for `Codex Micro`:

```text
9.484s Connection failed rc = 8
10.531s subscribe event; attr_handle=8, subscribed: true
10.531s subscribe event; attr_handle=58, subscribed: true
10.547s subscribe event; attr_handle=35, subscribed: true
10.547s subscribe event; attr_handle=39, subscribed: true
10.562s subscribe event; attr_handle=43, subscribed: true
10.562s subscribe event; attr_handle=47, subscribed: true
10.562s mtu update event; conn_handle=1 mtu=255
```

Neither trace contains an `RPC RX` or `RPC TX` record. The live result therefore
separates the failure boundary: Windows establishes the BLE HID connection,
negotiates MTU 255, and subscribes to the reports, but ChatGPT does not write a
JSON-RPC request to the firmware's report-ID-6 vendor output characteristic.
The Settings search still exposes only `Switch to Codex`, not a Codex Micro
panel. There is consequently no serial RPC exchange to decode in this build;
the failure happens before the firmware's JSON-RPC parser.

The raw-write image was then flashed. An initial 35-second trace recorded three
connection timeouts (`rc = 8`), so that run was not used to judge report
traffic. A subsequent 55-second trace captured a successful reconnect:

```text
0.828s subscribe event; attr_handle=8, subscribed: true
0.844s subscribe event; attr_handle=58, subscribed: true
0.844s subscribe event; attr_handle=35, subscribed: true
0.844s subscribe event; attr_handle=39, subscribed: true
0.859s subscribe event; attr_handle=43, subscribed: true
0.859s subscribe event; attr_handle=47, subscribed: true
0.890s mtu update event; conn_handle=1 mtu=255
```

That trace contains no `RPC RAW`, `RPC RX`, or `RPC TX` record. Because the raw
logger runs before channel, length, and JSON validation, the absence proves that
no write reached the report-ID-6 vendor output characteristic during the
successful connection. The current blocker is therefore before the firmware's
RPC framing/parser and is not a malformed JSON response from this derivative.

As a control, a local Windows HID client opened the single enumerated vendor
interface (`VID 303A`, `PID 8360`, usage page `FF00`) and safely sent one
`sys.version` request through report ID 6. Serial captured the complete path:

```text
RPC RAW bytes=63 hex=02217B226964223A3939312C226D6574686F64223A227379732E76657273696F6E227D...
RPC RX bytes=33 json={"id":991,"method":"sys.version"}
RPC TX method=sys.version id=991 bytes=61 json={"id":991,"method":"sys.version","result":{"version":"v1.0"}}
```

The host wrote a 64-byte HID report; Windows removed the report-ID byte before
delivering the 63-byte value through BLE, exactly matching the firmware's
expected channel-2 framing. This control proves that the vendor interface,
report map, raw logger, JSON parser, and response transmission are operational.
The missing ChatGPT exchange is therefore caused by ChatGPT not opening or
initiating its Codex Micro control plane for this device, despite the normal
Windows BLE HID connection and subscriptions.

## Full GOROman RPC parity rebuild

Date: 2026-08-10 (Asia/Tokyo)

The Morse firmware's host-facing control plane was brought back to the complete
method set implemented by `GOROman/vibewatch` while retaining the Morse UI and
Grove key behavior. It now processes `device.status`, `sys.version`,
`v.oai.thstatus`, `v.oai.rgbcfg`, and `host.focused_app`; the latter three
notification-only messages are stored locally just as the upstream firmware
consumes them.

| Check | Result |
|---|---|
| HID report map, VID/PID, report ID, manufacturer/model, and `v1.0` version | Exact upstream values retained |
| Upstream RPC method set | PASS, 5/5 methods present |
| Native tests | PASS, 12/12 |
| PlatformIO StickS3 build | PASS, 822,601 bytes program usage |
| Application binary | 822,960 bytes |
| SHA-256 | `170D9891397651952C03CE893541C9D0B8D2C7790E666FC1CC9BE0210892B879` |
| Device upload | PASS: COM3, all written-region hashes verified by esptool |
| First post-flash reconnect window | BLOCKED: repeated BLE timeout `rc = 8` before subscriptions |
| Second 55-second retry window | BLOCKED: no connection or report traffic |

Because these attempts did not reach a BLE connection, they cannot judge
ChatGPT RPC behavior. The current next gate is a clean Windows Bluetooth
unpair, firmware bond clear, and re-pair so Windows rebuilds the BLE HID/GATT
cache for the parity image.

## Clean Windows and firmware re-pair

Date: 2026-08-10 (Asia/Tokyo)

With user approval, Windows removed the existing `Vibe Watch #1` pairing. A
temporary one-shot image then called `NimBLEDevice::deleteAllBonds()` and its
serial banner confirmed `bonds_cleared=1`. The temporary source change was
reverted, the normal full-parity image was rebuilt and flashed, and Windows
paired the newly advertising device successfully.

The normal image's post-pair reset produced:

```text
2.110s subscribe event; attr_handle=8, subscribed: true
2.110s subscribe event; attr_handle=58, subscribed: true
2.125s subscribe event; attr_handle=35, subscribed: true
2.125s subscribe event; attr_handle=39, subscribed: true
2.125s subscribe event; attr_handle=43, subscribed: true
2.141s subscribe event; attr_handle=47, subscribed: true
2.235s mtu update event; conn_handle=1 mtu=255
```

No `RPC RAW`, `RPC RX`, or `RPC TX` record followed during the 45-second
capture. ChatGPT Settings still had no Codex Micro category, and searching for
`Codex Micro` still returned only the unrelated `Switch to Codex` keyboard
shortcut. The clean re-pair therefore fixed the BLE connection timeout but did
not advance ChatGPT discovery. A stale Windows bond/GATT cache is no longer a
sufficient explanation; the remaining gates are a complete ChatGPT process
restart and, if that remains silent, comparison with the official Micro's
unpublished Windows-visible identity/fingerprint.

## Full ChatGPT process restart

Date: 2026-08-10 (Asia/Tokyo)

With user approval, every running ChatGPT desktop UI process was terminated and
the packaged application was launched again. The relaunched main window had a
new process set and returned to this task successfully. The device was then
reset during a new 50-second serial capture.

The restarted app/Windows stack made one connection attempt that timed out, then
connected and subscribed normally:

```text
10.781s Connection failed rc = 8
11.140s subscribe event; attr_handle=8, subscribed: true
11.140s subscribe event; attr_handle=58, subscribed: true
11.140s subscribe event; attr_handle=35, subscribed: true
11.156s subscribe event; attr_handle=39, subscribed: true
11.156s subscribe event; attr_handle=43, subscribed: true
11.156s subscribe event; attr_handle=47, subscribed: true
11.203s mtu update event; conn_handle=1 mtu=255
```

No `RPC RAW`, `RPC RX`, or `RPC TX` followed. A full ChatGPT restart therefore
does not change the Windows result. With the GOROman host-facing method set
restored, clean bonds on both sides, a fresh pairing, and a fresh ChatGPT
process, the remaining evidence points to a Windows-only support/fingerprint
gap rather than firmware JSON-RPC behavior.

## Remaining USER_REVIEW gates

- Visual inspection of all display states
- Subjective crispness and measured latency of the ton/tsu sidetone
- Button-key timing across operator speeds
- Controlled microphone-tone and ambient-noise trials
- Native Codex Micro detection and every command on macOS and Windows
- Current draw and battery runtime by mode
## `morse-v1.2` simplified-number-default build

This revision changes decoding preference, command letters, and the external
key input from Grove GPIO9 to GPIO10 without changing the upstream host-facing
BLE identity. Automatic commits prefer simplified
numbers; Button B selects the alphabet alternate. The conflicting commands are
remapped from `A` to `C` for Codex and from `N` to `X` for decline/NG.

| Check | Result |
|---|---|
| Native tests | PASS: 12/12 |
| StickS3 build | PASS: 822,617 bytes flash, 34,124 bytes RAM |
| Firmware binary SHA-256 | `478B0BCF41FCBCD6C1B188A308917169DBC167C1137F051C1E67636388C6E670` |
| Physical flash | PASS: COM3, ESP32-S3-PICO-1, 8 MB flash/PSRAM; every written segment hash verified |
| Post-flash boot | PASS: M5StickS3 autodetected, PSRAM enabled, NimBLE synchronized |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.2`, `pin=10`, `grove_key=G10` |
| BLE bonds | Preserved: `bonds_cleared=0` |
| macOS recognition | USER_REPORTED_PASS before this flash; reconnection of `morse-v1.2` requires user confirmation |

## `morse-v1.3.3` Morse-key decoder-mode toggle

This revision keeps Button B as the control button. Holding either Morse input
(Button A or the Grove GPIO10 key) continuously for 1.2 seconds toggles the
decoder between SIMPLIFIED and NORMAL. The gesture is consumed and is not
entered as a long dash. SIMPLIFIED remains the boot default.

| Check | Result |
|---|---|
| Decoder/input tests | PASS: 12/12 under WSL g++ 11.4.0 |
| StickS3 build | PASS: 822,957 bytes flash, 34,140 bytes RAM |
| Firmware binary SHA-256 | `C61015DEB8AC4268DAFB7F1D42822F7ECE0CB2BCE155699C4AF49046ECE08986` |
| Physical flash | PASS: COM3, ESP32-S3-PICO-1, MAC `70:04:1D:DA:7F:D0`; all written segments hash-verified |
| Post-flash boot | PASS: M5StickS3 autodetected, PSRAM enabled, NimBLE synchronized |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.3.3`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| BLE/RPC after flash | PASS: reconnect, subscriptions, MTU 255, `v.oai.rgbcfg`, `v.oai.thstatus`, and `device.status` exchanges observed |
| Physical 1.2-second hold toggle | USER_REVIEW: confirm `SIMPLE` changes to `NORMAL` with Button A and Grove key |

## `morse-v1.4` live pending decode and commit-gated send

The first key-down of a new character clears the previous decoded character and
action. While input is pending, the display shows the accumulated Morse pattern;
`_` marks a currently held key. Character dispatch remains blocked until either
the automatic three-unit gap or a Button B single-press commit fixes the result.

| Check | Result |
|---|---|
| Decoder/input tests | PASS: 13/13, including no character before commit |
| StickS3 build | PASS: 823,245 bytes flash, 34,140 bytes RAM |
| Firmware binary SHA-256 | `4B6E326DFB5FEC13306E4C072D90549FBF2F59B56D41524DDDAE05087082C739` |
| Physical flash | PASS: COM3, all written segments hash-verified |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.4`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| BLE after flash | PASS: reconnect, subscriptions, and MTU 255 observed |
| Physical display lifecycle and command timing | USER_REVIEW |

## `morse-v1.4.1` tentative alphabet preview

After each dot or dash is released, the display shows the standard Morse
alphabet represented by the current partial pattern as `PENDING`. This preview
does not dispatch a command. Only the automatic gap or Button B commit produces
the final mode-dependent result and sends its mapped command.

| Check | Result |
|---|---|
| Decoder/input tests | PASS: 13/13; pending preview remains pre-commit |
| StickS3 build | PASS: 823,409 bytes flash, 34,140 bytes RAM |
| Firmware binary SHA-256 | `F8CD8AC675999D48F9EFDE9C02E8D23E973C3543897EF038B243E950E91CA476` |
| Physical flash | PASS: COM3, all written segments hash-verified |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.4.1`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| Physical preview sequence and final dispatch timing | USER_REVIEW |

## `morse-v1.4.2` mode-aware tentative preview

Pending preview and final commit now use the same active-mode resolver. In
SIMPLIFIED mode, `.-` previews and commits as `1`; in NORMAL mode it previews
and commits as `A`. Preview remains display-only until the automatic gap or a
Button B commit.

| Check | Result |
|---|---|
| Decoder/input tests | PASS: 14/14, including SIMPLE `.-` -> `1` and NORMAL `.-` -> `A` before commit |
| StickS3 build | PASS: 823,441 bytes flash, 34,140 bytes RAM |
| Firmware binary SHA-256 | `DD3D7E76C1CB4FBFD90CF0C0A6010E1C49BE74FE335E94167CE555801F625290` |
| Physical flash | PASS: COM3, ESP32-S3-PICO-1, MAC `70:04:1D:DA:7F:D0`; all written segments hash-verified |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.4.2`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| Physical mode-aware preview and final dispatch timing | USER_REVIEW |

## `morse-v1.4.3` held-dash realtime preview

While a key or detected microphone tone remains active, reaching the decoder's
two-unit dash threshold adds a non-mutating tentative dash to the live display
and updates the active-mode character immediately. Release still records the
symbol; timeout or Button B still commits and sends it. Reaching the 1.2-second
mode gesture clears the temporary preview instead of entering the dash.

| Check | Result |
|---|---|
| Decoder/input tests | PASS: 15/15, including the exact held-dash threshold, active mode, and unchanged decoder state |
| StickS3 build | PASS: 824,265 bytes flash, 34,140 bytes RAM |
| Firmware binary SHA-256 | `874C1F06A596359EEA5281D38C5EC13EE18235BFA0F7B74CC8EDCCB7DB0D2A9E` |
| Physical flash | PASS: COM3, ESP32-S3-PICO-1, MAC `70:04:1D:DA:7F:D0`; all written segments hash-verified |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.4.3`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| Physical held-dash transition and mode-hold cancellation | USER_REVIEW |

## `morse-v1.5.0` standalone Morse Trainer

Holding Button A and Button B together for 1.5 seconds at runtime switches
between the Codex controller and a local A-Z/0-9 standard-Morse trainer. The
gesture is latched until both buttons are released. Trainer answers use the
normal Morse decoder but return before command mapping or BLE dispatch; correct
answers advance after 900 ms and wrong answers retain the same target.

| Check | Result |
|---|---|
| Decoder/trainer/input tests | PASS: 16/16, including target generation and local correct/retry scoring |
| StickS3 build | PASS: 826,429 bytes flash, 34,156 bytes RAM |
| Firmware binary SHA-256 | `3C6C006465EE210486701A8853F753B790032F69566913330126B6990EE4F4ED` |
| Patch whitespace check | PASS: no errors; only existing LF-to-CRLF checkout warnings |
| Physical flash | PASS: COM3, ESP32-S3-PICO-1, MAC `70:04:1D:DA:7F:D0`; all written segments hash-verified |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.5.0`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| Physical A+B isolation, trainer UI, answer flow, and return to Codex mode | USER_REVIEW |

## `morse-v1.5.1` training modes and three-attempt reveal

The trainer now keeps its own decoder-mode selection. A 1.2-second Button A or
Grove-key hold switches SIMPLE/NORMAL and starts a fresh problem without
changing the saved Codex-mode selection. SIMPLE draws digit problems and uses
cut-number answers; NORMAL draws standard A-Z/0-9 problems. Wrong and invalid
commits count as attempts. Attempts one and two retry the same target; attempt
three reveals the mode-correct Morse pattern for 2.2 seconds before advancing.

| Check | Result |
|---|---|
| Decoder/trainer/input tests | PASS: 16/16, including both pools, standard/cut-number encoding, and RETRY/RETRY/REVEAL |
| StickS3 build | PASS: 827,373 bytes flash, 34,172 bytes RAM |
| Firmware binary SHA-256 | `A798717869ACACC6597929EB68842B64C310818F88F75C69D27DD7BDFDDA2357` |
| Physical flash | PASS: COM3, ESP32-S3-PICO-1, MAC `70:04:1D:DA:7F:D0`; all written segments hash-verified |
| Firmware banner | PASS: `version=v1.0 local=morse-v1.5.1`, `mode=KEY decode=SIMPLE`, `grove_key=G10` |
| Physical mode switching, three failed attempts, answer visibility, and Codex-mode restoration | USER_REVIEW |
