#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define THRESHOLD 1e-4
#define STABLE_STEPS 50
#define MAX_STEPS 100000

int* allocate_road (int n) {
  int* road = (int*) malloc(n * sizeof(int));
  if (!road) {
    printf("Memory allocation failed\n");
    exit(1);
  }
  return road;
}

void initialize_road (int* road, int n, double density) {
  for (int i = 0; i < n; i++) {
    double r = (double) rand() / RAND_MAX;
    road[i] = (r < density) ? 1 : 0;
  }
}

void update_road (int* old, int* new, int n) {
  #pragma omp parallel for
  for (int i = 0; i < n; i++) {
    int left  = (i - 1 + n) % n;
    int right = (i + 1) % n;

    if (old[i] == 1)
      new[i] = (old[right] == 1) ? 1 : 0;
    else
      new[i] = (old[left] == 1) ? 1 : 0;
  }
}

int count_cars (int* road, int n) {
  int total = 0;
  #pragma omp parallel for reduction(+:total)
  for (int i = 0; i < n; i++)
    if (road[i] == 1) total++;
  return total;
}

int count_moved_cars(int* road, int n) {
  int moved = 0;
  #pragma omp parallel for reduction(+:moved)
  for (int i = 0; i < n; i++) {
    int right = (i + 1) % n;
    if (road[i] == 1 && road[right] == 0)
      moved++;
  }
  return moved;
}

void copy_road (int* dest, int* src, int n) {
  #pragma omp parallel for
  for (int i = 0; i < n; i++)
    dest[i] = src[i];
}

double compute_velocity (int moved, int total) {
  if (total == 0) return 0.0;
  return (double) moved / total;
}

void cellular_automaton (int* old, int* new, int cells_number) {
  double prev_velocity = 0.0;
  int stable_count = 0;
  int step = 0;

  while (1) {
    int total = count_cars(old, cells_number);
    int moved = count_moved_cars(old, cells_number);
    double velocity = compute_velocity(moved, total);

    // printf("Step %d: velocity = %f\n", step, velocity);

    // Steady-state check
    if (fabs(velocity - prev_velocity) < THRESHOLD) {
      stable_count++;
    } else {
      stable_count = 0;
    }

    if (stable_count >= STABLE_STEPS) {
      // printf("\nSteady state reached at step %d\n", step);
      // printf("Final steady velocity ≈ %f\n", velocity);
      break;
    }

    prev_velocity = velocity;

    update_road(old, new, cells_number);
    copy_road(old, new, cells_number);

    step++;

    // Safety break
    if (step > MAX_STEPS) {
      // printf("\nMax steps reached without clear steady state\n");
      break;
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    printf("Usage: %s <CELLS_NUMBER> <DENSITY> <SEED>\n", argv[0]);
    return 1;
  }

  int cells_number = atoi(argv[1]);
  double density = atof(argv[2]);

  if (cells_number <= 0) {
    printf("CELLS_NUMBER must be > 0\n");
    return 1;
  }

  if (density <= 0 || density >= 1) {
    printf("DENSITY must be > 0 and < 1\n");
    return 1;
  }

  int seed = atoi(argv[3]);
  srand(seed);

  int* old = allocate_road(cells_number);
  int* new = allocate_road(cells_number);
  initialize_road(old, cells_number, density);

  struct timespec start, end;
  // Here it starts taking wall-clock time
  clock_gettime(CLOCK_MONOTONIC, &start);

  cellular_automaton(old, new, cells_number);

  clock_gettime(CLOCK_MONOTONIC, &end);
  // It ends taking wall-clock time

  double time_taken =
    (end.tv_sec - start.tv_sec) +
    (end.tv_nsec - start.tv_nsec) / 1e9;

  printf("%f", time_taken);

  free(old);
  free(new);

  return 0;
}
