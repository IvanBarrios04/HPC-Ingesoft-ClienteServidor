#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

typedef struct {
  int** matrix_a;
  int** matrix_b;
  int** resulting_matrix;
  int size;
  int start_row;
  int end_row;
} ThreadData;

int* random_vector (int size) {
  int* vector = malloc(size * sizeof(int));

  for (int i = 0; i < size; i++)
    vector[i] = rand() % 472;

  return vector;
}

int** random_matrix (int size) {
  int** matrix = malloc(size * sizeof(int*));

  for (int i = 0; i < size; i++)
    matrix[i] = random_vector(size);

  return matrix;
}

void* partial_matrix_product (void* arg) {
  ThreadData* data = (ThreadData*) arg;

  for (int a_row = data -> start_row; a_row < data -> end_row; a_row++) {
    for (int b_col = 0; b_col < data -> size; b_col++) {
      int sum = 0;
      for (int counter = 0; counter < data -> size; counter++) {
        sum += data -> matrix_a[a_row][counter] * data -> matrix_b[counter][b_col];
      }
      data -> resulting_matrix[a_row][b_col] = sum;
    }
  }

  pthread_exit(NULL);
}

int** matrix_product_n_threads (int** matrix_a, int** matrix_b, int size, int num_threads) {
  pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
  ThreadData* data = malloc(num_threads * sizeof(ThreadData));

  int** resulting_matrix = malloc(size * sizeof(int*));
  for (int i = 0; i < size; i++)
    resulting_matrix[i] = malloc(size * sizeof(int));

  int rows_per_thread = size / num_threads;

  for (int i = 0; i < num_threads; i++) {
    int start = i * rows_per_thread;
    int end = (i == num_threads - 1) ? size : start + rows_per_thread;

    data[i] = (ThreadData) { matrix_a, matrix_b, resulting_matrix, size, start, end };

    pthread_create(&threads[i], NULL, partial_matrix_product, &data[i]);
  }

  for (int i = 0; i < num_threads; i++)
    pthread_join(threads[i], NULL);

  free(threads);
  free(data);

  return resulting_matrix;
}

void free_matrix (int** matrix, int size) {
  for (int i = 0; i < size; i++)
    free(matrix[i]);
  free(matrix);
}

int main (int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s [matrix_size] [num_threads]\n", argv[0]);
    return 1;
  }

  srand(time(NULL));

  int size = atoi(argv[1]);
  int num_threads = atoi(argv[2]);

  int** matrix_a = random_matrix(size);
  int** matrix_b = random_matrix(size);

  struct timespec start, end;
  clock_gettime(CLOCK_MONOTONIC, &start);

  int** resulting_matrix = matrix_product_n_threads(matrix_a, matrix_b, size, num_threads);

  clock_gettime(CLOCK_MONOTONIC, &end);

  double time_taken =
    (end.tv_sec - start.tv_sec) +
    (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("%f\n", time_taken);

  free_matrix(matrix_a, size);
  free_matrix(matrix_b, size);
  free_matrix(resulting_matrix, size);

  return 0;
}
