#pragma once

void spi_bus_init();
void spi_bus_lock();
void spi_bus_unlock();

// Holds the shared OLED/SD SPI bus until the end of the scope.
class SpiBusLock {
public:
    SpiBusLock() {
        spi_bus_lock();
    }

    ~SpiBusLock() {
        spi_bus_unlock();
    }
};