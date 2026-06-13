#include <gtest/gtest.h>
#include "ThreadSafeValue.h"
#include <string>

// Test Básico 1: Verificar el valor inicial
TEST(ThreadSafeValueTest, InitialValueIsCorrect) {
    ThreadSafeValue<int> val(42);
    EXPECT_EQ(val.get(), 42);
}

// Test Básico 2: Verificar que el set funciona
TEST(ThreadSafeValueTest, SetUpdatesValue) {
    ThreadSafeValue<float> val(0.0f);
    val.set(3.14f);
    EXPECT_FLOAT_EQ(val.get(), 3.14f);
}

// Test Básico 3: Verificar que funciona con strings (Tipos complejos)
TEST(ThreadSafeValueTest, HandlesStrings) {
    ThreadSafeValue<std::string> val("OFF");
    EXPECT_EQ(val.get(), "OFF");
    
    val.set("ON");
    EXPECT_EQ(val.get(), "ON");
}
