#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

int* random_vector(int size) {
  int* vector = malloc(size * sizeof(int));
  for (int i = 0; i < size; i++) {
    vector[i] = rand() % 472;
  }
  return vector;
}

int* random_matrix_flat(int size) {
  int* matrix = malloc(size * size * sizeof(int));
  for (int i = 0; i < size * size; i++) {
    matrix[i] = rand() % 472;
  }
  return matrix;
}

/* ---------------------------------------------------------------
 * Multiplica un bloque de filas de A por la matriz B completa.
 *
 *   a_rows   : número de filas de A que tiene ESTE proceso
 *   size     : dimensión N de las matrices (N x N)
 *   a_block  : bloque local de filas de A  (a_rows * size enteros)
 *   b_flat   : matriz B completa en forma plana (size * size enteros)
 *   result   : buffer de salida para las filas resultado (a_rows * size)
 * --------------------------------------------------------------- */
void multiply_block(int a_rows, int size, int* a_block, int* b_flat, int* result) {
  for (int i = 0; i < a_rows; i++) {
    for (int j = 0; j < size; j++) {
      int sum = 0;
      for (int k = 0; k < size; k++) {
        sum += a_block[i * size + k] * b_flat[k * size + j];
      }
      result[i * size + j] = sum;
    }
  }
}

int main(int argc, char* argv[]) {

  /* --- Inicialización MPI --- */
  MPI_Init(&argc, &argv);

  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);   // ID de este proceso
  MPI_Comm_size(MPI_COMM_WORLD, &world_size); // Total de procesos

  /* ---------------------------------------------------------------
   * Obtener hostname para verificar distribución del cluster
   * --------------------------------------------------------------- */
  char hostname[256];
  gethostname(hostname, sizeof(hostname));

  printf("Rank %d of %d running on %s\n",
         rank,
         world_size,
         hostname);

  /* --- Validar argumentos --- */
  if (argc != 2) {
    if (rank == 0)
      printf("Usage: %s [matrix_size]\n", argv[0]);
    MPI_Finalize();
    return 1;
  }

  int size = atoi(argv[1]);

  /* ---------------------------------------------------------------
   * Validar que el tamaño sea divisible entre el número de procesos.
   * MPI_Scatter requiere bloques del mismo tamaño para todos.
   * --------------------------------------------------------------- */
  if (size % world_size != 0) {
    if (rank == 0)
      printf("Error: matrix_size (%d) must be divisible by number of processes (%d)\n", size, world_size);
    MPI_Finalize();
    return 1;
  }

  /* ---------------------------------------------------------------
   * Calcular cuántas filas le corresponden a cada proceso.
   * Si size=512 y world_size=4 → rows_per_proc=128
   * --------------------------------------------------------------- */
  int rows_per_proc = size / world_size;

  /* --- Buffers --- */
  int* a_flat = NULL;   // Matriz A completa (solo en raíz)
  int* b_flat = NULL;   // Matriz B completa (todos la tendrán)
  int* result = NULL;   // Resultado completo (solo en raíz al final)

  int* a_block      = malloc(rows_per_proc * size * sizeof(int)); // Bloque local de A
  int* result_block = malloc(rows_per_proc * size * sizeof(int)); // Bloque local del resultado

  /* ---------------------------------------------------------------
   * Variables para medición de tiempo con MPI_Wtime
   * --------------------------------------------------------------- */
  double start, end;

  /* ---------------------------------------------------------------
   * PROCESO RAÍZ: genera las matrices
   * --------------------------------------------------------------- */
  if (rank == 0) {
    srand(time(NULL));

    a_flat = random_matrix_flat(size);
    b_flat = random_matrix_flat(size);
    result = malloc(size * size * sizeof(int));
  } else {
    // Los demás procesos también necesitan buffer para B
    b_flat = malloc(size * size * sizeof(int));
  }

  /* ---------------------------------------------------------------
   * Sincronizar todos los procesos antes de iniciar el timer
   * --------------------------------------------------------------- */
  MPI_Barrier(MPI_COMM_WORLD);

  start = MPI_Wtime();

  /* ---------------------------------------------------------------
   * PASO 1 — MPI_Bcast: el raíz envía B completa a todos los procesos.
   * Cada proceso necesita B entera para poder multiplicar sus filas.
   * --------------------------------------------------------------- */
  MPI_Bcast(b_flat, size * size, MPI_INT, 0, MPI_COMM_WORLD);

  /* ---------------------------------------------------------------
   * PASO 2 — MPI_Scatter: el raíz reparte las filas de A.
   * Cada proceso recibe `rows_per_proc * size` enteros contiguos.
   * --------------------------------------------------------------- */
  MPI_Scatter(
    a_flat,                    // buffer origen (solo importa en raíz)
    rows_per_proc * size,      // cuántos elementos envía a cada proceso
    MPI_INT,
    a_block,                   // buffer destino local
    rows_per_proc * size,      // cuántos elementos recibe
    MPI_INT,
    0,                         // proceso raíz
    MPI_COMM_WORLD
  );

  /* ---------------------------------------------------------------
   * PASO 3 — Multiplicación local: cada proceso calcula su bloque.
   * --------------------------------------------------------------- */
  multiply_block(rows_per_proc, size, a_block, b_flat, result_block);

  /* ---------------------------------------------------------------
   * PASO 4 — MPI_Gather: el raíz recolecta todos los bloques.
   * Los bloques llegan en orden de rank, reconstruyendo la matriz.
   * --------------------------------------------------------------- */
  MPI_Gather(
    result_block,              // buffer local a enviar
    rows_per_proc * size,      // cuántos elementos envía cada proceso
    MPI_INT,
    result,                    // buffer destino (solo importa en raíz)
    rows_per_proc * size,      // cuántos elementos recibe de cada uno
    MPI_INT,
    0,                         // proceso raíz
    MPI_COMM_WORLD
  );

  /* ---------------------------------------------------------------
   * Sincronizar todos los procesos antes de detener el timer
   * --------------------------------------------------------------- */
  MPI_Barrier(MPI_COMM_WORLD);

  end = MPI_Wtime();

  /* ---------------------------------------------------------------
   * PROCESO RAÍZ: imprime el tiempo total de ejecución
   * --------------------------------------------------------------- */
  if (rank == 0) {
    printf("Execution time: %f seconds\n", end - start);

    free(a_flat);
    free(result);
  }

  /* --- Liberar memoria local --- */
  free(b_flat);
  free(a_block);
  free(result_block);

  MPI_Finalize();
  return 0;
}