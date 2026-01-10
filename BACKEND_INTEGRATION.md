# Tote Inbound - Backend Integration Update

## Cambios Realizados

Se ha actualizado el sistema **tote_inbound** para que tenga la misma estructura que **tote_outbound** y se integre con el backend.

### 1. Estructura Unificada

#### Nuevo Flujo con `ToteState`
Ahora ambos sistemas (inbound y outbound) usan la misma máquina de estados:

```
IDLE → DISPENSING_ICE → DISPENSING_WATER → WAITING_TOTE_ID → COMPLETED
```

#### Eliminación de `ControllerState`
- Se removió el switch basado en `ControllerState` (IDLE, WATER_FILLING, ICE_FILLING, TOTE_READY)
- Se reemplazó con `handleToteState()` usando `ToteState`

### 2. Orden de Dispensado

**Antes**: Agua → Hielo
**Ahora**: Hielo → Agua

#### Cambios en Stages:
- **Stage 1** ahora dispensa **hielo** (antes era agua)
- **Stage 2** ahora llena **agua** (antes era hielo)

### 3. Integración con Backend

#### Nueva Función: `createToteInBackend()`

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

**Cuándo se ejecuta**: Al finalizar el proceso (en `destroyStage3`)

### 4. Nuevas Funciones Implementadas

#### `onIDLE()`
- Estado de espera
- No hace nada, espera el botón START

#### `onWaitingToteID()`
- Muestra prompt cada 3 segundos
- Espera que el operador ingrese el ID del tote vía UI
- Similar a outbound pero sin validación contra backend

#### `onCanceled()`
- Detiene todas las bombas
- Limpia datos
- Resetea stages
- Vuelve a IDLE

### 5. Flujo Completo Inbound

```
1. IDLE
   └─> Operador presiona START
   
2. DISPENSING_ICE
   └─> Dispensa hielo durante 4 segundos
   └─> Guarda ice_out_kg
   
3. DISPENSING_WATER
   └─> Llena agua durante 4 segundos
   └─> Guarda water_out_kg
   
4. WAITING_TOTE_ID
   └─> Espera ID del operador vía UI
   
5. COMPLETED
   └─> Muestra resumen
   └─> Envía POST al backend
   └─> Vuelve a IDLE
```

### 6. Configuración

#### Backend URL
En `include/config.h`:
```cpp
#define BACKEND_HOST "192.168.100.10"
#define BACKEND_PORT 3000
#define BACKEND_URL "http://" BACKEND_HOST ":3000"
```

### 7. Comparación Inbound vs Outbound

| Característica | Inbound | Outbound |
|----------------|---------|----------|
| Operación HTTP | POST (crear) | PUT (actualizar) |
| Detección automática | No | Sí (peso) |
| Inicio | Botón START | Automático |
| Validación ID | No | Sí (GET) |
| fish_kg | No captura | Sí captura |
| Campos enviados | tote_kg, water_kg, ice_kg, raw_kg | fish_kg, ice_out_kg, water_out_kg, temp_out |

### 8. Datos Capturados

#### Inbound Captura:
- `tote_id`: ID del tote (ingresado por operador)
- `tote_weight`: Peso inicial del tote
- `ice_out_kg`: Peso del hielo dispensado
- `water_out_kg`: Peso del agua agregada
- `raw_kg`: Peso total al finalizar

**Los envía al backend con**: `water_out_kg = 0` (inicial)

#### Outbound Captura:
- `fish_kg`: Peso del pescado detectado automáticamente
- `ice_out_kg`: Peso del hielo dispensado
- `water_out_kg`: Peso del agua agregada
- `temp_out`: Temperatura de salida

**Los envía al backend**: actualiza el mismo tote creado por inbound

### 9. Ejemplo de Logs

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

### 10. Flujo Completo del Sistema

```
┌──────────────────────────────────────────────────────────┐
│                    PROCESO COMPLETO                       │
└──────────────────────────────────────────────────────────┘

1. INBOUND (Entrada):
   ├─ Operador coloca tote vacío
   ├─ Presiona START
   ├─ Sistema dispensa HIELO → 30 kg
   ├─ Sistema llena AGUA → 50 kg
   ├─ Operador ingresa ID: "TOTE001"
   ├─ POST /api/totes (crea registro)
   └─ Tote listo para salida

2. BACKEND (Almacenamiento):
   ├─ Tote ID: TOTE001
   ├─ tote_kg: 100
   ├─ water_kg: 50
   ├─ ice_kg: 30
   ├─ fish_kg: null
   ├─ raw_kg: 180
   ├─ ice_out_kg: null
   ├─ water_out_kg: 0
   └─ temp_out: null

3. OUTBOUND (Salida):
   ├─ Tote con pescado llega a báscula
   ├─ Sistema detecta peso (200 kg fish)
   ├─ Dispensa HIELO → 20 kg
   ├─ Llena AGUA → 40 kg
   ├─ Operador ingresa ID: "TOTE001"
   ├─ GET /api/totes/TOTE001 (valida existencia)
   ├─ PUT /api/totes/TOTE001 (actualiza)
   └─ Tote completo

4. BACKEND (Actualizado):
   ├─ Tote ID: TOTE001
   ├─ tote_kg: 100
   ├─ water_kg: 50
   ├─ ice_kg: 30
   ├─ fish_kg: 200        ← actualizado
   ├─ raw_kg: 180
   ├─ ice_out_kg: 20      ← actualizado
   ├─ water_out_kg: 40    ← actualizado
   └─ temp_out: 0.0       ← actualizado
```

### 11. Archivos Modificados

- `include/config.h` - Agregado configuración del backend
- `src/main.h` - Agregado includes HTTP y declaraciones
- `src/main.cpp` - Completa reestructuración:
  - Nuevo `handleToteState()`
  - Intercambio de stages (hielo/agua)
  - Nuevas funciones: `onIDLE()`, `onWaitingToteID()`, `onCanceled()`
  - Nueva función: `createToteInBackend()`
  - Actualizado: `onStart()`, `onStop()`, `destroyStage3()`
- `src/hardware/Controller.h` - Removido `WAITING_START` del enum

### 12. Dependencias

Ya incluidas en `platformio.ini`:
```ini
lib_deps = 
    bblanchon/ArduinoJson@6.20.0
```

`HTTPClient.h` es parte del framework ESP32.

### 13. Próximos Pasos

- [ ] Probar integración completa inbound → backend → outbound
- [ ] Verificar que los datos se persistan correctamente
- [ ] Implementar manejo de errores de red más robusto
- [ ] Agregar indicadores LED de estado
- [ ] Implementar cola offline si backend no está disponible
