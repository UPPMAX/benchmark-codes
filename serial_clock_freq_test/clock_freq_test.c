/* This program estimates processor clock frequency by repeating a
   simple operation (i++) that is assumed to take only one clock cycle
   to execute.

   To make this work, compile without optimization flags since
   otherwise the compiler will probably optimize away most of the
   work.

   Written by Elias Rudberg. A similar code previously used in the
   High Performance Computing and Programming course.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

int main (int argc, char** argv) {
  if(argc != 2) {
    printf("Please give one argument, saying how many billions of operations to perform.\n");
    return -1;
  }
  int nBillions = atoi(argv[1]);
  printf("Hello! nBillions = %d\n", nBillions);
  long int N_one_billion = 1000000000;
  long int N = nBillions*N_one_billion;
  double startTime = get_wall_seconds();
  register long int i = 0;
  while(i < N) {
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
  }
  double timeTaken = get_wall_seconds() - startTime;
  printf("N = %ld, timeTaken = %7.3f\n", N, timeTaken);
  double ops_per_second = (double)N / timeTaken;
  printf("--> processor clock frequency seems to be %4.2f GHz\n", ops_per_second/N_one_billion);
}
