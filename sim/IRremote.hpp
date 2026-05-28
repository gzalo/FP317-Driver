#ifndef SIM_IRREMOTE_HPP
#define SIM_IRREMOTE_HPP

#include <cstdint>
#include <mutex>
#include <queue>

#define DISABLE_LED_FEEDBACK 0

struct IRData {
  uint8_t command = 0;
};

class SimIrReceiver {
 public:
  IRData decodedIRData;

  void begin(int pin, int feedback) {
    (void)pin;
    (void)feedback;
  }

  bool decode() {
    std::lock_guard<std::mutex> lock(commandsMutex);
    if (commands.empty()) {
      return false;
    }
    decodedIRData.command = commands.front();
    commands.pop();
    return true;
  }

  void resume() {}

  void inject(uint8_t command) {
    std::lock_guard<std::mutex> lock(commandsMutex);
    commands.push(command);
  }

 private:
  std::mutex commandsMutex;
  std::queue<uint8_t> commands;
};

inline SimIrReceiver IrReceiver;

#endif
