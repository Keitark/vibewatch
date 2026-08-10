#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "command_mapper.h"
#include "morse_decoder.h"
#include "sound.h"
#include "tone_detector.h"
#include "vibe_hid.h"

namespace {

constexpr std::uint32_t kBatteryUpdateMs = 30000;
constexpr std::uint32_t kUiUpdateMs = 50;
constexpr std::uint32_t kStatusHoldMs = 1300;
constexpr std::uint32_t kButtonLongPressMs = 800;
constexpr std::uint32_t kButtonDoubleClickMs = 300;
constexpr std::uint32_t kHostMicSafetyMs = 30000;
constexpr std::uint32_t kPairingHoldMs = 3000;
constexpr std::size_t kAudioBlockSamples = 256;
constexpr std::uint32_t kAudioSampleRate = 16000;
constexpr std::uint8_t kKeyToneVolume = 115;

enum class InputMode {
    Key,
    Microphone,
};

morse::Decoder g_decoder;
morse::ToneDetector g_toneDetector;
InputMode g_inputMode = InputMode::Key;

NimBLEServer* g_server = nullptr;
NimBLEHIDDevice* g_hid = nullptr;
NimBLECharacteristic* g_vendorInput = nullptr;
NimBLECharacteristic* g_vendorOutput = nullptr;
QueueHandle_t g_rpcQueue = nullptr;

volatile bool g_connected = false;
volatile bool g_connectPending = false;
volatile bool g_disconnectPending = false;
String g_rxBuffer;
volatile bool g_uiDirty = true;
bool g_hostMicOn = false;
bool g_micTonePresent = false;
std::uint32_t g_hostMicReleaseAt = 0;
std::uint32_t g_lastBatteryUpdate = 0;
std::uint32_t g_lastUiDraw = 0;
std::uint8_t g_batteryLevel = 100;
bool g_isCharging = false;

std::int16_t g_audioSamples[kAudioBlockSamples] = {};
String g_lastPattern;
String g_history;
char g_lastCharacter = '-';
String g_lastLabel = "READY";
String g_transientStatus;
std::uint32_t g_transientStatusUntil = 0;

std::uint32_t g_buttonBPressedAt = 0;
std::uint32_t g_buttonBFirstReleasedAt = 0;
bool g_buttonBLongHandled = false;
bool g_buttonBSecondClick = false;
bool g_buttonBSinglePending = false;

void setTransientStatus(const char* text, std::uint32_t durationMs = kStatusHoldMs) {
    g_transientStatus = text;
    g_transientStatusUntil = millis() + durationMs;
    g_uiDirty = true;
}

void updateBattery(bool notify) {
    const int level = M5.Power.getBatteryLevel();
    if (level >= 0 && level <= 100) {
        g_batteryLevel = static_cast<std::uint8_t>(level);
    }
    g_isCharging = M5.Power.isCharging() == m5::Power_Class::is_charging;
    if (g_hid != nullptr) {
        g_hid->setBatteryLevel(g_batteryLevel, notify && g_connected);
    }
    g_lastBatteryUpdate = millis();
    g_uiDirty = true;
}

void appendHistory(char character) {
    if (character == ' ' && (g_history.isEmpty() || g_history.endsWith(" "))) {
        return;
    }
    g_history += character;
    while (g_history.length() > 14) {
        g_history.remove(0, 1);
    }
}

void sendFramedJson(String payload, bool appendCrlf) {
    if (!g_connected || g_vendorInput == nullptr) {
        return;
    }
    if (appendCrlf && !payload.endsWith("\r\n")) {
        payload += "\r\n";
    }
    std::size_t offset = 0;
    while (offset < payload.length()) {
        const std::size_t chunk =
            std::min(vibe::kRpcChunkLength, payload.length() - offset);
        std::uint8_t report[vibe::kBleReportLength] = {};
        report[0] = vibe::kChannelJsonRpc;
        report[1] = static_cast<std::uint8_t>(chunk);
        std::memcpy(&report[2], payload.c_str() + offset, chunk);
        g_vendorInput->setValue(report, sizeof(report));
        if (!g_vendorInput->notify()) {
            Serial.println("BLE notify failed");
            return;
        }
        offset += chunk;
        if (offset < payload.length()) {
            delay(8);
        }
    }
}

bool sendKeyEvent(const char* key, bool pressed) {
    if (!g_connected || g_vendorInput == nullptr) {
        return false;
    }
    std::uint8_t report[vibe::kBleReportLength] = {};
    report[0] = vibe::kChannelJsonRpc;
    const int written = std::snprintf(
        reinterpret_cast<char*>(&report[2]), vibe::kRpcChunkLength,
        "{\"m\":\"v.oai.hid\",\"p\":{\"k\":\"%s\",\"act\":%u}}\r\n",
        key, pressed ? 1U : 0U);
    if (written < 0 || written >= static_cast<int>(vibe::kRpcChunkLength)) {
        Serial.println("HID event payload overflow");
        return false;
    }
    report[1] = static_cast<std::uint8_t>(written);
    g_vendorInput->setValue(report, sizeof(report));
    const bool sent = g_vendorInput->notify();
    Serial.printf("HID %s %s %s\n", key, pressed ? "DOWN" : "UP",
                  sent ? "sent" : "failed");
    return sent;
}

bool pulseKeyEvent(const char* key) {
    if (!sendKeyEvent(key, true)) {
        return false;
    }
    delay(12);
    return sendKeyEvent(key, false);
}

bool sendHostMicState(bool pressed) {
    if (!sendKeyEvent("ACT10", pressed)) {
        return false;
    }
    delay(12);
    return sendKeyEvent("ACT11", pressed);
}

void releaseHostMic(const char* status) {
    if (!g_hostMicOn) {
        return;
    }
    if (g_connected) {
        sendHostMicState(false);
    }
    g_hostMicOn = false;
    g_hostMicReleaseAt = 0;
    setTransientStatus(status);
}

void dispatchCommand(const morse::CommandMapping& mapping) {
    g_lastLabel = mapping.label;
    if (!g_connected) {
        setTransientStatus("NO BLE - NOT SENT", 1800);
        return;
    }

    bool sent = false;
    if (mapping.type == morse::CommandType::Action ||
        mapping.type == morse::CommandType::Agent) {
        sent = pulseKeyEvent(mapping.eventKey);
    } else if (mapping.type == morse::CommandType::MicrophoneToggle) {
        const bool next = !g_hostMicOn;
        sent = sendHostMicState(next);
        if (sent) {
            g_hostMicOn = next;
            g_hostMicReleaseAt = next ? millis() + kHostMicSafetyMs : 0;
            g_lastLabel = next ? "HOST MIC ON" : "HOST MIC OFF";
        }
    }

    if (sent) {
        setTransientStatus(g_lastLabel.c_str());
        if (g_inputMode == InputMode::Key) {
            sound::playAck(mapping.type == morse::CommandType::MicrophoneToggle
                               ? 1180.0f
                               : 1040.0f,
                           45);
        }
    } else {
        setTransientStatus("SEND FAILED", 1800);
    }
}

void handleDecodeEvent(const morse::DecodeEvent& event) {
    if (event.type == morse::EventType::None) {
        return;
    }
    if (event.type == morse::EventType::WordGap) {
        appendHistory(' ');
        g_uiDirty = true;
        return;
    }

    g_lastPattern = event.pattern.c_str();
    if (event.type == morse::EventType::Invalid) {
        g_lastCharacter = '?';
        g_lastLabel = "INVALID";
        setTransientStatus("INVALID MORSE");
        return;
    }

    char resolvedCharacter = event.character;
    if (event.forced) {
        const char cutNumber = morse::Decoder::decodeCutNumber(event.pattern);
        if (cutNumber != '\0') {
            resolvedCharacter = cutNumber;
        }
    }
    g_lastCharacter = resolvedCharacter;
    appendHistory(resolvedCharacter);
    const auto mapping = morse::mapCommand(resolvedCharacter);
    if (mapping.type == morse::CommandType::None) {
        g_lastLabel = "UNMAPPED";
        setTransientStatus("UNMAPPED - NOT SENT");
        return;
    }
    dispatchCommand(mapping);
}

void sendRpcResponse(const char* method, int id) {
    JsonDocument response;
    response["id"] = id;
    response["method"] = method;
    if (std::strcmp(method, "device.status") == 0) {
        updateBattery(false);
        JsonObject result = response["result"].to<JsonObject>();
        result["version"] = vibe::kFirmwareVersion;
        result["profile_index"] = 0;
        result["layer_index"] = 1;
        result["battery"] = g_batteryLevel;
        result["is_charging"] = g_isCharging;
    } else if (std::strcmp(method, "sys.version") == 0) {
        response["result"]["version"] = vibe::kFirmwareVersion;
    } else {
        response["result"]["ok"] = 1;
    }
    String json;
    serializeJson(response, json);
    sendFramedJson(json, true);
}

void processRpc(const char* json) {
    JsonDocument request;
    const auto error = deserializeJson(request, json);
    if (error) {
        Serial.printf("RPC parse failed: %s\n", error.c_str());
        return;
    }
    const char* method = request["method"] | request["m"] | "";
    const int id = request["id"] | request["i"] | -1;
    if (id >= 0 && method[0] != '\0') {
        sendRpcResponse(method, id);
    }
}

class RpcOutputCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* characteristic, NimBLEConnInfo&) override {
        const NimBLEAttValue value = characteristic->getValue();
        const auto* data = value.data();
        const std::size_t length = value.size();
        if (data == nullptr || length < 2 || data[0] != vibe::kChannelJsonRpc) {
            return;
        }
        const std::size_t chunkLength = data[1];
        if (chunkLength > vibe::kRpcChunkLength || chunkLength > length - 2 ||
            g_rxBuffer.length() + chunkLength > vibe::kRpcBufferLength) {
            g_rxBuffer = "";
            return;
        }
        for (std::size_t i = 0; i < chunkLength; ++i) {
            g_rxBuffer += static_cast<char>(data[i + 2]);
        }

        JsonDocument probe;
        const auto result = deserializeJson(probe, g_rxBuffer);
        if (result == DeserializationError::IncompleteInput) {
            return;
        }
        if (result) {
            g_rxBuffer = "";
            return;
        }
        auto* message = static_cast<char*>(std::malloc(g_rxBuffer.length() + 1));
        if (message == nullptr) {
            g_rxBuffer = "";
            return;
        }
        std::memcpy(message, g_rxBuffer.c_str(), g_rxBuffer.length());
        message[g_rxBuffer.length()] = '\0';
        if (xQueueSend(g_rpcQueue, &message, 0) != pdTRUE) {
            std::free(message);
        }
        g_rxBuffer = "";
    }
};

class HidServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* server, NimBLEConnInfo& connection) override {
        g_connected = true;
        g_connectPending = true;
        g_uiDirty = true;
        server->updateConnParams(connection.getConnHandle(), 12, 24, 0, 180);
    }

    void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int) override {
        g_connected = false;
        g_disconnectPending = true;
        g_uiDirty = true;
        NimBLEDevice::startAdvertising();
    }
};

RpcOutputCallbacks g_rpcCallbacks;
HidServerCallbacks g_serverCallbacks;

void addDeviceInfoCharacteristic(std::uint16_t uuid, const char* value) {
    auto* characteristic =
        g_hid->getDeviceInfoService()->createCharacteristic(uuid, NIMBLE_PROPERTY::READ);
    characteristic->setValue(value);
}

void initializeBle(bool clearBonds) {
    NimBLEDevice::init(vibe::kDeviceName);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityAuth(true, false, true);

    g_server = NimBLEDevice::createServer();
    g_server->setCallbacks(&g_serverCallbacks);
    g_hid = new NimBLEHIDDevice(g_server);
    g_hid->setManufacturer(vibe::kManufacturer);
    g_hid->setPnp(0x01, vibe::kVendorId, vibe::kProductId, vibe::kProductVersion);
    g_hid->setHidInfo(0x00, 0x01);
    g_hid->setReportMap(vibe::kReportMap, sizeof(vibe::kReportMap));

    char serial[17];
    std::snprintf(serial, sizeof(serial), "%016llX", ESP.getEfuseMac());
    addDeviceInfoCharacteristic(0x2A24, vibe::kModelNumber);
    addDeviceInfoCharacteristic(0x2A25, serial);
    addDeviceInfoCharacteristic(0x2A26, vibe::kFirmwareVersion);

    auto* keyboardInput = g_hid->getInputReport(1);
    auto* consumerInput = g_hid->getInputReport(2);
    auto* pointerInput = g_hid->getInputReport(3);
    g_vendorInput = g_hid->getInputReport(vibe::kVendorReportId);
    g_vendorOutput = g_hid->getOutputReport(vibe::kVendorReportId);
    g_hid->getFeatureReport(vibe::kVendorReportId);

    const std::uint8_t keyboardIdle[8] = {};
    const std::uint8_t consumerIdle[2] = {};
    const std::uint8_t pointerIdle[5] = {};
    const std::uint8_t vendorIdle[vibe::kBleReportLength] = {};
    keyboardInput->setValue(keyboardIdle, sizeof(keyboardIdle));
    consumerInput->setValue(consumerIdle, sizeof(consumerIdle));
    pointerInput->setValue(pointerIdle, sizeof(pointerIdle));
    g_vendorInput->setValue(vendorIdle, sizeof(vendorIdle));
    g_vendorOutput->setCallbacks(&g_rpcCallbacks);

    if (clearBonds) {
        NimBLEDevice::deleteAllBonds();
    }
    updateBattery(false);
    if (!g_server->start()) {
        setTransientStatus("BLE START FAILED", 5000);
        return;
    }
    auto* advertising = NimBLEDevice::getAdvertising();
    advertising->setName(vibe::kDeviceName);
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(g_hid->getHidService()->getUUID());
    advertising->enableScanResponse(true);
    advertising->start();
    Serial.printf("MORSE_VIBE_BLE name=%s version=%s\n",
                  vibe::kDeviceName, vibe::kFirmwareVersion);
}

String defaultStatus() {
    if (!g_connected) {
        return "BLE ADVERTISING";
    }
    if (g_inputMode == InputMode::Microphone) {
        if (!g_toneDetector.calibrated()) {
            return "CALIBRATING";
        }
        return g_micTonePresent ? "TONE" : "LISTENING";
    }
    return g_decoder.isKeyDown() ? "KEYING" : "READY";
}

void renderUi(std::uint32_t now) {
    const std::uint16_t cyan = M5.Display.color565(40, 210, 235);
    const std::uint16_t amber = M5.Display.color565(255, 174, 55);
    const std::uint16_t green = M5.Display.color565(62, 220, 140);
    const std::uint16_t muted = M5.Display.color565(125, 140, 155);
    const std::uint16_t panel = M5.Display.color565(17, 24, 31);

    M5.Display.startWrite();
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.fillRect(0, 0, 240, 20, panel);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextSize(1);
    M5.Display.setTextDatum(middle_left);
    M5.Display.setTextColor(cyan, panel);
    M5.Display.drawString("MORSE VIBE", 5, 10);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextColor(g_inputMode == InputMode::Key ? amber : green, panel);
    M5.Display.drawString(g_inputMode == InputMode::Key ? "KEY" : "MIC", 139, 10);
    M5.Display.fillCircle(172, 10, 4, g_connected ? green : amber);
    char battery[12];
    std::snprintf(battery, sizeof(battery), "%s%u%%", g_isCharging ? "+" : "",
                  g_batteryLevel);
    M5.Display.setTextDatum(middle_right);
    M5.Display.setTextColor(TFT_WHITE, panel);
    M5.Display.drawString(battery, 236, 10);

    String shownPattern = g_decoder.pattern().empty()
                              ? g_lastPattern
                              : String(g_decoder.pattern().c_str());
    if (shownPattern.isEmpty()) {
        shownPattern = "-";
    }
    M5.Display.setTextDatum(middle_center);
    M5.Display.setFont(&fonts::FreeMonoBold12pt7b);
    M5.Display.setTextColor(amber, TFT_BLACK);
    M5.Display.drawString(shownPattern, 120, 37);

    char decoded[2] = {g_lastCharacter, '\0'};
    M5.Display.setFont(&fonts::FreeSansBold24pt7b);
    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(decoded, 62, 75);
    M5.Display.setFont(&fonts::Font2);
    M5.Display.setTextColor(g_hostMicOn ? amber : cyan, TFT_BLACK);
    M5.Display.drawString(g_lastLabel, 164, 70);

    String status = (g_transientStatusUntil != 0 &&
                     static_cast<std::int32_t>(g_transientStatusUntil - now) > 0)
                        ? g_transientStatus
                        : defaultStatus();
    M5.Display.setTextColor(muted, TFT_BLACK);
    M5.Display.drawString(status, 120, 96);

    if (g_inputMode == InputMode::Microphone) {
        const float denominator = std::max(1.0f, g_toneDetector.threshold());
        const float ratio = std::clamp(g_toneDetector.level() / denominator, 0.0f, 1.0f);
        M5.Display.fillRoundRect(15, 106, 210, 5, 2, panel);
        M5.Display.fillRoundRect(15, 106, static_cast<int>(210.0f * ratio), 5, 2,
                                 g_micTonePresent ? green : cyan);
    }

    M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Display.drawString(g_history.isEmpty() ? "B:COMMIT  HOLD:MODE" : g_history,
                          120, 124);
    M5.Display.endWrite();
    g_lastUiDraw = now;
    g_uiDirty = false;
}

void setInputMode(InputMode mode) {
    if (mode == g_inputMode) {
        return;
    }
    releaseHostMic("HOST MIC RELEASED");
    g_decoder.clear();
    g_lastPattern = "";
    g_micTonePresent = false;

    if (mode == InputMode::Microphone) {
        sound::stopKeyTone();
        sound::playAck(1320.0f, 55);
        delay(70);
        M5.Speaker.end();
        delay(10);
        M5.Mic.begin();
        g_toneDetector.reset();
        setTransientStatus("MIC CALIBRATING", 1100);
    } else {
        while (M5.Mic.isRecording()) {
            delay(1);
        }
        M5.Mic.end();
        delay(10);
        M5.Speaker.begin();
        sound::playAck(760.0f, 55);
        setTransientStatus("KEY MODE");
    }
    g_inputMode = mode;
    g_uiDirty = true;
}

void handleKeyInput() {
    if (g_inputMode == InputMode::Key) {
        if (M5.BtnA.wasPressed()) {
            g_decoder.keyDown(millis());
            sound::startKeyTone(kKeyToneVolume);
            g_uiDirty = true;
        }
        if (M5.BtnA.wasReleased()) {
            sound::stopKeyTone();
            handleDecodeEvent(g_decoder.keyUp(millis()));
            g_uiDirty = true;
        }
        return;
    }

    if (M5.BtnA.wasPressed()) {
        g_toneDetector.reset();
        g_decoder.clear();
        g_micTonePresent = false;
        setTransientStatus("MIC RECALIBRATE", 1000);
    }
}

void handleControlButton(std::uint32_t now) {
    if (M5.BtnB.wasPressed()) {
        g_buttonBPressedAt = now;
        g_buttonBLongHandled = false;
        if (g_buttonBSinglePending &&
            now - g_buttonBFirstReleasedAt <= kButtonDoubleClickMs) {
            g_buttonBSinglePending = false;
            g_buttonBSecondClick = true;
            g_decoder.clear();
            g_lastPattern = "";
            setTransientStatus("CLEARED");
        } else {
            g_buttonBSecondClick = false;
        }
    }

    if (M5.BtnB.isPressed() && !g_buttonBLongHandled && !g_buttonBSecondClick &&
        now - g_buttonBPressedAt >= kButtonLongPressMs) {
        g_buttonBLongHandled = true;
        g_buttonBSinglePending = false;
        setInputMode(g_inputMode == InputMode::Key ? InputMode::Microphone
                                                   : InputMode::Key);
    }

    if (M5.BtnB.wasReleased()) {
        if (g_buttonBSecondClick) {
            g_buttonBSecondClick = false;
        } else if (!g_buttonBLongHandled) {
            g_buttonBSinglePending = true;
            g_buttonBFirstReleasedAt = now;
        }
    }

    if (g_buttonBSinglePending &&
        now - g_buttonBFirstReleasedAt > kButtonDoubleClickMs) {
        g_buttonBSinglePending = false;
        handleDecodeEvent(g_decoder.forceCommit(now));
    }
}

void processMicrophone() {
    if (g_inputMode != InputMode::Microphone || !M5.Mic.isEnabled()) {
        return;
    }
    if (!M5.Mic.record(g_audioSamples, kAudioBlockSamples, kAudioSampleRate)) {
        return;
    }
    const bool wasTone = g_micTonePresent;
    g_micTonePresent = g_toneDetector.process(g_audioSamples, kAudioBlockSamples);
    const std::uint32_t now = millis();
    if (!wasTone && g_micTonePresent) {
        g_decoder.keyDown(now);
    } else if (wasTone && !g_micTonePresent) {
        handleDecodeEvent(g_decoder.keyUp(now));
    }
    g_uiDirty = true;
}

bool detectPairingReset() {
    M5.update();
    if (!M5.BtnA.isPressed() || !M5.BtnB.isPressed()) {
        return false;
    }
    const std::uint32_t startedAt = millis();
    while (M5.BtnA.isPressed() && M5.BtnB.isPressed()) {
        M5.update();
        const std::uint32_t held = millis() - startedAt;
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.setTextDatum(middle_center);
        M5.Display.setFont(&fonts::Font2);
        M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
        M5.Display.drawString("HOLD TO CLEAR BLE", 120, 55);
        M5.Display.progressBar(20, 78, 200, 12,
                               std::min<std::uint32_t>(100, held * 100 / kPairingHoldMs));
        if (held >= kPairingHoldMs) {
            M5.Display.drawString("PAIRING RESET", 120, 105);
            delay(350);
            return true;
        }
        delay(20);
    }
    return false;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);

    auto config = M5.config();
    config.clear_display = true;
    config.internal_spk = true;
    config.internal_mic = true;
    M5.begin(config);
    M5.Display.setRotation(1);
    M5.Display.setBrightness(82);
    M5.Mic.end();
    M5.Speaker.begin();
    M5.Speaker.setVolume(kKeyToneVolume);

    const bool clearBonds = detectPairingReset();
    updateBattery(false);
    g_rpcQueue = xQueueCreate(6, sizeof(char*));
    if (g_rpcQueue == nullptr) {
        M5.Display.fillScreen(TFT_BLACK);
        M5.Display.drawString("RPC QUEUE FAILED", 120, 67);
        while (true) {
            delay(1000);
        }
    }
    initializeBle(clearBonds);
    renderUi(millis());
    Serial.printf("MORSE_VIBE_READY board=M5StickS3 mode=KEY bonds_cleared=%u\n",
                  clearBonds ? 1U : 0U);
}

void loop() {
    M5.update();
    const std::uint32_t now = millis();
    handleKeyInput();
    handleControlButton(now);
    processMicrophone();
    // The B gesture decides whether a pending pattern means an explicit cut
    // number, a clear, or a mode change. Pause the automatic letter gap until
    // that gesture is resolved so .- + B cannot race and send A before 1.
    const bool controlGesturePending = M5.BtnB.isPressed() ||
                                       g_buttonBSinglePending ||
                                       g_buttonBSecondClick;
    if (!controlGesturePending) {
        handleDecodeEvent(g_decoder.update(now));
    }

    if (g_connectPending) {
        g_connectPending = false;
        setTransientStatus("BLE CONNECTED");
    }
    if (g_disconnectPending) {
        g_disconnectPending = false;
        g_hostMicOn = false;
        g_hostMicReleaseAt = 0;
        setTransientStatus("BLE DISCONNECTED");
    }

    char* message = nullptr;
    while (xQueueReceive(g_rpcQueue, &message, 0) == pdTRUE) {
        processRpc(message);
        std::free(message);
        message = nullptr;
    }

    if (g_hostMicOn &&
        static_cast<std::int32_t>(now - g_hostMicReleaseAt) >= 0) {
        releaseHostMic("MIC SAFETY RELEASE");
    }
    if (now - g_lastBatteryUpdate >= kBatteryUpdateMs) {
        updateBattery(true);
    }
    if (g_uiDirty && now - g_lastUiDraw >= kUiUpdateMs) {
        renderUi(now);
    }
    delay(2);
}
