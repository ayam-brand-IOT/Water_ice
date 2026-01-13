#include <unity.h>
#include <Arduino.h>
#include <ArduinoJson.h>

void setUp(void) {
    // Se ejecuta antes de cada test
}

void tearDown(void) {
    // Se ejecuta después de cada test
}

// Test: Crear payload JSON válido
void test_create_tote_json_payload() {
    StaticJsonDocument<512> doc;
    
    // Simular datos del tote
    doc["tote_id"] = "TOTE001";
    doc["tote_kg"] = 100;
    doc["water_kg"] = 50;
    doc["ice_kg"] = 30;
    doc["raw_kg"] = 180;
    doc["water_out_kg"] = 0;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Verificar que el JSON contiene los campos esperados
    TEST_ASSERT_TRUE(jsonString.indexOf("\"tote_id\":\"TOTE001\"") > 0);
    TEST_ASSERT_TRUE(jsonString.indexOf("\"tote_kg\":100") > 0);
    TEST_ASSERT_TRUE(jsonString.indexOf("\"water_kg\":50") > 0);
    TEST_ASSERT_TRUE(jsonString.indexOf("\"ice_kg\":30") > 0);
    TEST_ASSERT_TRUE(jsonString.indexOf("\"raw_kg\":180") > 0);
}

// Test: JSON con valores límite
void test_json_with_boundary_values() {
    StaticJsonDocument<512> doc;
    
    doc["tote_id"] = "T-MAX-999";
    doc["tote_kg"] = 999;
    doc["water_kg"] = 999;
    doc["ice_kg"] = 999;
    doc["raw_kg"] = 2997;
    doc["water_out_kg"] = 0;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    TEST_ASSERT_TRUE(jsonString.length() > 0);
    TEST_ASSERT_TRUE(jsonString.indexOf("T-MAX-999") > 0);
}

// Test: Validar estructura JSON
void test_json_structure() {
    StaticJsonDocument<512> doc;
    
    doc["tote_id"] = "TOTE123";
    doc["tote_kg"] = 150;
    doc["water_kg"] = 75;
    doc["ice_kg"] = 45;
    doc["raw_kg"] = 270;
    doc["water_out_kg"] = 0;
    
    // Verificar que todos los campos están presentes
    TEST_ASSERT_TRUE(doc.containsKey("tote_id"));
    TEST_ASSERT_TRUE(doc.containsKey("tote_kg"));
    TEST_ASSERT_TRUE(doc.containsKey("water_kg"));
    TEST_ASSERT_TRUE(doc.containsKey("ice_kg"));
    TEST_ASSERT_TRUE(doc.containsKey("raw_kg"));
    TEST_ASSERT_TRUE(doc.containsKey("water_out_kg"));
}

// Test: Parsear JSON recibido del backend
void test_parse_backend_response() {
    const char* jsonResponse = "{\"success\":true,\"tote_id\":\"TOTE001\",\"message\":\"Tote created\"}";
    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonResponse);
    
    TEST_ASSERT_FALSE(error);
    TEST_ASSERT_TRUE(doc["success"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("TOTE001", doc["tote_id"].as<const char*>());
}

// Test: Validar tipos de datos
void test_json_data_types() {
    StaticJsonDocument<512> doc;
    
    doc["tote_id"] = "TOTE999";
    doc["tote_kg"] = 100;
    doc["water_kg"] = 50;
    doc["ice_kg"] = 30;
    doc["raw_kg"] = 180;
    doc["water_out_kg"] = 0;
    
    // Verificar tipos
    TEST_ASSERT_TRUE(doc["tote_id"].is<const char*>());
    TEST_ASSERT_TRUE(doc["tote_kg"].is<int>());
    TEST_ASSERT_TRUE(doc["water_kg"].is<int>());
    TEST_ASSERT_TRUE(doc["ice_kg"].is<int>());
    TEST_ASSERT_TRUE(doc["raw_kg"].is<int>());
}

// Test: ID de tote vacío (error)
void test_empty_tote_id() {
    StaticJsonDocument<512> doc;
    
    doc["tote_id"] = "";  // ID vacío
    doc["tote_kg"] = 100;
    
    String toteId = doc["tote_id"].as<String>();
    
    TEST_ASSERT_TRUE(toteId.length() == 0);
    // En la implementación real, esto debería rechazarse
}

// Test: Valores negativos (error)
void test_negative_values() {
    StaticJsonDocument<512> doc;
    
    doc["tote_id"] = "TOTE001";
    doc["tote_kg"] = -100;  // Valor negativo inválido
    
    int toteKg = doc["tote_kg"].as<int>();
    
    TEST_ASSERT_TRUE(toteKg < 0);
    // En la implementación real, esto debería validarse y rechazarse
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_create_tote_json_payload);
    RUN_TEST(test_json_with_boundary_values);
    RUN_TEST(test_json_structure);
    RUN_TEST(test_parse_backend_response);
    RUN_TEST(test_json_data_types);
    RUN_TEST(test_empty_tote_id);
    RUN_TEST(test_negative_values);
    
    UNITY_END();
}

void loop() {
    // Tests ejecutados una sola vez
}
