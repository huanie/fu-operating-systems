#pragma once

// technical reference pdf

// physical address space
auto constexpr PIOA = 0xfffff400;
// pio-controller register addresses
auto constexpr PIO_PDR = 0x4;
auto constexpr PIO_ASR = 0x70;
// pio-controller multiplexing
auto constexpr DBGU_RX_PIN = 1 << 30; // receive
auto constexpr DBGU_TX_PIN = 1 << 31; // transmit
auto constexpr DBGU_PINS = DBGU_RX_PIN | DBGU_TX_PIN;
