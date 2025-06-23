/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#define STAY_AWAKE_PIN D0
#define NO_HEARTBEAT_PIN D0
#define FORMAT_LITTLEFS_PIN D1
#define LORA_LED_GREEN GPIO_NUM_48
#define LORA_USER_BUTTON GPIO_NUM_21
#define WAKEUP_PIN GPIO_NUM_9
#define MASK (1ULL << WAKEUP_PIN)

#define LORA_MODULE_PINOUT 0
#if LORA_MODULE_PINOUT == 0
#define CS 41
#define IRQ 39
#define RST 42
#define GPIO 40
#endif

#define LORA_SETTINGS 1
#if LORA_SETTINGS == 0
// Default parameters defined in SX1262.h.
#define FREQ 434.0  // Seeed XIA SX1262 range is 862 - 930 MHz
#define BW 125.0
#define SF 9
#define CR 7
#define SYNCWORD RADIOLIB_SX126X_SYNC_WORD_PRIVATE
#define POWER 10
#define PREAMBLELENGTH 8
#define TCXOVOLTAGE 1.6
#define USEREGULATORLDO false
#elif LORA_SETTINGS == 1
#define FREQ 868.0
#define BW 62.5
#define SF 12
#define CR 8
#define SYNCWORD 0x12
#define POWER 14
#define PREAMBLELENGTH 12
#define TCXOVOLTAGE 1.6
#define USEREGULATORLDO false
#endif

#if defined(ESP32)
// Add ESP32-specific code here if needed
#endif

#if defined(NRF52840_XXAA)
// Add NRF52840_XXAA-specific code here if needed
#endif
