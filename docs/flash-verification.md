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

## Remaining USER_REVIEW gates

- Visual inspection of all display states
- Subjective crispness and measured latency of the ton/tsu sidetone
- Button-key timing across operator speeds
- Controlled microphone-tone and ambient-noise trials
- Native Codex Micro detection and every command on macOS and Windows
- Current draw and battery runtime by mode
