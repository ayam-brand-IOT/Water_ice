# Tests del Sistema Tote Inbound

## 📋 Tests Implementados

### 1. **test_states** - Transiciones de Estados
Valida que el flujo de estados funcione correctamente:
- ✅ IDLE → DISPENSING_ICE
- ✅ DISPENSING_ICE → DISPENSING_WATER
- ✅ DISPENSING_WATER → WAITING_TOTE_ID
- ✅ WAITING_TOTE_ID → COMPLETED
- ✅ COMPLETED → IDLE
- ✅ Cancelación (STOP)
- ✅ Flujo completo sin interrupciones

### 2. **test_backend** - Validación de JSON
Valida la estructura y contenido de los payloads JSON:
- ✅ Crear payload válido para POST
- ✅ Validar estructura JSON
- ✅ Parsear respuestas del backend
- ✅ Validar tipos de datos
- ✅ Detectar IDs vacíos
- ✅ Detectar valores negativos

### 3. **test_weight** - Validación de Pesos
Valida las mediciones y cálculos de peso:
- ✅ Rango válido (10kg - 500kg)
- ✅ Detección de valores fuera de rango
- ✅ Detección de NaN (error de sensor)
- ✅ Cálculo de diferencias (dispensado)
- ✅ Tolerancia de medición (±0.5kg)
- ✅ Suma de componentes
- ✅ Conversión y redondeo

## 🚀 Cómo Ejecutar los Tests

### Todos los tests
```bash
pio test -e test
```

### Test específico
```bash
# Solo transiciones de estados
pio test -e test -f test_states

# Solo validación de JSON
pio test -e test -f test_backend

# Solo validación de pesos
pio test -e test -f test_weight
```

### Con output verbose
```bash
pio test -e test -v
```

### En el ESP32 físico
```bash
pio test -e test --upload-port /dev/cu.wchusbserial10
```

## 📊 Ejemplo de Salida

```
Testing...
If you don't see any output for the first 10 secs, please reset board (press reset button)

test/test_states/test_state_transitions.cpp:27:test_idle_to_dispensing_ice	[PASSED]
test/test_states/test_state_transitions.cpp:35:test_dispensing_ice_to_water	[PASSED]
test/test_states/test_state_transitions.cpp:43:test_dispensing_water_to_waiting_id	[PASSED]
test/test_states/test_state_transitions.cpp:51:test_waiting_id_to_completed	[PASSED]
test/test_states/test_state_transitions.cpp:59:test_completed_to_idle	[PASSED]
test/test_states/test_state_transitions.cpp:67:test_cancel_from_any_state	[PASSED]
test/test_states/test_state_transitions.cpp:75:test_complete_flow	[PASSED]

-----------------------
7 Tests 0 Failures 0 Ignored 
OK
```

## 🎯 Qué Valida Cada Test

### Estados (test_states)
- Previene transiciones inválidas
- Garantiza que el flujo sigue el orden correcto
- Valida que STOP funciona desde cualquier estado

### Backend (test_backend)
- Garantiza que los datos enviados al backend son válidos
- Previene envío de datos corruptos
- Valida que las respuestas se parsean correctamente

### Pesos (test_weight)
- Detecta errores de sensor (NaN, negativos)
- Valida rangos de operación
- Garantiza cálculos correctos
- Previene datos fuera de rango

## 🔧 Integración Continua

Puedes agregar estos tests a tu workflow:

```bash
# Antes de flashear
pio test -e test

# Si pasan, flashear
pio run -e edgebox-esp-100 -t upload
```

## 📝 Agregar Nuevos Tests

1. Crear archivo en `test/test_nombre/`
2. Incluir `<unity.h>`
3. Crear funciones `test_algo()`
4. Agregar a `setup()` con `RUN_TEST(test_algo)`
5. Ejecutar con `pio test -f test_nombre`

## ⚠️ Notas Importantes

- Los tests se ejecutan **en el ESP32** (embedded tests)
- Requieren conexión física al dispositivo
- Delay de 2s al inicio para estabilización
- Los tests se ejecutan una sola vez en `setup()`
- Para re-ejecutar, presiona el botón RESET en el ESP32
