#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // Validar que se pase el parámetro N desde la línea de comandos
    if (argc != 2) {
        printf("Uso: %s <N>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    if (N <= 0) {
        printf("El parámetro N debe ser un entero positivo mayor que cero.\n");
        return 1;
    }

    // Inicializar la semilla para los números aleatorios
    srand((unsigned int)time(NULL));

    int A[N][N];
    int B[N][N];
    int C[N][N];

    // Llenar las matrices A y B con números aleatorios enteros entre 0 y 1000
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = rand() % 1001;
            B[i][j] = rand() % 1001;
            C[i][j] = 0; // inicializar C en 0
        }
    }

    // Multiplicación de matrices cuadradas (C = A * B)
    clock_t start = clock();
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    // Mensaje de finalización
    printf("finalizado\n");
    printf("Tiempo de multiplicacion (N=%d): %.9f s\n", N, elapsed);

    return 0;
}
