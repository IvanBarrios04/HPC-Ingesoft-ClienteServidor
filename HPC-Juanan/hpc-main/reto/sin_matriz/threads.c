#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

typedef struct {
  double** u;
  double** u_new;
  double h;
  int start;
  int end;
  int* finish;
  pthread_barrier_t* calc_barrier;
  pthread_barrier_t* swap_barrier;
} ThreadData;

double f (double x) {
  return (-x) * (x + 3) * exp(x);
}

double known_u (double x) {
  return x * (x - 1) * exp(x);
}

double* initialize_u(int n) {
  double* u = malloc(n * sizeof(double));
  for (int i = 0; i < n; i++)
    u[i] = 0.0;
  return u;
}

void update_u (double* u, double* u_new, double h, int n) {
  for (int j = 1; j < (n - 1); j++) {
    double x_j = j * h;
    u_new[j] = ((h * h) * f(x_j) + u[j - 1] + u[j + 1]) / 2.0;
  }
}

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

void* partial_jacobi (void* arg) {
  ThreadData* data = (ThreadData*) arg;

  while (1) {
    double* u_old = *(data->u);
    double* u_new = *(data->u_new);

    for (int j = data->start; j <= data->end; j++) {
      double x_j = j * data->h;
      u_new[j] = ((data->h * data->h) * f(x_j) + u_old[j-1] + u_old[j+1]) / 2.0;
    }

    pthread_barrier_wait(data->calc_barrier);
    pthread_barrier_wait(data->swap_barrier);

    if (*(data->finish)) break;
  }   
  pthread_exit(NULL);
}

double* jacobi_n_threads (int k, int num_threads, int* out_iters) {
  pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
  ThreadData* data = malloc(num_threads * sizeof(ThreadData));

  pthread_barrier_t calc_barrier;
  pthread_barrier_t swap_barrier;

  pthread_barrier_init(&calc_barrier, NULL, num_threads + 1);
  pthread_barrier_init(&swap_barrier, NULL, num_threads + 1);

  int n = round(pow(2, k) + 1.0);
  double h = 1.0 / (double) (n - 1);
  double* u = initialize_u(n);
  double* u_new = initialize_u(n);

  int internal_nodes = n - 2;
  int base = internal_nodes / num_threads;
  int remainder = internal_nodes % num_threads;

  int finish = 0;
  int iter = 0;
  for (int i = 0; i < num_threads; i++) {
    int start = 1 + i * base;
    int end = start + base - 1;
    if (i == num_threads - 1) end += remainder;

    data[i] = (ThreadData) {&u, &u_new, h, start, end, &finish, &calc_barrier, &swap_barrier};
    pthread_create(&threads[i], NULL, partial_jacobi, &data[i]);
  }

  double tol = 1e-6;
  while (1) {
    pthread_barrier_wait(&calc_barrier);

    double* tmp = u;
    u = u_new;
    u_new = tmp;

    double rms = residual_rms(u, n, h);
    iter++;

    if (rms <= tol) finish = 1;

    pthread_barrier_wait(&swap_barrier);

    if (finish) break;
  }

  for (int t = 0; t < num_threads; t++)
    pthread_join(threads[t], NULL);

  pthread_barrier_destroy(&calc_barrier);
  pthread_barrier_destroy(&swap_barrier);

  *out_iters = iter;
  free(data);
  free(threads);
  free(u_new);
  return u;
}

int main (int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s [k_value] [num_hilos]\n", argv[0]);
    return 1;
  }

  int k = atoi(argv[1]);
  int num_threads = atoi(argv[2]);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int iters = 0;
  double* u = jacobi_n_threads(k, num_threads, &iters);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double time_taken =
    (end.tv_sec  - start.tv_sec) +
    (end.tv_nsec - start.tv_nsec) / 1e9;

  int n = (int)round(pow(2, k) + 1.0);
  printf("%f %d %d\n", time_taken, iters, n);

  // int n = round(pow(2, k) + 1.0);
  // double h = 1.0 / (double) (n - 1);
  // printf("\n  I       x         U_Jacobi      U_Exact\n");
  // for (int j = 0; j < n; j++) {
  //   double x_j     = j * h;
  //   double u_exact = known_u(x_j);
  //   printf("%4d  %8.4f  %12.6f  %12.6f\n", j + 1, x_j, u[j], u_exact);
  // }

  free(u);
  return 0;
}
