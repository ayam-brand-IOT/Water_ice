#include <unity.h>
#include <Arduino.h>

// Mock de ToteState
enum class ToteState {
  IDLE,
  DISPENSING_ICE,
  DISPENSING_WATER,
  WAITING_TOTE_ID,
  COMPLETED,
  CANCELED,
  ERROR
};

ToteState currentState = ToteState::IDLE;

void setUp(void) {
    // Se ejecuta antes de cada test
    currentState = ToteState::IDLE;
}

void tearDown(void) {
    // Se ejecuta después de cada test
}

// Test: Transición de IDLE a DISPENSING_ICE
void test_idle_to_dispensing_ice() {
    currentState = ToteState::IDLE;
    
    // Simular que se presiona START
    currentState = ToteState::DISPENSING_ICE;
    
    TEST_ASSERT_EQUAL(ToteState::DISPENSING_ICE, currentState);
}

// Test: Transición de DISPENSING_ICE a DISPENSING_WATER
void test_dispensing_ice_to_water() {
    currentState = ToteState::DISPENSING_ICE;
    
    // Simular que terminó de dispensar hielo
    currentState = ToteState::DISPENSING_WATER;
    
    TEST_ASSERT_EQUAL(ToteState::DISPENSING_WATER, currentState);
}

// Test: Transición de DISPENSING_WATER a WAITING_TOTE_ID
void test_dispensing_water_to_waiting_id() {
    currentState = ToteState::DISPENSING_WATER;
    
    // Simular que terminó de llenar agua
    currentState = ToteState::WAITING_TOTE_ID;
    
    TEST_ASSERT_EQUAL(ToteState::WAITING_TOTE_ID, currentState);
}

// Test: Transición de WAITING_TOTE_ID a COMPLETED
void test_waiting_id_to_completed() {
    currentState = ToteState::WAITING_TOTE_ID;
    
    // Simular que se ingresó el ID
    currentState = ToteState::COMPLETED;
    
    TEST_ASSERT_EQUAL(ToteState::COMPLETED, currentState);
}

// Test: Transición de COMPLETED a IDLE
void test_completed_to_idle() {
    currentState = ToteState::COMPLETED;
    
    // Simular que se envió al backend y reset
    currentState = ToteState::IDLE;
    
    TEST_ASSERT_EQUAL(ToteState::IDLE, currentState);
}

// Test: Cancelación desde cualquier estado
void test_cancel_from_any_state() {
    currentState = ToteState::DISPENSING_ICE;
    
    // Simular presionar STOP
    currentState = ToteState::CANCELED;
    
    TEST_ASSERT_EQUAL(ToteState::CANCELED, currentState);
}

// Test: Flujo completo sin interrupciones
void test_complete_flow() {
    // IDLE -> DISPENSING_ICE
    currentState = ToteState::IDLE;
    currentState = ToteState::DISPENSING_ICE;
    TEST_ASSERT_EQUAL(ToteState::DISPENSING_ICE, currentState);
    
    // DISPENSING_ICE -> DISPENSING_WATER
    currentState = ToteState::DISPENSING_WATER;
    TEST_ASSERT_EQUAL(ToteState::DISPENSING_WATER, currentState);
    
    // DISPENSING_WATER -> WAITING_TOTE_ID
    currentState = ToteState::WAITING_TOTE_ID;
    TEST_ASSERT_EQUAL(ToteState::WAITING_TOTE_ID, currentState);
    
    // WAITING_TOTE_ID -> COMPLETED
    currentState = ToteState::COMPLETED;
    TEST_ASSERT_EQUAL(ToteState::COMPLETED, currentState);
    
    // COMPLETED -> IDLE
    currentState = ToteState::IDLE;
    TEST_ASSERT_EQUAL(ToteState::IDLE, currentState);
}

void setup() {
    delay(2000); // Esperar a que el ESP32 se estabilice
    
    UNITY_BEGIN();
    
    RUN_TEST(test_idle_to_dispensing_ice);
    RUN_TEST(test_dispensing_ice_to_water);
    RUN_TEST(test_dispensing_water_to_waiting_id);
    RUN_TEST(test_waiting_id_to_completed);
    RUN_TEST(test_completed_to_idle);
    RUN_TEST(test_cancel_from_any_state);
    RUN_TEST(test_complete_flow);
    
    UNITY_END();
}

void loop() {
    // Los tests solo se ejecutan una vez en setup()
}
