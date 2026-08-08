# Xiaomiao OS Hardware Design

## Battery Detection Circuit

Original button A pin (GPIO34) now used for battery voltage detection ADC.

### Divider Resistors
- Top: 9.1k Ohm (to Vbat)
- Bottom: 2.4k Ohm (to GND)
- ADC pin: GPIO32 (ADC1_CH4)

### Voltage Formula
```
Vadc = Vbat * R_bottom / (R_top + R_bottom)
Vadc = Vbat * 2.4 / (9.1 + 2.4) = Vbat / 4.792
Vbat = Vadc * 4.792
```

### ADC Config
- ADC1 Channel 4 (GPIO32)
- 12-bit resolution (0-4095)
- 12dB attenuation (full-scale ~3.3V)

| State | Voltage | ADC Raw | Percent |
|-------|---------|---------|---------|
| Full  | 4.2V    | 2684    | 100%    |
| Mid   | 3.7V    | 2365    | 53%     |
| Low   | 3.3V    | 2110    | 0%      |

## SPI Bus Conflict

LCD and SD card share SPI2_HOST - must alternate access:
1. LCD render: LCD CS low, SD CS high
2. SD read/write: SD CS low, LCD CS high
3. Use mutex to protect SPI bus access

## GPIO34/35 Limitations

- GPIO34 (Button A/ENTER): Input only, no internal pull-up
- GPIO35 (Button RIGHT): Input only, no internal pull-up
- External pull-up resistors required

## Backlight

Hardwired to VCC - no PWM brightness control.
Use DISPON/DISPOFF for power saving.