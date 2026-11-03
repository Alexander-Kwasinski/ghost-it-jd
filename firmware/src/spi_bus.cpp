#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "spi_bus.h"

static SemaphoreHandle_t busMutex = nullptr;


void spi_bus_init() {
    if (busMutex == nullptr) {
        busMutex = xSemaphoreCreateRecursiveMutex();
    }
}


void spi_bus_lock() {
    if (busMutex != nullptr) {
        xSemaphoreTakeRecursive(busMutex, portMAX_DELAY);
    }
}


void spi_bus_unlock() {
    if (busMutex != nullptr) {
        xSemaphoreGiveRecursive(busMutex);
    }
}
