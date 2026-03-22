# LEDC Reference – Basic & Fade Examples

This document combines the reference material from the ESP‑IDF LEDC (LED Controller) basic and fade examples. It serves as a convenient single-file reference for PWM control and LED fading on ESP32‑series chips.

---

## 1. Overview

The LEDC peripheral provides:

- High‑resolution PWM generation
- Multiple speed modes
- Configurable timers and channels
- Hardware‑accelerated fade functions

These examples demonstrate:

- Basic PWM output
- Configuring LEDC timers and channels
- Using hardware fade to smoothly change duty cycle

Supported targets include:

- ESP32
- ESP32‑S2
- ESP32‑S3
- ESP32‑C3
- ESP32‑C6
- ESP32‑P4

---

## 2. LEDC Basic Example

### Purpose

Demonstrates how to:

- Configure an LEDC timer
- Configure an LEDC channel
- Set PWM duty cycle
- Update duty cycle in a loop

### Key Steps

1. **Configure LEDC timer**

   - Speed mode (high‑speed or low‑speed)
   - PWM frequency
   - Duty resolution (e.g., 13‑bit)

2. **Configure LEDC channel**

   - GPIO output pin
   - Channel number
   - Timer to use
   - Initial duty cycle

3. **Set duty cycle**

   Use:
ledc_set_duty()
ledc_update_duty()



4. **Loop and adjust brightness**

Increment or decrement duty cycle to change LED brightness.

---

## 3. LEDC Fade Example

### Purpose

Demonstrates how to use the hardware fade engine to smoothly transition PWM duty cycles.

### Features

- Fade up or down automatically
- Non‑blocking fade operations
- Configurable fade time in milliseconds

### Key Steps

1. **Enable fade service**

ledc_fade_func_install(0);


2. **Configure timer and channel**  
(same as basic example)

3. **Start fade**

ledc_set_fade_with_time();
ledc_fade_start();



4. **Loop fade patterns**

Example pattern:

- Fade from off → max brightness
- Fade from max → off
- Repeat

---

## 4. Typical LEDC Configuration Parameters

| Parameter        | Description                                      |
|------------------|--------------------------------------------------|
| speed_mode       | LEDC_LOW_SPEED_MODE or LEDC_HIGH_SPEED_MODE      |
| timer_num        | LEDC_TIMER_0 … LEDC_TIMER_3                      |
| freq_hz          | PWM frequency (e.g., 5000 Hz)                    |
| duty_resolution  | Bit depth (e.g., LEDC_TIMER_13_BIT)              |
| channel          | LEDC_CHANNEL_0 … LEDC_CHANNEL_7                  |
| gpio_num         | Output pin                                       |
| duty             | Initial duty cycle                               |
| hpoint           | Optional high‑point offset                       |

---

## 5. Example Fade Pattern (Conceptual)

Fade 0% → 100% over 1000 ms
Hold
Fade 100% → 0% over 1000 ms
Repeat


This is handled entirely by hardware, freeing the CPU.

---

## 6. Notes

- LEDC supports up to 8 channels depending on the chip.
- High‑speed mode uses APB clock; low‑speed mode uses REF_TICK.
- Fade engine is only available on supported channels.
- Duty resolution affects maximum frequency.

---

## 7. References

This merged document is based on the ESP‑IDF LEDC example READMEs from:

- `ledc_basic`
- `ledc_fade`

These examples are part of the official ESP‑IDF peripherals documentation.

