#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <semaphore.h>

#define SHM_NAME  "/jacobi_shm"
#define MAX_PROCS 20
#define TOL       1e-6

/* ==========================================================================
 * Estructura de memoria compartida
 * Despues de la cabecera se almacenan dos buffers de doubles: buf[0] y buf[1]
 * current indica cual buffer es el activo (points) y 1-current el nuevo
 * ========================================================================== */
typedef struct {
    sem_t  sem_start[MAX_PROCS];
    sem_t  sem_done [MAX_PROCS];
    int    stop;
    int    current;
    int    n;
    double h;
} SharedHeader;

/* ==========================================================================
 * Funciones del problema de Poisson
 * ========================================================================== */
double f(double x) {
    return (-x) * (x + 3) * exp(x);
}

double known_u(double x) {
    return x * (x - 1) * exp(x);
}

/* ==========================================================================
 * Funciones de memoria compartida
 * ========================================================================== */
static inline double* get_buf(SharedHeader* shm, int which) {
    return (double*)((char*)shm + sizeof(SharedHeader)) + which * shm->n;
}

SharedHeader* create_shm(int n) {
    size_t size = sizeof(SharedHeader) + 2 * n * sizeof(double);
    shm_unlink(SHM_NAME);
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, size);
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void destroy_shm(SharedHeader* shm) {
    size_t size = sizeof(SharedHeader) + 2 * shm->n * sizeof(double);
    munmap(shm, size);
    shm_unlink(SHM_NAME);
}

void init_shm(SharedHeader* shm, int n, double h, int num_procs) {
    shm->n       = n;
    shm->h       = h;
    shm->stop    = 0;
    shm->current = 0;

    double* buf0 = get_buf(shm, 0);
    double* buf1 = get_buf(shm, 1);
    for (int i = 0; i < n; i++) { buf0[i] = 0.0; buf1[i] = 0.0; }

    for (int i = 0; i < num_procs; i++) {
        sem_init(&shm->sem_start[i], 1, 0);
        sem_init(&shm->sem_done [i], 1, 0);
    }
}

void destroy_semaphores(SharedHeader* shm, int num_procs) {
    for (int i = 0; i < num_procs; i++) {
        sem_destroy(&shm->sem_start[i]);
        sem_destroy(&shm->sem_done [i]);
    }
}

/* ==========================================================================
 * Funciones de Jacobi
 * ========================================================================== */
void update_chunk(SharedHeader* shm, int start, int end) {
    double* points     = get_buf(shm, shm->current);
    double* points_new = get_buf(shm, 1 - shm->current);
    double h = shm->h;

    for (int j = start; j < end; j++) {
        double x_j = j * h;
        points_new[j] = ((h * h) * f(x_j) + points[j - 1] + points[j + 1]) / 2.0;
    }
}

double residual_rms(SharedHeader* shm) {
    double* u  = get_buf(shm, shm->current);
    int     n  = shm->n;
    double  h  = shm->h;
    double sum = 0.0;

    for (int j = 1; j < n - 1; j++) {
        double x_j  = j * h;
        double Au_j = (-u[j - 1] + 2.0 * u[j] - u[j + 1]) / (h * h);
        double r_j  = Au_j - f(x_j);
        sum += r_j * r_j;
    }
    return sqrt(sum / n);
}

/* ==========================================================================
 * Gestion de procesos
 * ========================================================================== */
void child_process(SharedHeader* shm, int id, int start, int end) {
    while (1) {
        sem_wait(&shm->sem_start[id]);
        if (shm->stop) break;
        update_chunk(shm, start, end);
        sem_post(&shm->sem_done[id]);
    }
    exit(0);
}

void spawn_children(SharedHeader* shm, int num_procs) {
    int interior = shm->n - 2;
    int base     = interior / num_procs;
    int rem      = interior % num_procs;

    for (int i = 0; i < num_procs; i++) {
        int start = 1 + i * base + (i < rem ? i : rem);
        int end   = start + base  + (i < rem ? 1 : 0);

        if (fork() == 0)
            child_process(shm, i, start, end);
    }
}

void signal_start(SharedHeader* shm, int num_procs) {
    for (int i = 0; i < num_procs; i++)
        sem_post(&shm->sem_start[i]);
}

void wait_done(SharedHeader* shm, int num_procs) {
    for (int i = 0; i < num_procs; i++)
        sem_wait(&shm->sem_done[i]);
}

void stop_children(SharedHeader* shm, int num_procs) {
    shm->stop = 1;
    signal_start(shm, num_procs);
    for (int i = 0; i < num_procs; i++)
        wait(NULL);
}

/* ==========================================================================
 * Jacobi paralelo (ejecutado por el padre)
 * ========================================================================== */
int run_jacobi(SharedHeader* shm, int num_procs) {
    int iter = 0;
    while (1) {
        signal_start(shm, num_procs);
        wait_done   (shm, num_procs);

        shm->current = 1 - shm->current;
        iter++;

        if (residual_rms(shm) <= TOL) break;
    }
    return iter;
}

/* ==========================================================================
 * Main
 * ========================================================================== */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s [k] [num_processes]\n", argv[0]);
        return 1;
    }

    int    k         = atoi(argv[1]);
    int    num_procs = atoi(argv[2]);
    int    n         = (int)round(pow(2, k) + 1.0);
    double h         = 1.0 / (double)(n - 1);

    SharedHeader* shm = create_shm(n);
    init_shm(shm, n, h, num_procs);
    spawn_children(shm, num_procs);

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    int iter = run_jacobi(shm, num_procs);

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    stop_children     (shm, num_procs);
    destroy_semaphores(shm, num_procs);

    double time_taken =
        (t_end.tv_sec  - t_start.tv_sec) +
        (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("%f %d %d\n", time_taken, iter, n);

    destroy_shm(shm);
    return 0;
}
