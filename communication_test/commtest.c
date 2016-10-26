#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

static double get_wall_seconds() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double seconds = tv.tv_sec + (double)tv.tv_usec / 1000000;
  return seconds;
}

static void modifyBuf(char* buf, int bufSz) {
  for(int i = 0; i < bufSz; i+=200)
    buf[i]++;
}

static int mainFuncMaster(int nProcsTot, int noOfMessageBatches, int messageSizeInBytes, int nMessagesPerBatch) {
  // This is the "master" process, it's job is to send messages to workers, measure the time it takes for them to respond, and check that the received data is correct.
  printf("Doing communication test with the following parameters:\n");
  printf("nProcsTot          = %d\n", nProcsTot);
  printf("noOfMessageBatches = %d\n", noOfMessageBatches);
  printf("messageSizeInBytes = %d --> %f MB\n", messageSizeInBytes, (double)messageSizeInBytes/1000000);
  printf("nMessagesPerBatch  = %d\n", nMessagesPerBatch);
  double timeTaken_min = -1;
  int timeTaken_min_batchIdx = -1;
  int timeTaken_min_slaveIdx = -1;
  double timeTaken_max = -1;
  int timeTaken_max_batchIdx = -1;
  int timeTaken_max_slaveIdx = -1;
  double timeTaken_tot = 0;
  int counter = 0;
  char* buf1 = (char*)malloc(messageSizeInBytes);
  char* buf2 = (char*)malloc(messageSizeInBytes);
  int nSlaveProcs = nProcsTot - 1;
  int firstTime = 1; // 1 means it is first time, 0 if not first time
  for(int batchIdx = 0; batchIdx < noOfMessageBatches; batchIdx++) {
    for(int slaveIdx = 0; slaveIdx < nSlaveProcs; slaveIdx++) {
      // Prepare buf1 contents to send
      for(int i = 0; i < messageSizeInBytes; i++)
	buf1[i] = rand() % 77;
      // Prepare expected received buf contents in buf2
      memcpy(buf2, buf1, messageSizeInBytes);
      for(int k = 0; k < nMessagesPerBatch; k++)
	modifyBuf(buf2, messageSizeInBytes);
      double startTime = get_wall_seconds();
      int rankToCommunicateWith = slaveIdx + 1;
      int tag = 0;
      for(int k = 0; k < nMessagesPerBatch; k++) {
	// Send buf1 contents
	MPI_Send(buf1, messageSizeInBytes, MPI_UNSIGNED_CHAR, rankToCommunicateWith, tag, MPI_COMM_WORLD);
	// Receive message into buf1 (overwriting previous buf1 contents)
	MPI_Recv(buf1, messageSizeInBytes, MPI_UNSIGNED_CHAR, rankToCommunicateWith, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      double timeTaken = get_wall_seconds() - startTime;
      if(firstTime || timeTaken < timeTaken_min) {
	timeTaken_min = timeTaken;
	timeTaken_min_batchIdx = batchIdx;
	timeTaken_min_slaveIdx = slaveIdx;
      }
      if(firstTime || timeTaken > timeTaken_max) {
	timeTaken_max = timeTaken;
	timeTaken_max_batchIdx = batchIdx;
	timeTaken_max_slaveIdx = slaveIdx;
      }
      timeTaken_tot += timeTaken;
      firstTime = 0;
      counter++;
      // Verify that received data is correct, by comparing to buf2 contents.
      if(memcmp(buf1, buf2, messageSizeInBytes) != 0) {
	printf("ERROR: received data not correct.\n");
	return -1;
      }
    }
  }
  double timeTaken_avg = timeTaken_tot / counter;
  printf("OK, communication test done. Results (times in wall seconds):\n");
  printf("timeTaken_min = %f\n", timeTaken_min);
  printf("timeTaken_max = %f\n", timeTaken_max);
  printf("timeTaken_avg = %f\n", timeTaken_avg);
  printf("min time occurred for batchIdx %d and slaveIdx %d\n", timeTaken_min_batchIdx, timeTaken_min_slaveIdx);
  printf("max time occurred for batchIdx %d and slaveIdx %d\n", timeTaken_max_batchIdx, timeTaken_max_slaveIdx);
  int factor = 2*nMessagesPerBatch; // Use factor 2*nMessagesPerBatch here because there are 2 messages sent, back and forth
  // Estimate latency time for one message
  double latency_one_msg_min = timeTaken_min / factor;
  double latency_one_msg_max = timeTaken_max / factor;
  double latency_one_msg_avg = timeTaken_avg / factor;
  printf("Latency numbers (really only correspond to \"latency\" if message size is small):\n");
  printf("latency_one_msg_min = %.9f = %.3f microseconds\n", latency_one_msg_min, latency_one_msg_min*1e6);
  printf("latency_one_msg_max = %.9f = %.3f microseconds\n", latency_one_msg_max, latency_one_msg_max*1e6);
  printf("latency_one_msg_avg = %.9f = %.3f microseconds\n", latency_one_msg_avg, latency_one_msg_avg*1e6);
  double messageSizeInGB = (double)messageSizeInBytes/(1e9);
  double bandwidth_best_GB_per_sec    = factor * messageSizeInGB / timeTaken_min;
  double bandwidth_worst_GB_per_sec   = factor * messageSizeInGB / timeTaken_max;
  double bandwidth_typical_GB_per_sec = factor * messageSizeInGB / timeTaken_avg;
  printf("bandwidth_best    = %f GB/second\n", bandwidth_best_GB_per_sec);
  printf("bandwidth_worst   = %f GB/second\n", bandwidth_worst_GB_per_sec);
  printf("bandwidth_typical = %f GB/second\n", bandwidth_typical_GB_per_sec);
  free(buf1);
  free(buf2);
  return 0;
}

static int mainFuncSlave(int noOfMessageBatches, int messageSizeInBytes, int nMessagesPerBatch) {
  // This is a "slave" process, it's job is simply to wait for messages and send a response back each time a message arrives.
  char* buf = (char*)malloc(messageSizeInBytes);
  for(int batchIdx = 0; batchIdx < noOfMessageBatches; batchIdx++) {
    for(int k = 0; k < nMessagesPerBatch; k++) {
      int rankToCommunicateWith = 0;
      int tag = 0;
      MPI_Recv(buf, messageSizeInBytes, MPI_UNSIGNED_CHAR, rankToCommunicateWith, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      // OK, message received. Now modify each byte in buf before sending a message back.
      modifyBuf(buf, messageSizeInBytes);
      MPI_Send(buf, messageSizeInBytes, MPI_UNSIGNED_CHAR, rankToCommunicateWith, tag, MPI_COMM_WORLD);
    }
  }
  free(buf);
  return 0;
}

int main(int argc, const char* argv[]) {
  MPI_Init(0, 0);
  int nProcs;
  MPI_Comm_size(MPI_COMM_WORLD, &nProcs);
  int myRank;
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  if(nProcs <= 1) {
    printf("Please run this test with at least two MPI processes.\n");
    return -1;
  }
  if(argc != 4) {
    printf("Please give 3 arguments: noOfMessageBatches messageSizeInBytes nMessagesPerBatch\n");
    return -1;
  }
  int noOfMessageBatches = atoi(argv[1]);
  int messageSizeInBytes = atoi(argv[2]);
  int nMessagesPerBatch  = atoi(argv[3]);
  if(noOfMessageBatches <= 0 || messageSizeInBytes <= 0 || nMessagesPerBatch <= 0) {
    printf("Error: (noOfMessageBatches <= 0 || messageSizeInBytes <= 0 || nMessagesPerBatch <= 0).\n");
    return -1;
  }
  MPI_Barrier(MPI_COMM_WORLD);
  int resultCode = 0;
  if(myRank == 0)
    resultCode = mainFuncMaster(nProcs, noOfMessageBatches, messageSizeInBytes, nMessagesPerBatch);
  else
    resultCode = mainFuncSlave(noOfMessageBatches, messageSizeInBytes, nMessagesPerBatch);
  if(resultCode == 0 && myRank == 0)
    printf("MPI communication test finished OK.\n");
  MPI_Finalize();
  return 0;
}
