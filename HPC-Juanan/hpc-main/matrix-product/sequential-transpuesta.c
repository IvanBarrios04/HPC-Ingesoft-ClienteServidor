#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int* random_vector (int size) {
  int* vector = malloc(size * sizeof(int));
  
  for (int i = 0; i < size; i++) {
    vector[i] = rand() % 472;
  }

  return vector;
}

int** random_matrix (int size) {
  int** matrix = malloc(size * sizeof(int*));
  
  for (int i = 0; i < size; i++) {
    matrix[i] = random_vector(size);
  }

  return matrix;
}

int** matrix_product (int** matrix_a, int** matrix_b, int size) {
  int** resulting_matrix = malloc(size * sizeof(int*));
  for (int i = 0; i < size; i++)
    resulting_matrix[i] = malloc(size * sizeof(int));

  for (int a_row = 0; a_row < size; a_row++) {
    for (int b_col = 0; b_col < size; b_col++) {
      int sum = 0;
      for (int counter = 0; counter < size; counter++) {
        sum += matrix_a[a_row][counter] * matrix_b[b_col][counter];
      }
      resulting_matrix[a_row][b_col] = sum;
    }
  }

  return resulting_matrix;
}

void free_matrix(int** matrix, int size) {
  for (int i = 0; i < size; i++) {
    free(matrix[i]);
  }
  free(matrix);
}

// void print_long_matrix(int** matrix, int size) {
//   for (int i = 0; i < size; i++) {
//     for (int j = 0; j < size; j++) {
//       printf("%ld ", matrix[i][j]);
//     }
//     printf("\n");
//   }
// }

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Usage: %s [matrix_size]\n", argv[0]);
    return 1;
  }

  srand(time(NULL)); // Initialize seed using current time in seconds

  int size = atoi(argv[1]);
  int** matrix_a = random_matrix(size);
  int** matrix_b = random_matrix(size);
  
  struct timespec start, end;
  // Here it starts taking wall-clock time
  clock_gettime(CLOCK_MONOTONIC, &start);

  int** resulting_matrix = matrix_product(matrix_a, matrix_b, size);

  clock_gettime(CLOCK_MONOTONIC, &end);
  // It ends taking wall-clock time

  double time_taken =
    (end.tv_sec - start.tv_sec) +
    (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("%f", time_taken);

  // printf("Matrix a\n" );
  // print_long_matrix(matrix_a, size);
  // printf("Matrix b\n" );
  // print_long_matrix(matrix_b, size);
  // printf("Matrix resulting\n" );
  // print_long_matrix(resulting_matrix, size);

  return 0;
}
