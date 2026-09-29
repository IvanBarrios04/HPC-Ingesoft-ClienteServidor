#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

int* random_matrix(int size) {
    int* matrix = malloc(size * size * sizeof(int));

    for (int i = 0; i < size * size; i++)
        matrix[i] = rand() % 472;

    return matrix;
}

void partial_matrix_product(int* A, int* B, int* C, int size, int start_row, int end_row) {

    for (int i = start_row; i < end_row; i++) {

        for (int j = 0; j < size; j++) {

            int sum = 0;

            for (int k = 0; k < size; k++) {
                sum += A[i * size + k] * B[k * size + j];
            }

            C[i * size + j] = sum;
        }
    }
}

int* matrix_product_n_processes(int* A, int* B, int size, int num_processes) {

    int shm_fd = shm_open("/matrix_result", O_CREAT | O_RDWR, 0666);

    size_t bytes = size * size * sizeof(int);

    ftruncate(shm_fd, bytes);

    int* C = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    int rows_per_process = size / num_processes;

    for (int i = 0; i < num_processes; i++) {

        pid_t pid = fork();

        if (pid == 0) {

            int start = i * rows_per_process;
            int end = (i == num_processes - 1) ? size : start + rows_per_process;

            partial_matrix_product(A, B, C, size, start, end);

            exit(0);
        }
    }

    for (int i = 0; i < num_processes; i++)
        wait(NULL);

    return C;
}

void free_matrix(int* matrix) {
    free(matrix);
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Usage: %s [matrix_size] [num_processes]\n", argv[0]);
        return 1;
    }

    srand(time(NULL));

    int size = atoi(argv[1]);
    int num_processes = atoi(argv[2]);

    int* matrix_a = random_matrix(size);
    int* matrix_b = random_matrix(size);

    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    int* resulting_matrix = matrix_product_n_processes(matrix_a, matrix_b, size, num_processes);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double time_taken =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("%f\n", time_taken);

    free_matrix(matrix_a);
    free_matrix(matrix_b);

    munmap(resulting_matrix, size * size * sizeof(int));
    shm_unlink("/matrix_result");

    return 0;
}