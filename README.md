# Closed-Loop Temperature Control System (ESP32 PID Water Bath)

*Work in progress, started November 2026.*

A small water bath that holds a set temperature (35–45 °C) using a PID controller running on an ESP32. A DS18B20 probe measures the water, and the ESP32 switches a 12 V, 30 W silicone heater pad through a MOSFET module using slow PWM. A Python script logs every reading to CSV for analysis.

The goal is to model the bath with an energy balance, fit the model to step-test data, and compare tuned PID control against simple on/off control.

## Hardware
- ESP32-WROOM-32 dev board (CP2102 USB)
- DS18B20 waterproof temperature probe with adapter board
- MOSFET trigger module (3.3 V logic)
- 12 V 5 A UL-listed power supply
- 12 V 30 W silicone heater pad under a small stainless pot
- Safety: 5 A inline fuse, 73 °C thermal fuse on the pot wall, spill tray

## Repository layout
| Folder | Contents |
| --- | --- |
| `firmware/` | Arduino sketches for the ESP32 |
| `python/` | `logger.py` (serial → CSV) and `plot_run.py` (plots a run) |
| `data/` | CSV files from experiment runs |
| `docs/` | Photos, wiring diagrams, notes |

## Running the logger
1. Upload a sketch that prints lines like `12.3 s, 21.94 C`.
2. Close the Arduino Serial Monitor (only one program can use the port).
3. From the `python` folder: `python logger.py` (Ctrl+C to stop). Runs are saved to `data/`.
4. Plot the newest run: `python plot_run.py`

## Progress
- [x] Sensor reading and data logging
- [x] Non-blocking timing (`millis()`)
- [ ] Slow PWM heater output
- [ ] Power wiring and first step test
- [ ] Model fitting
- [ ] On/off, P, PI/PID control and comparison
