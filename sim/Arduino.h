#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define HEX 16
#define PROGMEM

typedef uint8_t byte;

namespace SimArduino {
inline auto startedAt = std::chrono::steady_clock::now();
inline std::map<int, int> pinStates;
inline std::function<void(int, int)> digitalWriteHook;
inline std::function<void()> delayHook;
inline std::function<bool()> stopDelayHook;
inline double delayScale = 1.0;
}

inline unsigned long millis() {
  auto elapsed = std::chrono::steady_clock::now() - SimArduino::startedAt;
  return static_cast<unsigned long>(
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
}

inline void delay(unsigned long ms) {
  if (ms == 0) {
    return;
  }

  auto scaled = std::chrono::duration<double, std::milli>(ms * SimArduino::delayScale);
  auto deadline = std::chrono::steady_clock::now() +
                  std::chrono::duration_cast<std::chrono::steady_clock::duration>(scaled);
  if (SimArduino::delayHook) {
    SimArduino::delayHook();
  }
  while (std::chrono::steady_clock::now() < deadline) {
    if (SimArduino::stopDelayHook && SimArduino::stopDelayHook()) {
      return;
    }
    if (SimArduino::delayHook) {
      SimArduino::delayHook();
    }
    auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining > std::chrono::milliseconds(20)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      std::this_thread::yield();
    }
  }
}

inline void delayMicroseconds(unsigned int us) {
  (void)us;
}

inline void pinMode(int pin, int mode) {
  (void)pin;
  (void)mode;
}

inline void digitalWrite(int pin, int value) {
  SimArduino::pinStates[pin] = value ? HIGH : LOW;
  if (SimArduino::digitalWriteHook) {
    SimArduino::digitalWriteHook(pin, value ? HIGH : LOW);
  }
}

inline long random(long max) {
  return max <= 0 ? 0 : std::rand() % max;
}

inline long random(long min, long max) {
  return min + random(max - min);
}

class SimSerial {
 public:
  void begin(unsigned long baud) {
    (void)baud;
  }

  template <typename T>
  void print(const T& value) {
    std::cout << value;
  }

  void print(uint8_t value, int base) {
    if (base == HEX) {
      std::cout << std::hex << static_cast<int>(value) << std::dec;
    } else {
      std::cout << static_cast<int>(value);
    }
  }

  template <typename T>
  void println(const T& value) {
    std::cout << value << '\n';
  }

  void println(uint8_t value, int base) {
    print(value, base);
    std::cout << '\n';
  }
};

inline SimSerial Serial;

#endif
