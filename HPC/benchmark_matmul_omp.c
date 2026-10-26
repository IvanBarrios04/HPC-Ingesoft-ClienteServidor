/*
 * benchmark_matmul_omp.c
 * Benchmark de multiplicación de matrices cuadradas de enteros con OpenMP.
 *
 * Barrido:  N      = {500, 1000, 2000, 4000, 8000}
 *           hilos  = {1, 2, 4, 6, 8, 16}   (1 = línea base para speedup)
 *           repeticiones por configuración = 10 (configurable)
 *
 * Salida:   resultados.csv  ->  n,hilos,repeticion,tiempo_s
 *
 * Compilar: gcc -O2 -fopenmp benchmark_matmul_omp.c -o benchmark_matmul_omp
 * Ejecutar: ./benchmark_matmul_omp [repeticiones] [salida.csv] [n_max]
 *           ./benchmark_matmul_omp 10 resultados.csv 8000
 *
 * Solo se cronometra el producto (A x B -> C). La generación de matrices y
 * la reserva de C quedan fuera de la medición.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

static const int SIZES[]   = {500, 1000, 2000, 4000, 8000};
static const int THREADS[] = {1, 2, 4, 6, 8, 16};
#define N_SIZES   (int)(sizeof(SIZES)   / sizeof(SIZES[0]))
#define N_THREADS (int)(sizeof(THREADS) / sizeof(THREADS[0]))

/* Reserva una matriz size x size (filas separadas, como en el código original). */
static int** alloc_matrix(int size) {
  int** m = malloc((size_t)size * sizeof(int*));
  if (!m) { perror("malloc"); exit(1); }
  for (int i = 0; i < size; i++) {
    m[i] = malloc((size_t)size * sizeof(int));
    if (!m[i]) { perror("malloc"); exit(1); }
  }
  return m;
}

static void free_matrix(int** m, int size) {
  for (int i = 0; i < size; i++) free(m[i]);
  free(m);
}

/* Rellena con valores aleatorios en [0, 471]. */
static void fill_random(int** m, int size) {
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      m[i][j] = rand() % 472;
}

/* C = A x B. Paraleliza los dos bucles externos (n^2 iteraciones repartidas). */
static void matrix_product(int** a, int** b, int** c, int size) {
  #pragma omp parallel for collapse(2) schedule(static)
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      long long sum = 0;              /* long long: evita desbordar int */
      for (int k = 0; k < size; k++)
        sum += (long long)a[i][k] * b[k][j];
      c[i][j] = (int)sum;
    }
  }
}

/* Suma de todos los elementos de C: sirve para verificar que todas las
 * configuraciones de hilos producen el mismo resultado. */
static long long checksum(int** c, int size) {
  long long s = 0;
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++)
      s += c[i][j];
  return s;
}

static double now_s(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + t.tv_nsec / 1e9;
}

int main(int argc, char* argv[]) {
  int reps            = (argc > 1) ? atoi(argv[1]) : 10;
  const char* outname = (argc > 2) ? argv[2] : "resultados.csv";
  int n_max           = (argc > 3) ? atoi(argv[3]) : 8000;
  if (reps < 1) { fprintf(stderr, "repeticiones debe ser >= 1\n"); return 1; }

  FILE* out = fopen(outname, "w");
  if (!out) { perror("fopen"); return 1; }
  fprintf(out, "n,hilos,repeticion,tiempo_s\n");

  fprintf(stderr, "Procesadores lógicos disponibles: %d\n", omp_get_num_procs());

  for (int s = 0; s < N_SIZES; s++) {
    int n = SIZES[s];
    if (n > n_max) break;

    srand(42);                         /* semilla fija: mismas matrices siempre */
    int** a = alloc_matrix(n);
    int** b = alloc_matrix(n);
    int** c = alloc_matrix(n);
    fill_random(a, n);
    fill_random(b, n);

    long long ref = 0;                 /* checksum de referencia (1 hilo) */

    for (int t = 0; t < N_THREADS; t++) {
      int th = THREADS[t];
      omp_set_num_threads(th);
      if (th > omp_get_num_procs())
        fprintf(stderr, "Aviso: %d hilos > %d procesadores lógicos (sobresuscripción)\n",
                th, omp_get_num_procs());

      for (int r = 1; r <= reps; r++) {
        double t0 = now_s();
        matrix_product(a, b, c, n);
        double dt = now_s() - t0;

        fprintf(out, "%d,%d,%d,%.6f\n", n, th, r, dt);
        fflush(out);
        fprintf(stderr, "N=%d hilos=%d rep=%d/%d  %.4f s\n", n, th, r, reps, dt);
      }

      long long cs = checksum(c, n);
      if (t == 0) ref = cs;
      else if (cs != ref)
        fprintf(stderr, "ERROR: checksum distinto con %d hilos (N=%d)\n", th, n);
    }

    free_matrix(a, n);
    free_matrix(b, n);
    free_matrix(c, n);
  }

  fclose(out);
  fprintf(stderr, "Listo -> %s\n", outname);
  return 0;
}
