import serial, csv, re
from datetime import datetime
from pathlib import Path

PORT = "COM4"        # ESP32's port
BAUD = 115200        # must match Serial.begin(115200) in the Arduino code

# save runs into the data/ folder next to this python/ folder
DATA_DIR = Path(__file__).resolve().parent.parent / "data"
DATA_DIR.mkdir(exist_ok=True)

# create a new CSV file with a timestamped name
filename = DATA_DIR / datetime.now().strftime("run_%Y-%m-%d_%H-%M-%S.csv")

# matches lines like "123.4 s, 25.6 C" and captures the two numbers
pattern = re.compile(r"([\d.]+)\s*s,\s*(-?[\d.]+)\s*C")

with serial.Serial(PORT, BAUD, timeout=2) as ser, open(filename, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["time_s", "temp_C"])          # column headers
    print(f"Logging to {filename}. Press Ctrl+C to stop.")
    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            match = pattern.search(line)
            if match:
                t = float(match.group(1))
                temp = float(match.group(2))
                writer.writerow([t, temp])
                f.flush()                           # saves every line to disk as backup in case of crash
                print(f"{t:8.1f} s   {temp:6.2f} C")
            elif line:
                print("ESP32:", line)               # show other messages, e.g. "Sensors found"
    except KeyboardInterrupt:
        print(f"Stopped. Data saved to {filename}")