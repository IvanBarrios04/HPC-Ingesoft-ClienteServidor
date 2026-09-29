import pandas as pd
import matplotlib.pyplot as plt

# -------------------------
# Cargar datos
# -------------------------
m1_openmp = pd.read_csv("times_m1/times_openmp.csv")
m1_seq = pd.read_csv("times_m1/times_sequential.csv")

m2_openmp = pd.read_csv("times_m2/times_openmp.csv")
m2_seq = pd.read_csv("times_m2/times_sequential.csv")

# Eliminar columna Run
m1_openmp = m1_openmp.drop(columns=["Run"])
m1_seq = m1_seq.drop(columns=["Run"])

m2_openmp = m2_openmp.drop(columns=["Run"])
m2_seq = m2_seq.drop(columns=["Run"])

# -------------------------
# Promedios
# -------------------------
m1_openmp_mean = m1_openmp.mean().round(6)
m1_seq_mean = m1_seq.mean().round(6)

m2_openmp_mean = m2_openmp.mean().round(6)
m2_seq_mean = m2_seq.mean().round(6)

sizes = m1_openmp.columns.astype(int)

# -------------------------
# Speedup
# -------------------------
m1_speedup = (m1_seq_mean / m1_openmp_mean).round(6)
m2_speedup = (m2_seq_mean / m2_openmp_mean).round(6)

# -------------------------
# Gráficas
# -------------------------
plt.figure()
plt.plot(sizes, m1_speedup, marker='o')
plt.xlabel("Tamaño")
plt.ylabel("Speedup")
plt.title("Speedup Máquina 1")
plt.grid()
plt.savefig("speedup_m1.png")

plt.figure()
plt.plot(sizes, m2_speedup, marker='o')
plt.xlabel("Tamaño")
plt.ylabel("Speedup")
plt.title("Speedup Máquina 2")
plt.grid()
plt.savefig("speedup_m2.png")

plt.figure()
plt.plot(sizes, m1_openmp_mean, marker='o', label="M1")
plt.plot(sizes, m2_openmp_mean, marker='o', label="M2")
plt.xlabel("Tamaño")
plt.ylabel("Tiempo")
plt.title("OpenMP M1 vs M2")
plt.legend()
plt.grid()
plt.savefig("openmp_comparison.png")

plt.figure()
plt.plot(sizes, m1_seq_mean, marker='o', label="M1")
plt.plot(sizes, m2_seq_mean, marker='o', label="M2")
plt.xlabel("Tamaño")
plt.ylabel("Tiempo")
plt.title("Secuencial M1 vs M2")
plt.legend()
plt.grid()
plt.savefig("sequential_comparison.png")

# -------------------------
# Función para crear tablas
# -------------------------
def create_table(series, column_name):
    df = series.to_frame(name=column_name)
    df.index.name = "Tamaño"
    df.reset_index(inplace=True)
    return df

# -------------------------
# Tablas de tiempos
# -------------------------
table_m1_openmp = create_table(m1_openmp_mean, "Tiempo Promedio")
table_m1_seq = create_table(m1_seq_mean, "Tiempo Promedio")

table_m2_openmp = create_table(m2_openmp_mean, "Tiempo Promedio")
table_m2_seq = create_table(m2_seq_mean, "Tiempo Promedio")

# -------------------------
# Tablas de speedup
# -------------------------
table_m1_speedup = create_table(m1_speedup, "Speedup")
table_m2_speedup = create_table(m2_speedup, "Speedup")

# -------------------------
# Imprimir tablas (para copiar a Docs)
# -------------------------
print("\n===== M1 OpenMP =====")
print(table_m1_openmp.to_csv(index=False))

print("\n===== M1 Secuencial =====")
print(table_m1_seq.to_csv(index=False))

print("\n===== M2 OpenMP =====")
print(table_m2_openmp.to_csv(index=False))

print("\n===== M2 Secuencial =====")
print(table_m2_seq.to_csv(index=False))

print("\n===== Speedup M1 =====")
print(table_m1_speedup.to_csv(index=False))

print("\n===== Speedup M2 =====")
print(table_m2_speedup.to_csv(index=False))

# -------------------------
# Guardar tablas en CSV
# -------------------------
table_m1_openmp.to_csv("table_m1_openmp.csv", index=False)
table_m1_seq.to_csv("table_m1_seq.csv", index=False)
table_m2_openmp.to_csv("table_m2_openmp.csv", index=False)
table_m2_seq.to_csv("table_m2_seq.csv", index=False)

table_m1_speedup.to_csv("table_m1_speedup.csv", index=False)
table_m2_speedup.to_csv("table_m2_speedup.csv", index=False)