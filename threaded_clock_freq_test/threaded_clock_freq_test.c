/* This program tests running a simple function using several threads,
   with each thread doing the same thing, completely independent of
   the other threads. The actual work done is just a loop made to
   estimate processor clock frequency by repeating a simple operation
   (i++) that is assumed to take only one clock cycle to execute.

   To make this work, compile without optimization flags since
   otherwise the compiler will probably optimize away most of the
   work.

   Written by Elias Rudberg.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

long int N_global = 0;

static void* thread_work_func(void* arg) {
  register long int i = 0;
  while(i < N_global) {
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
    i++; i++; i++; i++; i++; i++; i++; i++; i++; i++;
  }
}

int main (int argc, char** argv) {
  if(argc != 3) {
    printf("Please give two arguments: nBillions and nThreads\n");
    printf("     nBillions: how many billions of operations each thread should perform.\n");
    printf("     nThreads: number of threads to use.\n");
    return -1;
  }
  int nBillions = atoi(argv[1]);
  int nThreads = atoi(argv[2]);
  printf("Hello! nBillions = %d, nThreads = %d\n", nBillions, nThreads);
  long int N_one_billion = 1000000000;
  N_global = nBillions*N_one_billion;

  // Serial case
  double startTime_serial = get_wall_seconds();
  thread_work_func(NULL);
  double timeTaken_serial = get_wall_seconds() - startTime_serial;
  printf("N_global = %ld, timeTaken_serial = %7.3f wall seconds (serial case)\n", N_global, timeTaken_serial);
  double ops_per_second = (double)N_global / timeTaken_serial;
  printf("--> processor clock frequency seems to be %4.2f GHz (serial case)\n", ops_per_second/N_one_billion);
  
  // Threaded case
  int nThreadsToCreate = nThreads-1;
  printf("Now creating %d threads (in addition to main thread)...\n", nThreadsToCreate);
  double startTime_threaded = get_wall_seconds();
  pthread_t threads[nThreadsToCreate];
  int i;
  for(i = 0; i < nThreadsToCreate; i++) {
    if(pthread_create(&threads[i], NULL, thread_work_func, NULL) != 0) {
      printf("Error: pthread_create failed.\n");
      return -1;
    }
  }
  printf("OK, threads created. Now letting main thread work...\n");
  thread_work_func(NULL);
  printf("Main thread done with work. Now waiting for other threads to finish (calling pthread_join)...\n");
  for(i = 0; i < nThreadsToCreate; i++) {
    if(pthread_join(threads[i], NULL) != 0) {
      printf("Error: pthread_join failed.\n");
      return -1;
    }
  }
  double timeTaken_threaded = get_wall_seconds() - startTime_threaded;
  /* If the threads were able to run without delay and did not disturb
     eachother at all, then timeTaken_threaded should be the same as
     timeTaken_serial. */
  double percentage = 100.0 * (timeTaken_serial / timeTaken_threaded);
  printf("timeTaken_threaded = %7.3f wall seconds (%5.1f %% of ideal parallel performance)\n", timeTaken_threaded, percentage);

  return 0;
}
