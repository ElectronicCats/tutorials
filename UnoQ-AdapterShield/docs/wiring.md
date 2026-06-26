# Tracing the GIGA Display Shield DSI and touch lines to the UNO Q

The GIGA Display Shield (ASX00039) was designed for the Arduino GIGA R1, not the UNO Q. The display panel works through the UNO Q's stock JMEDIA connector, but the touch controller's I²C / RST / INT signals are not auto-routed.

## Pinout

| Shield signal | UNO Q JMISC pin | UNO Q JMEDIA pin | SoC GPIO | Function | Logic level |
|---|---|---|---|---|---|
| SDA (touch, shield pin D102) | **39** | - | GPIO_1 | qup0 SDA | 1.8 V |
| SCL (touch, shield pin D101) | **37** | - | GPIO_0 | qup0 SCL | 1.8 V |
| RST (touch) | **49** | - | GPIO_18 | reset | 1.8 V |
| INT (touch) | **46** | - | GPIO_98 | interrupt | 1.8 V |
| D0N (DSI) | - | L0N (DSI) | MIPI_DSI0_L0_N | DSI Lane 0 Negative | 1.8 V |
| D0P (DSI) | - | L0P (DSI) | MIPI_DSI0_L0_P | DSI Lane 0 Positive | 1.8 V |
| D1N (DSI) | - | L1N (DSI) | MIPI_DSI1_L1_N | DSI Lane 1 Negative | 1.8 V |
| D1P (DSI) | - | L1P (DSI) | MIPI_DSI1_L1_P | DSI Lane 1 Positive | 1.8 V |
| CKN (DSI) | - | CLKN (DSI) | MIPI_DSI1_CLK_N | DSI Clock Negative | 1.8 V |
| CKP (DSI) | - | CLKP (DSI) | MIPI_DSI1_CLK_P | DSI Clock Positive | 1.8 V |
| 3V3 (shield) | **53** or **55** | **58** or **60** | +3V3 | power | — |
| GND | any GND on JMISC | any GND on JMEDIA | - | ground | — |

Find the schematics in [HW](https://github.com/ElectronicCats/tutorials/tree/main/UnoQ-AdapterShield/HW/AdapterShield).

## Pitfalls

- **SDA / SCL are not in the order you'd expect.** Inside the QUP0 controller, GPIO_0 is the clock (SCL) and GPIO_1 is the data line (SDA) — the opposite of the nominal pinmux order. Verified empirically with an oscilloscope on a working board. If you swap them, `i2cdetect -y 0` reports nothing.
- **Level shifter not strictly required.** The shield's touch lines are 3.3 V; the UNO Q QUP0 is 1.8 V. The asymmetry creates a marginal swing (~1.9 V high) that works empirically over the short wire lengths used here. If your wires are long (more than ~10 cm) or if you see I²C errors in `dmesg`, add a level shifter.
- **Shield 3.3 V power is mandatory.** The touch chip needs its own VCC. JMISC pins 53 and 55 are both +3V3 and either works.

## Verifying the wiring before software setup

After flashing the official UNO Q image and connecting the shield, before running anything from this repo:

```bash
ssh arduino@<UNO-Q-IP>
sudo i2cdetect -y 0
```

You should see an address `0x14` (or `0x5d`) light up. If nothing appears at all, check power and SDA/SCL. If only one line appears, double-check you didn't swap SDA and SCL.
