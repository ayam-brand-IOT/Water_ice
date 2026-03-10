#pragma once
/**
 * BLEQRClient
 * -----------
 * BLE Central (client) that connects to the "QR-Reader" peripheral.
 * When in WAITING_TOTE_ID state the main loop calls requestQR(); the
 * peripheral replies via a BLE notification and the registered callback
 * is invoked with the raw QR string.
 *
 * UUIDs must match those in the QR-Reader firmware (main.cpp of the
 * QR-Reader-BLE project).
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEClient.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLERemoteCharacteristic.h>
#include <functional>

// ── UUIDs (must match QR-Reader firmware) ───────────────────────────────────
#define BLEQR_DEVICE_NAME     "QR-Reader"
#define BLEQR_SERVICE_UUID    "12345678-1234-1234-1234-123456789abc"
#define BLEQR_DATA_CHAR_UUID  "12345678-1234-1234-1234-123456789abd"  // READ+NOTIFY
#define BLEQR_REQ_CHAR_UUID   "12345678-1234-1234-1234-123456789abe"  // WRITE

enum class BLEQRState {
  IDLE,        // not yet initialized
  SCANNING,    // actively scanning for the QR-Reader peripheral
  CONNECTING,  // found device, attempting connection
  CONNECTED,   // connected and subscribed
  LOST         // connection lost, will re-scan
};

class BLEQRClient {
public:
  using QRCallback = std::function<void(const String&)>;

  /**
   * Call once in setup(). Safe to call even if BLEDevice::init() was already
   * invoked elsewhere (it will skip re-initialization).
   */
  void begin(QRCallback cb);

  /**
   * Call every loop iteration. Drives the state machine (scan → connect →
   * subscribe). Non-blocking.
   */
  void loop();

  /** Returns true when the peripheral is connected and ready. */
  bool isConnected() const { return _state == BLEQRState::CONNECTED; }

  BLEQRState getState() const { return _state; }

  /**
   * Sends a "GET" write to the request characteristic.
   * The peripheral will answer with a BLE notification containing the
   * buffered QR string (or "NO_QR" if nothing has been scanned yet).
   * Only has effect when isConnected() == true.
   */
  void requestQR();

  // ── Internal use by static callbacks ────────────────────────────────────
  void _onFound(BLEAdvertisedDevice* device);
  void _onNotify(uint8_t* data, size_t length);
  void _onDisconnect();

private:
  bool _connectToServer();
  void _startScan();

  BLEClient*                _pClient        = nullptr;
  BLERemoteCharacteristic*  _pDataChar      = nullptr;
  BLERemoteCharacteristic*  _pReqChar       = nullptr;

  BLEQRState  _state       = BLEQRState::IDLE;
  QRCallback  _callback;

  String      _serverAddress;           // found during scan
  bool        _foundDevice   = false;   // scan found the target
  uint32_t    _lastScanMs    = 0;
  static const uint32_t SCAN_INTERVAL_MS = 8000;  // re-scan every 8 s after lost
};
