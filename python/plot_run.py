import sys
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

# data/ folder sits next to this python/ folder
DATA_DIR = Path(__file__).resolve().parent.parent / "data"

# Use the file named on the command line, or the newest run in data/ if none given
if len(sys.argv) > 1:
    path = Path(sys.argv[1])
    if not path.exists():
        path = DATA_DIR / sys.argv[1]
else:
    runs = sorted(DATA_DIR.glob("run_*.csv"))
    if not runs:
        sys.exit("No run_*.csv files found in the data folder yet.")
    path = runs[-1]

df = pd.read_csv(path)
plt.plot(df["time_s"], df["temp_C"])
plt.xlabel("Time (s)")
plt.ylabel("Temperature (°C)")
plt.title(path.name)
plt.grid(True)
plt.show()
