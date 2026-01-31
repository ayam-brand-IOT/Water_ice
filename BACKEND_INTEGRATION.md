# Tote Inbound - Backend Integration Update

## Changes Made

The **tote_inbound** system has been updated to have the same structure as **tote_outbound** and integrate with the backend.

### 1. Unified Structure

#### New Flow with `ToteState`
Now both systems (inbound and outbound) use the same state machine:

```
IDLE → DISPENSING_ICE → DISPENSING_WATER → WAITING_TOTE_ID → COMPLETED
```

#### Elimination of `ControllerState`
- Removed switch based on `ControllerState` (IDLE, WATER_FILLING, ICE_FILLING, TOTE_READY)
- Replaced with `handleToteState()` using `ToteState`

### 2. Dispensing Order

**Before**: Water → Ice
**Now**: Ice → Water

#### Changes in Stages:
- **Stage 1** now dispenses **ice** (was water before)
- **Stage 2** now fills **water** (was ice before)

### 3. Backend Integration

#### New Function: `createToteInBackend()`

```cpp
bool createToteInBackend(
  const char* toteId, 
  uint32_t tote_kg, 
  uint32_t water_kg, 
  uint32_t ice_kg, 
  uint32_t raw_kg, 
  uint32_t water_out_kg
)
```

**Endpoint**: `POST /api/totes`

**Payload**:
```json
{
  "tote_id": "TOTE001",
  "tote_kg": 100,
  "water_kg": 50,
  "ice_kg": 30,
  "raw_kg": 150,
  "water_out_kg": 0
}
```

**When executed**: At process completion (in `destroyStage3`)

### 4. New Implemented Functions

#### `onIDLE()`
- Waiting state
- Does nothing, waits for START button

#### `onWaitingToteID()`
- Shows prompt every 3 seconds
- Waits for operator to enter tote ID via UI
- Similar to outbound but without backend validation

#### `onCanceled()`
- Stops all pumps
- Clears data
- Resets stages
- Returns to IDLE

### 5. Complete Inbound Flow

```
1. IDLE
   └─> Operator presses START
   
2. DISPENSING_ICE
   └─> Dispenses ice for 4 seconds
   └─> Saves ice_out_kg
   
3. DISPENSING_WATER
   └─> Fills water for 4 seconds
   └─> Saves water_out_kg
   
4. WAITING_TOTE_ID
   └─> Waits for operator ID via UI
   
5. COMPLETED
   └─> Shows summary
   └─> Sends POST to backend
   └─> Returns to IDLE
```

### 6. Configuration

#### Backend URL
In `include/config.h`:
```cpp
#define BACKEND_HOST "192.168.100.10"
#define BACKEND_PORT 3000
#define BACKEND_URL "http://" BACKEND_HOST ":3000"
```

### 7. Inbound vs Outbound Comparison

| Feature | Inbound | Outbound |
|---------|---------|----------|
| HTTP Operation | POST (create) | PUT (update) |
| Automatic detection | No | Yes (weight) |
| Start | START Button | Automatic |
| ID Validation | No | Yes (GET) |
| fish_kg | Doesn't capture | Yes captures |
| Fields sent | tote_kg, water_kg, ice_kg, raw_kg | fish_kg, ice_out_kg, water_out_kg, temp_out |

### 8. Captured Data

#### Inbound Captures:
- `tote_id`: Tote ID (entered by operator)
- `tote_weight`: Initial tote weight
- `ice_out_kg`: Dispensed ice weight
- `water_out_kg`: Added water weight
- `raw_kg`: Total weight at completion

**Sends to backend with**: `water_out_kg = 0` (initial)

#### Outbound Captures:
- `fish_kg`: Fish weight detected automatically
- `ice_out_kg`: Dispensed ice weight
- `water_out_kg`: Added water weight
- `temp_out`: Output temperature

**Sends to backend**: updates the same tote created by inbound

### 9. Log Example

```
=== System Started ===
Initial weight: 100 kg
Transitioning to DISPENSING_ICE

=== Stage 1: Dispensing Ice ===
....
Ice dispensed: 30 kg
Ice dispensing completed
Transitioning to DISPENSING_WATER

=== Stage 2: Filling Water ===
....
Water filled: 50 kg
Water filling completed
Transitioning to WAITING_TOTE_ID

╔════════════════════════════════════╗
║   WAITING FOR TOTE ID FROM UI      ║
╠════════════════════════════════════╣
║ Tote:  100 kg
║ Ice:   30 kg
║ Water: 50 kg
╠════════════════════════════════════╣
║ Enter Tote ID via web interface   ║
╚════════════════════════════════════╝

Tote ID set to: TOTE001

=== Tote Summary ===
ID: TOTE001
Tote: 100 kg
Ice: 30 kg
Water: 50 kg
Raw: 180 kg
==================

=== Creating Tote in Backend ===
POST: http://192.168.100.10:3000/api/totes
Payload: {"tote_id":"TOTE001","tote_kg":100,"water_kg":50,"ice_kg":30,"raw_kg":180,"water_out_kg":0}
HTTP Response code: 201
✓ Tote created in backend successfully!

=== Ready for next tote ===
```

### 10. Complete System Flow

```
┌──────────────────────────────────────────────────────────┐
│                    COMPLETE PROCESS                       │
└──────────────────────────────────────────────────────────┘

1. INBOUND (Entry):
   ├─ Operator places empty tote
   ├─ Presses START
   ├─ System dispenses ICE → 30 kg
   ├─ System fills WATER → 50 kg
   ├─ Operator enters ID: "TOTE001"
   ├─ POST /api/totes (creates record)
   └─ Tote ready for output

2. BACKEND (Storage):
   ├─ Tote ID: TOTE001
   ├─ tote_kg: 100
   ├─ water_kg: 50
   ├─ ice_kg: 30
   ├─ fish_kg: null
   ├─ raw_kg: 180
   ├─ ice_out_kg: null
   ├─ water_out_kg: 0
   └─ temp_out: null

3. OUTBOUND (Output):
   ├─ Tote with fish arrives at scale
   ├─ System detects weight (200 kg fish)
   ├─ Dispenses ICE → 20 kg
   ├─ Fills WATER → 40 kg
   ├─ Operator enters ID: "TOTE001"
   ├─ GET /api/totes/TOTE001 (validates existence)
   ├─ PUT /api/totes/TOTE001 (updates)
   └─ Tote complete

4. BACKEND (Updated):
   ├─ Tote ID: TOTE001
   ├─ tote_kg: 100
   ├─ water_kg: 50
   ├─ ice_kg: 30
   ├─ fish_kg: 200        ← updated
   ├─ raw_kg: 180
   ├─ ice_out_kg: 20      ← updated
   ├─ water_out_kg: 40    ← updated
   └─ temp_out: 0.0       ← updated
```

### 11. Modified Files

- `include/config.h` - Added backend configuration
- `src/main.h` - Added HTTP includes and declarations
- `src/main.cpp` - Complete restructure:
  - New `handleToteState()`
  - Stage swap (ice/water)
  - New functions: `onIDLE()`, `onWaitingToteID()`, `onCanceled()`
  - New function: `createToteInBackend()`
  - Updated: `onStart()`, `onStop()`, `destroyStage3()`
- `src/hardware/Controller.h` - Removed `WAITING_START` from enum

### 12. Dependencies

Already included in `platformio.ini`:
```ini
lib_deps = 
    bblanchon/ArduinoJson@6.20.0
```

`HTTPClient.h` is part of the ESP32 framework.

### 13. Next Steps

- [ ] Test complete integration inbound → backend → outbound
- [ ] Verify data persists correctly
- [ ] Implement more robust network error handling
- [ ] Add LED status indicators
- [ ] Implement offline queue if backend is unavailable
