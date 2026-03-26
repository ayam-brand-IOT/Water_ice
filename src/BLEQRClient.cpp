#include "BLEQRClient.h"

// ── Static singleton pointer for callbacks ───────────────────────────────────
static BLEQRClient* s_instance = nullptr;

// ── Advertised-device callback (fired during scan) ───────────────────────────
class QRAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (!s_instance) return;
    if (advertisedDevice.getName() == BLEQR_DEVICE_NAME) {
      Serial.printf("[BLE-QR] Peripheral found: %s\n",
                    advertisedDevice.getAddress().toString().c_str());
      // Stop the scan and signal the state machine to connect
      BLEDevice::getScan()->stop();
      s_instance->_onFound(new BLEAdvertisedDevice(advertisedDevice));
    }
  }
};

// ── Notification callback ────────────────────────────────────────────────────
static void notifyCallback(BLERemoteCharacteristic*, uint8_t* pData,
                            size_t length, bool /*isNotify*/) {
  if (s_instance) s_instance->_onNotify(pData, length);
}

// ── Client connection callbacks ───────────────────────────────────────────────
class QRClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient*)    override {}
  void onDisconnect(BLEClient*) override {
    if (s_instance) s_instance->_onDisconnect();
  }
};

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void BLEQRClient::begin(QRCallback cb) {
  s_instance = this;
  _callback  = cb;

  // BLEDevice::init() is idempotent – safe to call even if already initialised
  BLEDevice::init("");

  _pClient = BLEDevice::createClient();
  _pClient->setClientCallbacks(new QRClientCallbacks());

  Serial.println("[BLE-QR] Initialized – will scan for \"" BLEQR_DEVICE_NAME "\"");
  _startScan();
}

void BLEQRClient::loop() {
  switch (_state) {
    case BLEQRState::IDLE:
      break;

    case BLEQRState::SCANNING:
      // Scanning is driven by the BLE stack callbacks; nothing to poll here.
      break;

    case BLEQRState::CONNECTING:
      if (_foundDevice) {
        _foundDevice = false;
        if (_connectToServer()) {
          _state = BLEQRState::CONNECTED;
          Serial.println("[BLE-QR] Connected and subscribed");
        } else {
          Serial.println("[BLE-QR] Connection failed, re-scanning...");
          _state = BLEQRState::SCANNING;
          _startScan();
        }
      }
      break;

    case BLEQRState::CONNECTED:
      // Enviar ACK pendiente desde loop(), nunca desde el callback BLE
      // (escribir desde un callback del stack BLE puede causar deadlock)
      if (_pendingAck && _pReqChar) {
        _pReqChar->writeValue("ACK", false);
        _pendingAck = false;
        Serial.println("[BLE-QR] ACK enviado al periférico");
      }
      break;

    case BLEQRState::LOST:
      // Wait SCAN_INTERVAL_MS before trying again to avoid hammering BLE stack
      if (millis() - _lastScanMs > SCAN_INTERVAL_MS) {
        Serial.println("[BLE-QR] Re-scanning for peripheral...");
        _startScan();
      }
      break;
  }
}

void BLEQRClient::requestQR() {
  if (_state != BLEQRState::CONNECTED || !_pReqChar) {
    Serial.println("[BLE-QR] requestQR(): not connected");
    return;
  }
  Serial.println("[BLE-QR] Sending QR request...");
  _pReqChar->writeValue("GET", false);  // WRITE_NR (no response)
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal callbacks (called from BLE stack context)
// ─────────────────────────────────────────────────────────────────────────────

void BLEQRClient::_onFound(BLEAdvertisedDevice* device) {
  _serverAddress = device->getAddress().toString().c_str();
  delete device;
  _foundDevice = true;
  _state       = BLEQRState::CONNECTING;
}

void BLEQRClient::_onNotify(uint8_t* pData, size_t length) {
  String value((char*)pData, length);
  Serial.printf("[BLE-QR] Notification received: %s\n", value.c_str());
  if (_callback && _callback(value)) {
    _pendingAck = true;  // callback procesó exitosamente → ACK en loop()
  }
}

void BLEQRClient::_onDisconnect() {
  Serial.println("[BLE-QR] Peripheral disconnected");
  _pDataChar  = nullptr;
  _pReqChar   = nullptr;
  _pendingAck = false;  // descartar ACK pendiente si se cayó la conexión
  _state      = BLEQRState::LOST;
  _lastScanMs = millis();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void BLEQRClient::_startScan() {
  _state = BLEQRState::SCANNING;

  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new QRAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->setInterval(100);
  pScan->setWindow(99);
  // Start indefinite scan (duration=0) – we stop it manually in the callback
  pScan->start(0, nullptr, false);
  Serial.println("[BLE-QR] Scan started...");
}

bool BLEQRClient::_connectToServer() {
  BLEAddress address(_serverAddress.c_str());

  if (!_pClient->connect(address)) {
    Serial.println("[BLE-QR] connect() failed");
    return false;
  }

  BLERemoteService* pService = _pClient->getService(BLEQR_SERVICE_UUID);
  if (!pService) {
    Serial.println("[BLE-QR] Service not found");
    _pClient->disconnect();
    return false;
  }

  _pDataChar = pService->getCharacteristic(BLEQR_DATA_CHAR_UUID);
  if (!_pDataChar) {
    Serial.println("[BLE-QR] Data characteristic not found");
    _pClient->disconnect();
    return false;
  }

  _pReqChar = pService->getCharacteristic(BLEQR_REQ_CHAR_UUID);
  if (!_pReqChar) {
    Serial.println("[BLE-QR] Request characteristic not found");
    _pClient->disconnect();
    return false;
  }

  if (_pDataChar->canNotify()) {
    _pDataChar->registerForNotify(notifyCallback);
    Serial.println("[BLE-QR] Subscribed to QR notifications");
  } else {
    Serial.println("[BLE-QR] Data char cannot notify – proceeding with READ only");
  }

  return true;
}
