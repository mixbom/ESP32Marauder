#include "LedInterface.h"

LedInterface::LedInterface() {
  // Constructor remains empty
}

void LedInterface::RunSetup() {
  #ifdef HAS_NEOPIXEL_LED
    strip.setBrightness(50);
    strip.begin();
    strip.setPixelColor(0, strip.Color(0, 0, 0));
    strip.show();
  #endif

  #ifdef HAS_RMT_LED
    led_strip_config_t strip_config = {
        .strip_gpio_num = RMT_LED_GPIO,
        .max_leds = RMT_LED_COUNT,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,  // WS2812B uses GRB order
        .led_model = LED_MODEL_WS2812,             // Specifically for WS2812B
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,         // 10MHz for WS2812B
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
  #endif

  this->initTime = millis();
}

void LedInterface::setColor(int r, int g, int b) {
  #ifdef HAS_NEOPIXEL_LED
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
  #endif
  
  #ifdef HAS_RMT_LED
    // WS2812B uses GRB order, but our format is already set to GRB
    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
  #endif
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
  #if defined(HAS_NEOPIXEL_LED) || defined(HAS_RMT_LED)
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
  #if defined(HAS_NEOPIXEL_LED) || defined(HAS_RMT_LED)
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
