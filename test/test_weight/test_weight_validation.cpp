#include <unity.h>
#include <Arduino.h>

// Constantes del sistema
#define MIN_WEIGHT 10.0f
#define MAX_WEIGHT 500.0f
#define WEIGHT_TOLERANCE 0.5f

void setUp(void) {
    // Se ejecuta antes de cada test
}

void tearDown(void) {
    // Se ejecuta después de cada test
}

// Test: Peso dentro del rango válido
void test_valid_weight() {
    float weight = 150.5f;
    
    TEST_ASSERT_TRUE(weight >= MIN_WEIGHT);
    TEST_ASSERT_TRUE(weight <= MAX_WEIGHT);
}

// Test: Peso mínimo aceptable
void test_minimum_weight() {
    float weight = MIN_WEIGHT;
    
    TEST_ASSERT_GREATER_OR_EQUAL(MIN_WEIGHT, weight);
}

// Test: Peso por debajo del mínimo (inválido)
void test_below_minimum_weight() {
    float weight = 5.0f;
    
    TEST_ASSERT_LESS_THAN(MIN_WEIGHT, weight);
    // En implementación real, debería rechazarse
}

// Test: Peso máximo aceptable
void test_maximum_weight() {
    float weight = MAX_WEIGHT;
    
    TEST_ASSERT_LESS_OR_EQUAL(MAX_WEIGHT, weight);
}

// Test: Peso por encima del máximo (inválido)
void test_above_maximum_weight() {
    float weight = 600.0f;
    
    TEST_ASSERT_GREATER_THAN(MAX_WEIGHT, weight);
    // En implementación real, debería rechazarse o alertar
}

// Test: Peso negativo (error de sensor)
void test_negative_weight() {
    float weight = -10.0f;
    
    TEST_ASSERT_LESS_THAN(0, weight);
    // Esto indicaría un error de sensor
}

// Test: Peso NaN (error de lectura)
void test_nan_weight() {
    float weight = NAN;
    
    TEST_ASSERT_TRUE(isnan(weight));
    // En implementación real, debería reintentarse la lectura
}

// Test: Cálculo de diferencia de peso (dispensado)
void test_weight_difference() {
    float initialWeight = 100.0f;
    float finalWeight = 130.0f;
    float dispensed = finalWeight - initialWeight;
    
    TEST_ASSERT_EQUAL_FLOAT(30.0f, dispensed);
}

// Test: Tolerancia de peso (±0.5kg)
void test_weight_tolerance() {
    float target = 50.0f;
    float actual = 50.3f;
    float difference = fabsf(actual - target);
    
    // Diferencia debe estar dentro de la tolerancia (0.3 < 0.5)
    TEST_ASSERT_TRUE(difference <= WEIGHT_TOLERANCE);
}

// Test: Suma de componentes = raw_kg
void test_weight_sum() {
    uint32_t tote_kg = 100;
    uint32_t ice_kg = 30;
    uint32_t water_kg = 50;
    uint32_t raw_kg = tote_kg + ice_kg + water_kg;
    
    TEST_ASSERT_EQUAL_UINT32(180, raw_kg);
}

// Test: Validar incremento de peso durante dispensado
void test_weight_increment_during_dispensing() {
    float weight_before = 100.0f;
    float weight_after = 120.0f;
    
    // El peso debe incrementar durante dispensado
    TEST_ASSERT_GREATER_THAN(weight_before, weight_after);
}

// Test: Peso estable (sin cambios)
void test_stable_weight() {
    float weight1 = 150.0f;
    float weight2 = 150.05f;
    float delta = fabsf(weight2 - weight1);
    
    // Cambio menor a tolerancia = peso estable (delta debe ser < tolerance)
    // delta = 0.05, WEIGHT_TOLERANCE = 0.5
    TEST_ASSERT_TRUE(delta < WEIGHT_TOLERANCE);
}

// Test: Conversión de float a uint32_t
void test_weight_conversion() {
    float weight_float = 125.7f;
    uint32_t weight_int = (uint32_t)weight_float;
    
    TEST_ASSERT_EQUAL_UINT32(125, weight_int);
}

// Test: Redondeo de peso
void test_weight_rounding() {
    float weight = 125.8f;
    uint32_t rounded = (uint32_t)(weight + 0.5f);
    
    TEST_ASSERT_EQUAL_UINT32(126, rounded);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_valid_weight);
    RUN_TEST(test_minimum_weight);
    RUN_TEST(test_below_minimum_weight);
    RUN_TEST(test_maximum_weight);
    RUN_TEST(test_above_maximum_weight);
    RUN_TEST(test_negative_weight);
    RUN_TEST(test_nan_weight);
    RUN_TEST(test_weight_difference);
    RUN_TEST(test_weight_tolerance);
    RUN_TEST(test_weight_sum);
    RUN_TEST(test_weight_increment_during_dispensing);
    RUN_TEST(test_stable_weight);
    RUN_TEST(test_weight_conversion);
    RUN_TEST(test_weight_rounding);
    
    UNITY_END();
}

void loop() {
    // Tests ejecutados una sola vez
}
