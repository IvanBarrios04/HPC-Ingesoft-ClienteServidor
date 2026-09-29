/*
 * Benchmark de multiplicación de matrices paralela con PROCESOS (fork).
 *
 * Version equivalente a Multiplicación_matrices_hilos.c pero usando
 * procesos en lugar de hilos: cada proceso hijo tiene su propia memoria
 * (copy-on-write de A y B), y solo la matriz C se coloca en memoria
 * compartida (mmap) para que los hijos puedan escribir su bloque de filas
 * y el padre pueda leer el resultado final.
 *
 * Prueba N = {500, 1000, 2000, 4000, 8000} x Procesos = {1,2,4,8,16},
 * 10 repeticiones cada combinación, y escribe cada tiempo en
 * resultados.csv (formato: N,Procesos,Repeticion,Tiempo_s) para
 * graficar/promediar después en Excel.
 *
 * Requiere un sistema POSIX (Linux/WSL/macOS): usa fork(), mmap() y wait().
 * No compila en Windows/MSVC porque fork() no existe en esa plataforma.
 *
 * Compilar:  gcc -O2 -Wall -o benchmark_procesos MultiplicacionMatrices3.c
 * Correr todo:        ./benchmark_procesos
 * Correr solo un N:   ./benchmark_procesos 2000     (agrega filas a resultados.csv)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

int **alloc_matrix(int N) {
    int **M = malloc(N * sizeof(int *));
    for (int i = 0; i < N; i++) M[i] = malloc(N * sizeof(int));
    return M;
}

void free_matrix(int **M, int N) {
    for (int i = 0; i < N; i++) free(M[i]);
    free(M);
}

void fill_random(int **M, int N) {
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            M[i][j] = rand() % 1001;
}

/* Reserva la matriz C en memoria compartida (mmap) para que los procesos
   hijos puedan escribir directamente su bloque de filas. */
int **alloc_shared_matrix(int N) {
    int *data = mmap(NULL, (size_t)N * N * sizeof(int),
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    int **M = malloc(N * sizeof(int *));
    for (int i = 0; i < N; i++) M[i] = data + (size_t)i * N;
    return M;
}

void free_shared_matrix(int **M, int N) {
    munmap(M[0], (size_t)N * N * sizeof(int));
    free(M);
}

double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* Multiplica las filas [start, end) de A por B, guardando en C. */
void multiply_rows(int **A, int **B, int **C, int N, int start, int end) {
    for (int i = start; i < end; i++) {
        for (int j = 0; j < N; j++) {
            int sum = 0;
            for (int k = 0; k < N; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

/* Reparte las filas de la multiplicación entre nprocs procesos hijos. */
void multiply(int **A, int **B, int **C, int N, int nprocs) {
    if (nprocs <= 1) {
        multiply_rows(A, B, C, N, 0, N);
        return;
    }

    int rows_per_proc = N / nprocs;
    int extra = N % nprocs;
    int start = 0;
    pid_t pids[nprocs];

    for (int p = 0; p < nprocs; p++) {
        int rows = rows_per_proc + (p < extra ? 1 : 0);
        int end = start + rows;

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        if (pid == 0) {
            /* Proceso hijo: calcula su bloque de filas y termina */
            multiply_rows(A, B, C, N, start, end);
            _exit(0);
        }
        pids[p] = pid;
        start = end;
    }

    for (int p = 0; p < nprocs; p++) {
        waitpid(pids[p], NULL, 0);
    }
}

int main(int argc, char *argv[]) {
    int all_sizes[] = {500, 1000, 2000, 4000, 8000};
    int all_procs[] = {1, 2, 4, 8, 16};
    const int N_PROC_OPTS = 5;
    const int REPS = 10;

    int *sizes = all_sizes;
    int nsizes = 5;
    int one_size = 0;

    /* Uso opcional: pasar un solo N por argumento, para correr
       tamaños grandes por separado (ver explicación abajo) */
    if (argc == 2) {
        one_size = atoi(argv[1]);
        if (one_size <= 0) {
            printf("N invalido\n");
            return 1;
        }
        sizes = &one_size;
        nsizes = 1;
    }

    srand((unsigned int)time(NULL));

    /* Si se corre un solo N, se agrega (append) al CSV existente
       en vez de sobrescribirlo */
    FILE *csv = fopen("resultados.csv", one_size ? "a" : "w");
    if (!csv) {
        perror("No se pudo abrir resultados.csv");
        return 1;
    }
    if (!one_size) {
        fprintf(csv, "N,Procesos,Repeticion,Tiempo_s\n");
    }

    for (int s = 0; s < nsizes; s++) {
        int N = sizes[s];
        printf("\n=== N = %d ===\n", N);

        int **A = alloc_matrix(N);
        int **B = alloc_matrix(N);
        fill_random(A, N);
        fill_random(B, N);

        for (int t = 0; t < N_PROC_OPTS; t++) {
            int np = all_procs[t];
            int **C = alloc_shared_matrix(N);
            double total = 0.0;

            for (int r = 1; r <= REPS; r++) {
                double t0 = now_seconds();
                multiply(A, B, C, N, np);
                double t1 = now_seconds();
                double dt = t1 - t0;
                total += dt;

                fprintf(csv, "%d,%d,%d,%.6f\n", N, np, r, dt);
                fflush(csv); /* guarda en disco de inmediato: si se corta,
                                no se pierde lo ya corrido */
                printf("  N=%d Procesos=%2d Rep=%2d/%d  t=%.6f s\n",
                       N, np, r, REPS, dt);
            }
            printf(" -> PROMEDIO N=%d Procesos=%2d: %.6f s\n",
                   N, np, total / REPS);

            free_shared_matrix(C, N);
        }

        free_matrix(A, N);
        free_matrix(B, N);
    }

    fclose(csv);
    printf("\nListo. Resultados en resultados.csv\n");
    return 0;
}
