/*
 * Benchmark de multiplicación de matrices paralela con OpenMP.
 *
 * Prueba N = {500, 1000, 2000, 4000, 8000} x Hilos = {1,2,4,8,16},
 * 10 repeticiones cada combinación, y escribe cada tiempo en
 * resultados.csv (formato: N,Hilos,Repeticion,Tiempo_s) para
 * graficar/promediar después en Excel.
 *
 * Compilar:  gcc -O2 -Wall -fopenmp -o benchmark benchmark_matmul.c
 * Correr todo:        ./benchmark
 * Correr solo un N:   ./benchmark 2000     (agrega filas a resultados.csv)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

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

void multiply(int **A, int **B, int **C, int N) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int sum = 0;
            for (int k = 0; k < N; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

int main(int argc, char *argv[]) {
    int all_sizes[]   = {500, 1000, 2000, 4000, 8000};
    int all_threads[] = {1, 2, 4, 8, 16};
    const int N_THREAD_OPTS = 5;
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
            omp_set_num_threads(nt);
            double total = 0.0;

            for (int r = 1; r <= REPS; r++) {
                double t0 = omp_get_wtime();
                multiply(A, B, C, N);
                double t1 = omp_get_wtime();
                double dt = t1 - t0;
                total += dt;

                fprintf(csv, "%d,%d,%d,%.6f\n", N, nt, r, dt);
                fflush(csv); /* guarda en disco de inmediato: si se corta,
                                no se pierde lo ya corrido */
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
