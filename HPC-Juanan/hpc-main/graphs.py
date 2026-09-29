import pandas as pd
import matplotlib.pyplot as plt

# Files and corresponding thread counts
files = {
    1: "matrix-product/times_1.csv",
    2: "matrix-product/times_2.csv",
    4: "matrix-product/times_4.csv",
    6: "matrix-product/times_6.csv",
    8: "matrix-product/times_8.csv"
}

avg_times = {}

# Load each file and compute mean time
for threads, file in files.items():
    df = pd.read_csv(file)

    avg = df.iloc[:,1:].mean()
    avg.index = avg.index.astype(int)

    avg_times[threads] = avg

# baseline
baseline = avg_times[1]

# problem sizes
sizes = baseline.index

# =========================
# GRAPH 1: Speedup vs Input Size
# =========================

plt.figure()

for threads, times in avg_times.items():
    speedup = baseline / times
    plt.plot(sizes, speedup, marker='o', label=f"{threads} threads")

plt.xlabel("Input Size")
plt.ylabel("Speedup")
plt.title("Speedup vs Input Size")
plt.grid(True)
plt.legend()

# =========================
# GRAPH 2: Speedup vs Threads
# =========================

plt.figure()

threads_list = sorted(avg_times.keys())

for size in sizes:

    speedups = []

    for threads in threads_list:
        t1 = avg_times[1][size]
        tp = avg_times[threads][size]
        speedups.append(t1 / tp)

    plt.plot(threads_list, speedups, marker='o', label=f"Size {size}")

plt.xlabel("Number of Threads")
plt.ylabel("Speedup")
plt.title("Speedup vs Threads")
plt.grid(True)
plt.legend()

plt.show()