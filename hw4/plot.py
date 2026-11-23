import csv
from collections import defaultdict
from pathlib import Path
from statistics import median

import matplotlib.pyplot as plt

FILES = {
    "calc": "calc/times_calc_161727.csv",
    "remote.vdi": "vdi/times_vdi_238209.head.vdi.mipt.ru.csv",
}

OUT = Path("plots")
OUT.mkdir(exist_ok=True)

def read_times(filename):
    measurements = defaultdict(list)

    with open(filename, newline="") as f:
        for row in csv.DictReader(f):
            key = (row["version"], int(row["processes"]))
            measurements[key].append(float(row["time_s"]))

    return {key: median(values) for key, values in measurements.items()}

def draw(server, times, kind):
    plt.figure(figsize=(8, 5))

    for version, label in [("rows", "Rows"), ("cols", "Columns")]:
        processes = sorted(
            p for name, p in times if name == version
        )
        values = [times[(version, p)] for p in processes]

        if kind == "speedup":
            baseline = times[(version, 1)]
            values = [baseline / t for t in values]

        plt.plot(processes, values, "o-", label=label)

    if kind == "speedup":
        max_p = max(p for _, p in times)
        plt.plot(
            [1, max_p], [1, max_p],
            "k--", label="Ideal speedup"
        )
        plt.ylabel("S(p) = T(1) / T(p)")
    else:
        plt.ylabel("Execution time, seconds")
        plt.yscale("log")

    plt.xlabel("Number of processes")
    plt.title(f"{server}: {kind} (median of 3 runs)")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    filename = OUT / f"{server}_{kind}.png"
    plt.savefig(filename, dpi=200)
    plt.close()

for server, filename in FILES.items():
    times = read_times(filename)
    draw(server, times, "speedup")
    draw(server, times, "time")