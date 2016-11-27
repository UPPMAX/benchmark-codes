/* This program tests and measures timings for matrix-matrix
   multiplication using BLAS and compares to a naive
   implementation. The idea is to run this linking to different BLAS
   variants with and without threading inside the BLAS gemm routine,
   to see how much speedup can be achieved from threading.

   Written by Elias Rudberg.
*/

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void dgemm_(const char *ta,const char *tb,
	    const int *n, const int *k, const int *l,
	    const double *alpha,const double *A,const int *lda,
	    const double *B, const int *ldb,
	    const double *beta, double *C, const int *ldc);

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

/* When calling this routine, A is supposed to point to an array of
   n*n double numbers. */
static void fill_matrix_with_random_numbers(int n, double* A) {
  int i, j;
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++) {
      double randomNumber = rand();
      A[i*n+j] = randomNumber / RAND_MAX;
    }
}

/* Computes product C=A*B in a naive way. Each of the pointers A, B, C
   is supposed to point to an array of n*n double numbers. */
static void do_naive_mmul(double* C,
			  const double* A,
			  const double* B,
			  int n) {
  int i, j, k;
  // NOTE OpenMP pragma can be insterted here, if desired.
  // #pragma omp parallel for default(shared)
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++) {
      double sum = 0;
      for(k = 0; k < n; k++)
	sum += A[i*n+k] * B[k*n+j];
      C[j*n+i] = sum;
    }
}

static void verify_mmul_result(const double* A,
			       const double* B,
			       const double* C,
			       int n) {
  double maxabsdiff = 0;
  int count = 0;
  int i, j, k;
  for(i = 0; i < n; i++)
    for(j = 0; j < n; j++) {
      // Verify only certain elements to save time.
      if((rand() % 5000) != 0)
	continue;
      double sum = 0;
      for(k = 0; k < n; k++)
	sum += A[i*n+k] * B[k*n+j];
      double absdiff = fabs(C[j*n+i] - sum);
      if(absdiff > maxabsdiff)
	maxabsdiff = absdiff;
      count++;
    }
  printf("verify_mmul_result, verified %d elements, maxabsdiff = %g\n", count, maxabsdiff);
}

double compare_matrices(const double* A,
			const double* B,
			int n) {
  double maxabsdiff = 0;
  int i;
  for(i = 0; i < n*n; i++) {
    double absdiff = fabs(A[i] - B[i]);
    if(absdiff > maxabsdiff)
      maxabsdiff = absdiff;
  }
  return maxabsdiff;
}

int main(int argc, char *argv[])
{
  int n = 500;
  if(argc >= 2)
    n = atoi(argv[1]);
  int do_naive_mmul_comparison = 1;
  if(argc >= 3)
    do_naive_mmul_comparison = atoi(argv[2]);
  printf("blas_mmul_test start, matrix size n = %6d, do_naive_mmul_comparison = %d.\n", n, do_naive_mmul_comparison);
  // Generate matrices A and B filled with random numbers.
  double* A = (double*)malloc(n*n*sizeof(double));
  double* B = (double*)malloc(n*n*sizeof(double));
  fill_matrix_with_random_numbers(n, A);
  fill_matrix_with_random_numbers(n, B);
  printf("random matrices A and B generated OK.\n");

  // Compute matrix C = A*B using naive implementation.
  double* C = (double*)malloc(n*n*sizeof(double));
  if(do_naive_mmul_comparison == 1) {
    double seconds_start = get_wall_seconds();
    do_naive_mmul(C, A, B, n);
    double secondsTaken_naive_mmul = get_wall_seconds() - seconds_start;
    printf("do_naive_mmul took   %6.3f wall seconds.\n", secondsTaken_naive_mmul);
    verify_mmul_result(A, B, C, n);
  }

  // Now do the same computation by calling the BLAS gemm routine.
  double alpha = 1;
  double beta = 0;
  double* C2 = (double*)malloc(n*n*sizeof(double));
  double seconds_start_BLAS_gemm_1 = get_wall_seconds();
  dgemm_("T", "T", &n, &n, &n, &alpha,
	 &A[0], &n, &B[0], &n,
	 &beta, &C2[0], &n);
  double secondsTaken_BLAS_gemm_1 = get_wall_seconds() - seconds_start_BLAS_gemm_1;
  printf("BLAS gemm call took   %6.3f wall seconds.\n", secondsTaken_BLAS_gemm_1);
  double diff1 = 0;
  if(do_naive_mmul_comparison == 1) {
    // Check that results are equal.
    diff1 = compare_matrices(C, C2, n);
    printf("Max abs diff (elementwise) between naive and BLAS gemm results: %6.3g\n", (double)diff1);
  }
  verify_mmul_result(A, B, C2, n);

  // Now do the same computation by again calling the BLAS gemm routine.
  double* C3 = (double*)malloc(n*n*sizeof(double));
  double seconds_start_BLAS_gemm_2 = get_wall_seconds();
  dgemm_("T", "T", &n, &n, &n, &alpha,
	 &A[0], &n, &B[0], &n,
	 &beta, &C3[0], &n);
  double secondsTaken_BLAS_gemm_2 = get_wall_seconds() - seconds_start_BLAS_gemm_2;
  printf("BLAS gemm call took   %6.3f wall seconds.\n", secondsTaken_BLAS_gemm_2);
  double diff2 = 0;
  if(do_naive_mmul_comparison == 1) {
    // Check that results are equal.
    diff2 = compare_matrices(C, C3, n);
    printf("Max abs diff (elementwise) between naive and BLAS gemm results: %6.3g\n", (double)diff2);
  }
  verify_mmul_result(A, B, C3, n);

  if(do_naive_mmul_comparison == 1) {
    double tol = 1e-4;
    if(diff1 > tol || diff2 > tol) {
      printf("Error: too large diff between naive mmul and BLAS gemm results.\n");
      return -1;
    }
  }
  
  printf("blas_mmul_test finished OK.\n");
  return 0;
}
