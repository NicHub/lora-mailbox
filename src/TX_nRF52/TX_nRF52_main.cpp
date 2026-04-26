/**
 * LoRa MailBox
 *
 * Copyright (C) 2025, GPL-3.0-or-later, Nicolas Jeanmonod, ouilogique.com
 */

#include "TX_nRF52.h"

void setup()
{
    setupGPIOs();
    writeRgbLeds(0, 0, 1);
    setupRtcWakeup();
    next_heartbeat_deadline_ms = millis() + settings::misc::TX_HEARTBEAT_INTERVAL_MS;
    setupSerial();
    setupMsgCounterStorage();
    setupLoRa();
}

void loop()
{
    writeRgbLeds(0, 1, 0);
    uint16_t battery_voltage = readBatteryVoltage();
    uint16_t cnt = readMsgCounter();
    uint16_t v_initial_raw = readBatteryRawInitial();
    String payload = buildTxPayload(getBoardUidHex(), cnt, battery_voltage, v_initial_raw, tx_trigger);

    writeRgbLeds(1, 0, 0);
    sendLoRaPayload(payload.c_str());
    if (tx_trigger == TxTrigger::HeartbeatTx)
        advanceHeartbeatDeadline(millis());

    /**
     * @note Put the radio to sleep immediately after transmission.
     * @note The SX1262 consumes about 1.6 mA in standby versus about 0.9 uA in sleep.
     * @note RadioLib wakes it automatically on the next `transmit()` call.
     */
    radio.sleep();

    /** @note Flush serial before sleeping to avoid UART current draw on buffered data. */
    Serial.flush();

    writeRgbLeds(0, 1, 0);
    saveMsgCounter(++cnt);

    writeRgbLeds(0, 0, 0);

    /** @note Short debounce window before re-checking wakeup conditions. */
    sleepSecondsOrPin(settings::misc::TX_DEBOUNCE_S, false);

    uint32_t now_ms = millis();
    if (digitalRead(settings::board::WAKEUP_PIN))
    {
        /** @note PIR first: a motion event must never be masked by a due heartbeat. */
        tx_trigger = TxTrigger::WakeupPinHigh;
    }
    else if (isHeartbeatDue(now_ms))
    {
        tx_trigger = TxTrigger::HeartbeatTx;
    }
    else
    {
        /** @note Wait until the next heartbeat or until the wakeup pin rises. */
        uint32_t remaining_ms = next_heartbeat_deadline_ms - now_ms;
        uint32_t sleep_seconds = (remaining_ms + 999) / 1000;
        tx_trigger = sleepSecondsOrPin(sleep_seconds, true) ? TxTrigger::WakeupPinHigh
                                                            : TxTrigger::HeartbeatTx;
    }
}
