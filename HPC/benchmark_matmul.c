/*
 * Benchmark de multiplicación de matrices paralela con pthreads (sin OpenMP).
 *
 * Prueba N = {500, 1000, 2000, 4000, 8000} x Hilos = {1,2,4,8,16},
 * 10 repeticiones cada combinación, y escribe cada tiempo en
 * resultados.csv (formato: N,Hilos,Repeticion,Tiempo_s).
 *
 * Compilar:  gcc -O2 -Wall -pthread -o benchmark benchmark_matmul.c
 * Correr todo:        ./benchmark
 * Correr solo un N:   ./benchmark 2000     (agrega filas a resultados.csv)
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct {
    int **A, **B, **C;
    int N;
    int row_start;  /* inclusive */
    int row_end;    /* exclusive */
} Task;

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

double wtime(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* Cada hilo calcula un bloque contiguo de filas de C */
void *worker(void *arg) {
    Task *t = (Task *)arg;
    int N = t->N;
    for (int i = t->row_start; i < t->row_end; i++) {
        for (int j = 0; j < N; j++) {
            int sum = 0;
            for (int k = 0; k < N; k++) {
                sum += t->A[i][k] * t->B[k][j];
            }
            t->C[i][j] = sum;
        }
    }
    return NULL;
}

/* Equivalente a schedule(static): bloques contiguos de tamaño ~N/nt */
void multiply(int **A, int **B, int **C, int N, int nt) {
    pthread_t threads[64];
    Task tasks[64];
    int base = N / nt, rem = N % nt, start = 0;

    for (int t = 0; t < nt; t++) {
        int rows = base + (t < rem ? 1 : 0);
        tasks[t] = (Task){A, B, C, N, start, start + rows};
        start += rows;
    }

    /* El hilo principal ejecuta el último bloque; se crean nt-1 hilos */
    for (int t = 0; t < nt - 1; t++) {
        if (pthread_create(&threads[t], NULL, worker, &tasks[t]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    worker(&tasks[nt - 1]);
    for (int t = 0; t < nt - 1; t++) pthread_join(threads[t], NULL);
}

int main(int argc, char *argv[]) {
    int all_sizes[]   = {500, 1000, 2000, 4000, 8000};
    int all_threads[] = {1, 2, 4, 8, 16};
    const int N_THREAD_OPTS = 5;
    const int REPS = 10;

    int *sizes = all_sizes;
    int nsizes = 5;
    int one_size = 0;

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

    FILE *csv = fopen("resultados.csv", one_size ? "a" : "w");
    if (!csv) {
        perror("No se pudo abrir resultados.csv");
        return 1;
    }
    if (!one_size) {
        fprintf(csv, "N,Hilos,Repeticion,Tiempo_s\n");
    }

    for (int s = 0; s < nsizes; s++) {
        int N = sizes[s];
        printf("\n=== N = %d ===\n", N);

        int **A = alloc_matrix(N);
        int **B = alloc_matrix(N);
        int **C = alloc_matrix(N);
        fill_random(A, N);
        fill_random(B, N);

        for (int t = 0; t < N_THREAD_OPTS; t++) {
            int nt = all_threads[t];
            if (nt > N) nt = N;
            double total = 0.0;

            for (int r = 1; r <= REPS; r++) {
                double t0 = wtime();
                multiply(A, B, C, N, nt);
                double t1 = wtime();
                double dt = t1 - t0;
                total += dt;

                fprintf(csv, "%d,%d,%d,%.6f\n", N, nt, r, dt);
                fflush(csv);
                printf("  N=%d Hilos=%2d Rep=%2d/%d  t=%.6f s\n",
                       N, nt, r, REPS, dt);
            }
            printf(" -> PROMEDIO N=%d Hilos=%2d: %.6f s\n",
                   N, nt, total / REPS);
        }

        free_matrix(A, N);
        free_matrix(B, N);
        free_matrix(C, N);
    }

    fclose(csv);
    printf("\nListo. Resultados en resultados.csv\n");
    return 0;
}
