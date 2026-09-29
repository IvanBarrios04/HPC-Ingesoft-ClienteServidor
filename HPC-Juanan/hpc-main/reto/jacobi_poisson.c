/*
 * ============================================================
 *  jacobi_poisson.c
 *  Jacobi Iterative Solution of Poisson's Equation in 1D
 *
 *  Basado en:
 *    John Burkardt, "Jacobi Iterative Solution of Poisson's
 *    Equation in 1D", Florida State University, 2011.
 *
 *  Problema:   -u''(x) = f(x),  x en [0, 1]
 *  Condiciones de frontera Dirichlet (aleatorias, reproducibles):
 *              u(0) = ua,  u(1) = ub
 *  Forcing function:
 *              f(x) = -x*(x+3)*exp(x)
 *  Solución exacta (cuando ua=ub=0):
 *              u_exact(x) = x*(x-1)*exp(x)
 *
 *  Uso:
 *    gcc -O2 -o jacobi_poisson jacobi_poisson.c -lm
 *    ./jacobi_poisson <k>
 *
 *  Ejemplo:
 *    ./jacobi_poisson 5
 *
 *  Parámetros:
 *    k : índice de malla  →  n = 2^k + 1 nodos
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/* ============================================================
 *  CONSTANTES GLOBALES
 * ============================================================ */
#define TOL        1.0e-6   /* Tolerancia RMS del residual           */
#define DOMAIN_A   0.0      /* Extremo izquierdo del dominio         */
#define DOMAIN_B   1.0      /* Extremo derecho del dominio           */

/* ============================================================
 *  PROTOTIPOS DE FUNCIONES
 * ============================================================ */
double  forcing_function (double x);
double  exact_solution   (double x);
void    build_grid       (int n, double a, double b, double *x, double *h_out);
void    build_system     (int n, const double *x, double h,
                          double ua, double ub, double **A, double *b);
double  residual_rms     (int n, double **A, const double *u, const double *b);
double  iterate_diff     (int n, const double *u_new, const double *u_old);
int     jacobi_solve     (int n, double **A, const double *b,
                          double *u, double tol, bool verbose);
double **alloc_matrix    (int n);
void    free_matrix      (double **A, int n);
void    mat_vec_mul      (int n, double **A, const double *v, double *result);
void    print_usage      (const char *prog);
void    print_solution   (int n, const double *x, const double *u,
                          double ua, double ub);

/* ============================================================
 *  MAIN
 * ============================================================ */
int main(int argc, char *argv[])
{
    /* ── Parseo de argumentos de línea de comandos ───────────── */
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int k = atoi(argv[1]);

    if (k < 0 || k > 20) {
        fprintf(stderr, "Error: k debe estar en [0, 20]. Recibido: %d\n", k);
        return EXIT_FAILURE;
    }

    /* ── Cálculo del número de nodos (fórmula del documento) ── */
    /*    n_k = 2^k + 1                                          */
    int n = (1 << k) + 1;   /* Equivalente a 2^k + 1            */

    /* ── Condiciones de frontera Dirichlet (documento, sección 2) ─ */
    /*    u(0) = 0,  u(1) = 0                                        */
    double ua = 0.0;
    double ub = 0.0;

    /* ── Encabezado de ejecución ─────────────────────────────── */
    printf("==========================================================\n");
    printf("  Jacobi Iterative Solution of Poisson's Equation in 1D  \n");
    printf("==========================================================\n");
    printf("  Parámetros de entrada:\n");
    printf("    k         = %d\n",   k);
    printf("    tolerancia RMS = %.2e\n", TOL);
    printf("----------------------------------------------------------\n");
    printf("  Geometría:\n");
    printf("    Número de nodos : n = 2^%d + 1 = %d\n", k, n);

    /* ── Reserva de memoria dinámica ─────────────────────────── */
    double  *x = (double *)malloc(n * sizeof(double));
    double  *b = (double *)malloc(n * sizeof(double));
    double  *u = (double *)calloc(n, sizeof(double)); /* u^0 = 0  */

    if (!x || !b || !u) {
        fprintf(stderr, "Error: no se pudo reservar memoria para vectores.\n");
        return EXIT_FAILURE;
    }

    double **A = alloc_matrix(n);
    if (!A) {
        fprintf(stderr, "Error: no se pudo reservar memoria para la matriz A.\n");
        free(x); free(b); free(u);
        return EXIT_FAILURE;
    }

    /* ── Construcción de la malla uniforme ───────────────────── */
    double h;
    build_grid(n, DOMAIN_A, DOMAIN_B, x, &h);

    printf("    Paso de malla   : h = (b-a)/2^k = %.10f\n", h);
    printf("    u(a=0)          : ua = %.1f  (Dirichlet)\n", ua);
    printf("    u(b=1)          : ub = %.1f  (Dirichlet)\n", ub);
    printf("----------------------------------------------------------\n");

    /* ── Construcción del sistema lineal A*u = b ─────────────── */
    build_system(n, x, h, ua, ub, A, b);

    /* ── Resolución con el método de Jacobi ──────────────────── */
    printf("  Ejecutando iteración de Jacobi...\n\n");
    printf("  %8s  %14s  %14s\n", "Iter", "Residual RMS", "||u_new-u_old||");
    printf("  %8s  %14s  %14s\n", "--------",
           "--------------", "---------------");

    clock_t t_start = clock();

    int iter = jacobi_solve(n, A, b, u, TOL, true);

    clock_t t_end = clock();
    double elapsed = (double)(t_end - t_start) / CLOCKS_PER_SEC;

    /* ── Métricas finales ────────────────────────────────────── */
    double rms_final = residual_rms(n, A, u, b);

    printf("\n==========================================================\n");
    printf("  RESULTADOS\n");
    printf("==========================================================\n");
    printf("  k                              = %d\n",    k);
    printf("  Número de nodos (n)            = %d\n",    n);
    printf("  Paso de malla (h)              = %.10f\n", h);
    printf("  Condiciones de frontera        : u(0) = u(1) = 0\n");
    printf("  Iteraciones realizadas         = %d\n",    iter);
    printf("  Residual RMS final             = %.6e\n",  rms_final);
    printf("  Tiempo de ejecución            = %.6f s\n", elapsed);
    printf("----------------------------------------------------------\n");

    /* ── Tabla de solución (solo para n <= 65 para no saturar) ─ */
    if (n <= 65) {
        print_solution(n, x, u, ua, ub);
    } else {
        printf("  (Tabla de solución omitida: n=%d > 65)\n", n);
    }

    printf("==========================================================\n");
    printf("  Ejemplo de compilación y ejecución:\n");
    printf("    gcc -O2 -o jacobi_poisson jacobi_poisson.c -lm\n");
    printf("    ./jacobi_poisson %d\n", k);
    printf("==========================================================\n");

    /* ── Liberación de memoria ───────────────────────────────── */
    free(x);
    free(b);
    free(u);
    free_matrix(A, n);

    return EXIT_SUCCESS;
}

/* ============================================================
 *  forcing_function
 *  f(x) = -x*(x+3)*exp(x)
 *  Fuente: documento, sección 2.
 * ============================================================ */
double forcing_function(double x)
{
    return -x * (x + 3.0) * exp(x);
}

/* ============================================================
 *  exact_solution
 *  u_exact(x) = x*(x-1)*exp(x)
 *  Válida solo cuando ua = ub = 0.
 *  Fuente: documento, sección 2.
 * ============================================================ */
double exact_solution(double x)
{
    return x * (x - 1.0) * exp(x);
}

/* ============================================================
 *  build_grid
 *  Construye la malla uniforme de n nodos en [a, b].
 *
 *  Fórmula (documento, sección 3):
 *    x_j = ((n-j)*a + (j-1)*b) / (n-1),  j = 1..n  (base 1)
 *  En base 0:
 *    x[i] = a + i*h,  i = 0..n-1
 *    h = (b-a) / (n-1)
 * ============================================================ */
void build_grid(int n, double a, double b, double *x, double *h_out)
{
    double h = (b - a) / (double)(n - 1);
    *h_out = h;

    for (int i = 0; i < n; i++) {
        x[i] = a + i * h;
    }
    /* Forzar exactitud en los extremos para evitar redondeo */
    x[0]     = a;
    x[n - 1] = b;
}

/* ============================================================
 *  build_system
 *  Construye la matriz A (n×n) y el vector b de tamaño n.
 *
 *  Estructura de A (documento, sección 6):
 *
 *  Fila 0 (frontera izq):   A[0][0] = 1,  resto = 0
 *  Fila i (interior):
 *      A[i][i-1] = -1/h^2
 *      A[i][i]   =  2/h^2
 *      A[i][i+1] = -1/h^2
 *  Fila n-1 (frontera der): A[n-1][n-1] = 1, resto = 0
 *
 *  Vector b:
 *      b[0]   = ua
 *      b[i]   = f(x[i])       para i = 1..n-2
 *      b[n-1] = ub
 *
 *  NOTA: No se escala por h^2 (ver documento, sección 9 y código
 *  MATLAB). Escalar causaría que la tolerancia RMS se satisfaga
 *  artificialmente con soluciones cada vez peores al crecer n.
 * ============================================================ */
void build_system(int n, const double *x, double h,
                  double ua, double ub,
                  double **A, double *b)
{
    double h2 = h * h;
    double inv_h2 = 1.0 / h2;

    /* Inicializar A en cero */
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            A[i][j] = 0.0;

    /* ── Fila 0: condición de frontera izquierda ────────────── */
    A[0][0] = 1.0;
    b[0]    = ua;

    /* ── Filas interiores: esquema de diferencias finitas ────── */
    /*    (-u_{i-1} + 2*u_i - u_{i+1}) / h^2 = f(x_i)           */
    for (int i = 1; i < n - 1; i++) {
        A[i][i - 1] = -inv_h2;   /* coeficiente de u_{i-1}       */
        A[i][i]     =  2.0 * inv_h2; /* coeficiente de u_i        */
        A[i][i + 1] = -inv_h2;   /* coeficiente de u_{i+1}       */
        b[i]        = forcing_function(x[i]);
    }

    /* ── Fila n-1: condición de frontera derecha ────────────── */
    A[n - 1][n - 1] = 1.0;
    b[n - 1]        = ub;
}

/* ============================================================
 *  residual_rms
 *  Calcula la norma RMS del residual:
 *      r = A*u - b
 *      rms = ||r||_2 / sqrt(n)
 *
 *  Fuente: documento, sección 9 y 10.
 * ============================================================ */
double residual_rms(int n, double **A, const double *u, const double *b)
{
    double *r = (double *)malloc(n * sizeof(double));
    if (!r) return -1.0;

    /* r = A*u - b */
    mat_vec_mul(n, A, u, r);
    for (int i = 0; i < n; i++)
        r[i] -= b[i];

    /* ||r||_2 / sqrt(n) */
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += r[i] * r[i];

    free(r);
    return sqrt(sum / (double)n);
}

/* ============================================================
 *  iterate_diff
 *  Calcula la diferencia entre iteraciones consecutivas:
 *      diff = ||u_new - u_old||_2
 *
 *  Útil para análisis adicional (no es el criterio de parada).
 *  Fuente: documento, sección 9.
 * ============================================================ */
double iterate_diff(int n, const double *u_new, const double *u_old)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double d = u_new[i] - u_old[i];
        sum += d * d;
    }
    return sqrt(sum);
}

/* ============================================================
 *  jacobi_solve
 *  Implementa la iteración de Jacobi para A*u = b.
 *
 *  Fórmula de actualización (documento, sección 8):
 *      u_new[i] = (b[i] - sum_{j!=i} A[i][j]*u_old[j]) / A[i][i]
 *
 *  Forma vectorial equivalente:
 *      u_new = D^{-1} * (b - (L+U)*u_old)
 *
 *  Criterio de parada (documento, sección 9):
 *      rms = ||A*u - b||_2 / sqrt(n) <= tol
 *
 *  Parámetros:
 *    n        : tamaño del sistema
 *    A        : matriz del sistema (n×n)
 *    b        : vector del lado derecho
 *    u        : vector solución (entrada: aproximación inicial;
 *               salida: solución aproximada)
 *    tol      : tolerancia para el residual RMS
 *    max_iter : número máximo de iteraciones permitidas
 *    verbose  : imprime progreso cada cierto número de iteraciones
 *
 *  Retorna: número de iteraciones realizadas
 * ============================================================ */
int jacobi_solve(int n, double **A, const double *b,
                 double *u, double tol, bool verbose)
{
    /* Reserva de vectores auxiliares */
    double *u_old  = (double *)malloc(n * sizeof(double));
    double *diag_A = (double *)malloc(n * sizeof(double));

    if (!u_old || !diag_A) {
        fprintf(stderr, "Error: no se pudo reservar memoria en jacobi_solve.\n");
        free(u_old); free(diag_A);
        return -1;
    }

    /* Extraer la diagonal de A (para la división en cada iteración) */
    for (int i = 0; i < n; i++)
        diag_A[i] = A[i][i];

    /* Frecuencia de impresión: cada 200 iteraciones */
    int print_freq = 200;

    int   it  = 0;
    double rms = 0.0, diff = 0.0;

    /* Bucle infinito: la única condición de parada es rms <= tol */
    while (1)
    {
        /* ── Guardar iterado anterior ───────────────────────── */
        memcpy(u_old, u, n * sizeof(double));

        /* ── Actualización de Jacobi ────────────────────────── */
        /*    u_new[i] = (b[i] - sum_{j≠i} A[i][j]*u_old[j])   */
        /*               / A[i][i]                               */
        for (int i = 0; i < n; i++) {
            double sigma = 0.0;
            for (int j = 0; j < n; j++) {
                if (j != i)
                    sigma += A[i][j] * u_old[j];
            }
            u[i] = (b[i] - sigma) / diag_A[i];
        }

        it++;

        /* ── Cálculo del residual RMS (único criterio de parada) */
        rms  = residual_rms(n, A, u, b);

        /* ── Diferencia entre iteraciones consecutivas ───────── */
        diff = iterate_diff(n, u, u_old);

        /* ── Salida periódica ────────────────────────────────── */
        if (verbose && (it % print_freq == 0 || it == 1)) {
            printf("  %8d  %14.6e  %14.6e\n", it, rms, diff);
        }

        /* ── Criterio de parada: residual RMS ≤ tol ─────────── */
        if (rms <= tol)
            break;
    }

    /* Imprimir la última iteración si no fue impresa */
    if (verbose && (it % print_freq != 0) && it > 1) {
        printf("  %8d  %14.6e  %14.6e\n", it, rms, diff);
    }

    /* Guardar métricas finales para el caller (vía stdout) */
    printf("\n  Residual RMS final          : %.6e\n", rms);
    printf("  Diferencia final ||u_n-u_{n-1}|| : %.6e\n", diff);

    free(u_old);
    free(diag_A);

    return it;
}

/* ============================================================
 *  mat_vec_mul
 *  Multiplica la matriz A (n×n) por el vector v,
 *  almacenando el resultado en 'result'.
 *      result = A * v
 * ============================================================ */
void mat_vec_mul(int n, double **A, const double *v, double *result)
{
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++)
            sum += A[i][j] * v[j];
        result[i] = sum;
    }
}

/* ============================================================
 *  alloc_matrix
 *  Reserva memoria dinámica para una matriz de doubles n×n.
 *  Retorna NULL si falla.
 * ============================================================ */
double **alloc_matrix(int n)
{
    double **A = (double **)malloc(n * sizeof(double *));
    if (!A) return NULL;

    for (int i = 0; i < n; i++) {
        A[i] = (double *)calloc(n, sizeof(double));
        if (!A[i]) {
            /* Liberar lo ya reservado antes de retornar NULL */
            for (int j = 0; j < i; j++) free(A[j]);
            free(A);
            return NULL;
        }
    }
    return A;
}

/* ============================================================
 *  free_matrix
 *  Libera la memoria de una matriz n×n reservada con alloc_matrix.
 * ============================================================ */
void free_matrix(double **A, int n)
{
    if (!A) return;
    for (int i = 0; i < n; i++)
        free(A[i]);
    free(A);
}

/* ============================================================
 *  print_solution
 *  Imprime la tabla de la solución numérica (para n <= 65).
 *  Si ua=ub=0, también muestra la solución exacta.
 * ============================================================ */
void print_solution(int n, const double *x, const double *u,
                    double ua, double ub)
{
    bool show_exact = (fabs(ua) < 1e-12 && fabs(ub) < 1e-12);

    printf("\n");
    if (show_exact) {
        printf("  %4s  %10s  %14s  %14s\n",
               "i", "x_i", "u_Jacobi", "u_exact");
        printf("  %4s  %10s  %14s  %14s\n",
               "----", "----------", "--------------", "--------------");
    } else {
        printf("  %4s  %10s  %14s\n", "i", "x_i", "u_Jacobi");
        printf("  %4s  %10s  %14s\n", "----", "----------", "--------------");
    }

    for (int i = 0; i < n; i++) {
        if (show_exact) {
            printf("  %4d  %10.6f  %14.6e  %14.6e\n",
                   i + 1, x[i], u[i], exact_solution(x[i]));
        } else {
            printf("  %4d  %10.6f  %14.6e\n", i + 1, x[i], u[i]);
        }
    }
}

/* ============================================================
 *  print_usage
 *  Imprime las instrucciones de uso del programa.
 * ============================================================ */
void print_usage(const char *prog)
{
    fprintf(stderr,
        "Uso: %s <k>\n"
        "\n"
        "  k : índice de malla. Número de nodos = 2^k + 1\n"
        "      Rango recomendado: 1..15\n"
        "\n"
        "Las condiciones de frontera son u(0) = u(1) = 0,\n"
        "igual que en el artículo de Burkardt (2011).\n"
        "El método de Jacobi itera hasta alcanzar RMS = 1e-6.\n"
        "\n"
        "Ejemplo:\n"
        "  gcc -O2 -o jacobi_poisson jacobi_poisson.c -lm\n"
        "  ./jacobi_poisson 5\n",
        prog
    );
}

/*
 * ============================================================
 *  NOTAS DE IMPLEMENTACIÓN
 * ============================================================
 *
 *  1. MALLA (build_grid):
 *     La malla uniforme tiene n = 2^k + 1 nodos en [0,1].
 *     El espaciado es h = 1/(n-1). Los índices son base 0.
 *
 *  2. MATRIZ A (build_system):
 *     La matriz es tridiagonal: diagonal principal = 2/h^2,
 *     subdiagonal y superdiagonal = -1/h^2. Las filas de
 *     frontera tienen solo un 1 en la diagonal (identidad),
 *     lo que impone directamente las condiciones Dirichlet.
 *     NO se escala por h^2 para evitar problemas de tolerancia
 *     al comparar grids de distintos tamaños.
 *
 *  3. VECTOR b (build_system):
 *     b[0] = ua, b[n-1] = ub, b[i] = f(x_i) en el interior.
 *
 *  4. RESIDUAL RMS (residual_rms):
 *     rms = ||A*u - b||_2 / sqrt(n)
 *     Es el criterio de parada principal (documento, sec. 9).
 *
 *  5. JACOBI (jacobi_solve):
 *     Actualización: u_new[i] = (b[i] - Σ_{j≠i} A[i][j]*u_old[j]) / A[i][i]
 *     El iterado inicial es el vector cero.
 *     El bucle es infinito: se detiene únicamente cuando rms <= tol = 1e-6.
 *     No existe límite de iteraciones.
 *
 *  6. FRONTERA:
 *     u(0) = u(1) = 0, igual que en el artículo (sección 2).
 *     La solución exacta u(x) = x*(x-1)*exp(x) es válida y
 *     se muestra en la tabla de resultados para comparación.
 *
 *  7. MEMORIA:
 *     Toda la memoria dinámica (A, x, b, u) es liberada al final.
 *     Se usan malloc/calloc con comprobación de NULL.
 * ============================================================
 */
