#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <mpi.h>

// ── Ruta en el NFS compartido ──────────────────────────────────────────────
#define B_NFS_PATH "/nfs/shared/matrix_b.bin"

int* random_vector(int size) {
  int* vector = malloc(size * sizeof(int));
  for (int i = 0; i < size; i++) vector[i] = rand() % 472;
  return vector;
}

int* random_matrix_flat(int size) {
  int* matrix = malloc(size * size * sizeof(int));
  for (int i = 0; i < size * size; i++) matrix[i] = rand() % 472;
  return matrix;
}

void multiply_block(int a_rows, int size, int* a_block, int* b_flat, int* result) {
  for (int i = 0; i < a_rows; i++) {
    for (int j = 0; j < size; j++) {
      int sum = 0;
      for (int k = 0; k < size; k++)
        sum += a_block[i * size + k] * b_flat[k * size + j];
      result[i * size + j] = sum;
    }
  }
}

/* ---------------------------------------------------------------
 * Escribe la matriz B en el NFS (solo la llama el rank 0)
 * --------------------------------------------------------------- */
void write_matrix_nfs(const char* path, int* matrix, int size) {
  FILE* f = fopen(path, "wb");
  if (!f) { perror("fopen write"); MPI_Abort(MPI_COMM_WORLD, 1); }
  fwrite(matrix, sizeof(int), size * size, f);
  fclose(f);
}

/* ---------------------------------------------------------------
 * Lee la matriz B desde el NFS (la llaman todos los procesos)
 * --------------------------------------------------------------- */
void read_matrix_nfs(const char* path, int* matrix, int size) {
  FILE* f = fopen(path, "rb");
  if (!f) { perror("fopen read"); MPI_Abort(MPI_COMM_WORLD, 1); }
  fread(matrix, sizeof(int), size * size, f);
  fclose(f);
}

int main(int argc, char* argv[]) {

  MPI_Init(&argc, &argv);

  int rank, world_size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  char hostname[256];
  gethostname(hostname, sizeof(hostname));
  printf("Rank %d of %d running on %s\n", rank, world_size, hostname);

  if (argc != 2) {
    if (rank == 0) printf("Usage: %s [matrix_size]\n", argv[0]);
    MPI_Finalize();
    return 1;
  }

  int size = atoi(argv[1]);

  if (size % world_size != 0) {
    if (rank == 0)
      printf("Error: matrix_size (%d) must be divisible by number of processes (%d)\n",
             size, world_size);
    MPI_Finalize();
    return 1;
  }

  int rows_per_proc = size / world_size;

  int* a_flat       = NULL;
  int* b_flat       = malloc(size * size * sizeof(int)); // todos reservan buffer para B
  int* result       = NULL;
  int* a_block      = malloc(rows_per_proc * size * sizeof(int));
  int* result_block = malloc(rows_per_proc * size * sizeof(int));

  double start, end;

  /* ---------------------------------------------------------------
   * PROCESO RAÍZ: genera A y B, escribe B en el NFS
   * --------------------------------------------------------------- */
  if (rank == 0) {
    srand(time(NULL));
    a_flat = random_matrix_flat(size);
    b_flat = random_matrix_flat(size);
    result = malloc(size * size * sizeof(int));

    // ── NUEVO: escribe B en el NFS antes de la barrera ────────────
    write_matrix_nfs(B_NFS_PATH, b_flat, size);
  }

  /* ---------------------------------------------------------------
   * Barrera: garantiza que B ya está escrita antes de que
   * cualquier worker intente leerla del NFS
   * --------------------------------------------------------------- */
  MPI_Barrier(MPI_COMM_WORLD);

  start = MPI_Wtime();

  /* ---------------------------------------------------------------
   * TODOS los procesos leen B desde el NFS
   * (sustituye completamente al MPI_Bcast anterior)
   * --------------------------------------------------------------- */
  read_matrix_nfs(B_NFS_PATH, b_flat, size);

  /* ---------------------------------------------------------------
   * MPI_Scatter: reparte filas de A (sin cambios)
   * --------------------------------------------------------------- */
  MPI_Scatter(
    a_flat, rows_per_proc * size, MPI_INT,
    a_block, rows_per_proc * size, MPI_INT,
    0, MPI_COMM_WORLD
  );

  /* ---------------------------------------------------------------
   * Multiplicación local (sin cambios)
   * --------------------------------------------------------------- */
  multiply_block(rows_per_proc, size, a_block, b_flat, result_block);

  /* ---------------------------------------------------------------
   * MPI_Gather: recolecta resultados (sin cambios)
   * --------------------------------------------------------------- */
  MPI_Gather(
    result_block, rows_per_proc * size, MPI_INT,
    result, rows_per_proc * size, MPI_INT,
    0, MPI_COMM_WORLD
  );

  MPI_Barrier(MPI_COMM_WORLD);
  end = MPI_Wtime();

  if (rank == 0) {
    printf("Execution time: %f seconds\n", end - start);
    free(a_flat);
    free(result);
  }

  free(b_flat);
  free(a_block);
  free(result_block);

  MPI_Finalize();
  return 0;
}
