#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

double f (double x) {
  return (-x) * (x + 3) * exp(x);
}

double known_u (double x) {
  return x * (x - 1) * exp(x);
}

double* initialize_points(int n) {
  double* points = malloc(n * sizeof(double));
  for (int i = 0; i < n; i++)
    points[i] = 0.0;
  return points;
}

void update_points (double* points, double* points_new, int n, double h) {
  for (int j = 1; j < (n - 1); j++) {
    double x_j = j * h;
    points_new[j] = ((h * h) * f(x_j) + points[j - 1] + points[j + 1]) / 2.0;
  }
}

/*
 * Calcula el residual RMS segun el articulo (Seccion 9):
 *
 *   r^j = A * u^j - f
 *   criterio: ||r^j|| / sqrt(n) <= tol
 *
 * Para cada nodo interior j (1 <= j <= n-2), la fila de A*u da:
 *   (A*u)[j] = (-u[j-1] + 2*u[j] - u[j+1]) / h^2
 *
 * Por tanto el residual en ese nodo es:
 *   r[j] = (-u[j-1] + 2*u[j] - u[j+1]) / h^2 - f(x_j)
 *
 * Los nodos de frontera siempre tienen u=0 (Dirichlet), su residual es 0.
 */
double residual_rms (double* u, int n, double h) {
  double sum = 0.0;
  for (int j = 1; j < n - 1; j++) {
    double x_j = j * h;
    double Au_j = (-u[j - 1] + 2.0 * u[j] - u[j + 1]) / (h * h);
    double r_j  = Au_j - f(x_j);
    sum += r_j * r_j;
  }
  return sqrt(sum / n);
}

double* jacobi (int k, int* out_iters) {
  int n = round(pow(2, k) + 1.0);
  double h = 1.0 / (double) (n - 1);
  double* points     = initialize_points(n);
  double* points_new = initialize_points(n);

  double tol = 1e-6;  // Tolerancia RMS del residual (Seccion 10 del articulo)

  int iter = 0;
  while (1) {
    update_points(points, points_new, n, h);

    double* tmp = points;
    points     = points_new;
    points_new = tmp;

    iter++;

    double rms = residual_rms(points, n, h);
    if (rms <= tol) {
      fprintf(stderr, "Converged at iteration %d  (RMS residual = %.6e)\n\n", iter, rms);
      break;
    }
  }

  *out_iters = iter;
  free(points_new);
  return points;
}

int main (int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s [k_value]\n", argv[0]);
    return 1;
  }

  int k = atoi(argv[1]);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int iters = 0;
  double* points = jacobi(k, &iters);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double time_taken =
    (end.tv_sec  - start.tv_sec) +
    (end.tv_nsec - start.tv_nsec) / 1e9;

  int n = round(pow(2, k) + 1.0);

  // stdout: tiempo iteraciones nodos  (para que el script los capture)
  printf("%f %d %d\n", time_taken, iters, n);

  // El resto va a stderr
  double h = 1.0 / (double) (n - 1);
  fprintf(stderr, "\n  I       x         U_Jacobi      U_Exact\n");
  for (int j = 0; j < n; j++) {
    double x_j     = j * h;
    double u_exact = known_u(x_j);
    fprintf(stderr, "%4d  %8.4f  %12.6f  %12.6f\n", j + 1, x_j, points[j], u_exact);
  }

  free(points);
  return 0;
}
