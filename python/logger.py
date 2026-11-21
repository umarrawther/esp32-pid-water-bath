import serial, csv, re, threading
from datetime import datetime
from pathlib import Path

PORT = "COM4"        # ESP32's port
BAUD = 115200        # must match Serial.begin(115200) in the Arduino code

# save runs into the data/ folder next to this python/ folder
DATA_DIR = Path(__file__).resolve().parent.parent / "data"
DATA_DIR.mkdir(exist_ok=True)

# create a new CSV file with a timestamped name
filename = DATA_DIR / datetime.now().strftime("run_%Y-%m-%d_%H-%M-%S.csv")

# older sketches print lines like "123.4 s, 25.6 C"
old_pattern = re.compile(r"([\d.]+)\s*s,\s*(-?[\d.]+)\s*C")

with serial.Serial(PORT, BAUD, timeout=2) as ser, open(filename, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["time_s", "temp_C", "setpoint_C", "power_pct", "mode"])  # column headers
    print(f"Logging to {filename}. Press Ctrl+C to stop.")
    print("Type a command (e.g. MAN 50, ONOFF 40, OFF) and press Enter to send it to the ESP32.")

    # read what you type in a background thread and send it to the ESP32
    def send_commands():
        while True:
            try:
                cmd = input()
            except EOFError:
                return
            ser.write((cmd.strip() + "\n").encode())

    threading.Thread(target=send_commands, daemon=True).start()
    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if line.startswith("DATA,"):
                # bath_controller lines: DATA,time_s,temp_C,setpoint_C,power_pct,mode
                parts = line.split(",")
                if len(parts) == 6:
                    writer.writerow(parts[1:])
                    f.flush()                       # saves every line to disk as backup in case of crash
                    print(f"{parts[1]:>8} s   {parts[2]:>6} C   SP {parts[3]:>5}   power {parts[4]:>5} %   {parts[5]}")
                continue
            match = old_pattern.search(line)
            if match:
                writer.writerow([match.group(1), match.group(2), "", "", ""])
                f.flush()
                print(f"{float(match.group(1)):8.1f} s   {float(match.group(2)):6.2f} C")
            elif line:
                print("ESP32:", line)               # show other messages, e.g. "OK: manual power 50 %"
    except KeyboardInterrupt:
        print(f"Stopped. Data saved to {filename}")
