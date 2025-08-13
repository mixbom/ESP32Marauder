#include "LedInterface.h"
#include "driver/gpio.h"

LedInterface::LedInterface() {
  #if CONFIG_IDF_TARGET_ESP32C5
    use_bitbang = true;
  #endif
}

void LedInterface::RunSetup() {
  #ifdef HAS_NEOPIXEL_LED
    #if CONFIG_IDF_TARGET_ESP32C5
      // ESP32-C5 specific setup
      gpio_reset_pin((gpio_num_t)NEOPIXEL_PIN);
      gpio_set_direction((gpio_num_t)NEOPIXEL_PIN, GPIO_MODE_OUTPUT);
      gpio_set_level((gpio_num_t)NEOPIXEL_PIN, 0);
    #else
      // Standard setup for other boards
      strip.begin();
      strip.setBrightness(50);
      strip.setPixelColor(0, strip.Color(0, 0, 0));
      strip.show();
    #endif
  #endif

  this->initTime = millis();
}

void LedInterface::setPixelColor(uint8_t r, uint8_t g, uint8_t b) {
  #ifdef HAS_NEOPIXEL_LED
    #if CONFIG_IDF_TARGET_ESP32C5
      // Custom bitbanging for WS2812B on ESP32-C5
      const int pin = NEOPIXEL_PIN;
      uint8_t bytes[3] = {g, r, b};  // GRB order for WS2812B
      
      // Critical section for precise timing
      portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
      portENTER_CRITICAL_SAFE(&mux);
      
      for (int i = 0; i < 3; i++) {
        for (int mask = 0x80; mask; mask >>= 1) {
          if (bytes[i] & mask) {
            // '1' bit: HIGH for 0.7µs, LOW for 0.6µs
            gpio_set_level((gpio_num_t)pin, 1);
            ets_delay_us(0.7);
            gpio_set_level((gpio_num_t)pin, 0);
            ets_delay_us(0.6);
          } else {
            // '0' bit: HIGH for 0.35µs, LOW for 0.8µs
            gpio_set_level((gpio_num_t)pin, 1);
            ets_delay_us(0.35);
            gpio_set_level((gpio_num_t)pin, 0);
            ets_delay_us(0.8);
          }
        }
      }
      
      portEXIT_CRITICAL_SAFE(&mux);
    #else
      // Standard NeoPixel for other boards
      strip.setPixelColor(0, strip.Color(r, g, b));
      strip.show();
    #endif
  #endif
}

void LedInterface::setColor(int r, int g, int b) {
  // Clamp values to 0-255
  r = (r < 0) ? 0 : (r > 255) ? 255 : r;
  g = (g < 0) ? 0 : (g > 255) ? 255 : g;
  b = (b < 0) ? 0 : (b > 255) ? 255 : b;
  
  this->setPixelColor((uint8_t)r, (uint8_t)g, (uint8_t)b);
}

void LedInterface::setMode(uint8_t new_mode) {
  this->current_mode = new_mode;
}

uint8_t LedInterface::getMode() {
  return this->current_mode;
}

void LedInterface::sniffLed() {
  this->setColor(0, 0, 255);
}

void LedInterface::attackLed() {
  this->setColor(255, 0, 0);
}

void LedInterface::ledOff() {
  this->setColor(0, 0, 0);
}

void LedInterface::probeDetectLed() {
  this->setColor(0, 50, 0);  // Green flash
  delay(100);
  this->ledOff();
}

void LedInterface::deauthLed() {
  this->setColor(50, 0, 0);  // Solid red
}

void LedInterface::beaconLed() {
  this->setColor(50, 30, 0);  // Orange
}

void LedInterface::main(uint32_t currentTime) {
  if ((!settings_obj.loadSetting<bool>("EnableLED")) ||
      (this->current_mode == MODE_OFF)) {
    this->ledOff();
    return;
  }
  else if (this->current_mode == MODE_RAINBOW) {
    this->rainbow();
  }
  else if (this->current_mode == MODE_ATTACK) {
    this->attackLed();
  }
  else if (this->current_mode == MODE_SNIFF) {
    this->sniffLed();
  }
  else if (this->current_mode == MODE_PROBE_DETECT) {
    this->probeDetectLed();
    this->current_mode = MODE_SNIFF;  // Revert to sniff mode
  }
  else if (this->current_mode == MODE_DEAUTH_ACTIVE) {
    this->deauthLed();
  }
  else if (this->current_mode == MODE_BEACON_ACTIVE) {
    this->beaconLed();
  }
  else if (this->current_mode == MODE_CUSTOM) {
    return;
  }
}

void LedInterface::rainbow() {
  #ifdef HAS_NEOPIXEL_LED
    uint32_t color = this->Wheel((0 * 256 / 100 + this->wheel_pos) % 256);
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    this->setColor(r, g, b);

    this->current_fade_itter++;
    this->wheel_pos = this->wheel_pos - this->wheel_speed;
    if (this->wheel_pos < 0) this->wheel_pos = 255;
  #endif
}

uint32_t LedInterface::Wheel(byte WheelPos) {
  #ifdef HAS_NEOPIXEL_LED
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
      return ((uint32_t)(255 - WheelPos * 3) << 16) | 
             ((uint32_t)(0) << 8) | 
             (WheelPos * 3);
    }
    if(WheelPos < 170) {
      WheelPos -= 85;
      return ((uint32_t)(0) << 16) | 
             ((uint32_t)(WheelPos * 3) << 8) | 
             (255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return ((uint32_t)(WheelPos * 3) << 16) | 
           ((uint32_t)(255 - WheelPos * 3) << 8) | 
           0;
  #endif
  return 0;
}
