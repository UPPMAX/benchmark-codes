#include <iostream>
#include <vector>
#include <cmath>
#include <stdlib.h>
#include "chunks_and_tasks.h"
#include "CInt.h"
#include "CDouble.h"
#include "CMatrix.h"
#include "CreateMatrix.h"
#include "GetMatrixElement.h"
#include "MatrixMultiply.h"
#include "MatrixAdd.h"
#include "MatrixElementValues.h"

static double compute_product_matrix_element(int N, int i, int j) {
  double sum = 0;
  for(int k = 0; k < N; k++) {
    double Aik = matElementFunc(MATRIX_TYPE_A, i, k);
    double Bkj = matElementFunc(MATRIX_TYPE_B, k, j);
    sum += Aik * Bkj;
  }
  return sum;
}

int main(int argc, char* const  argv[])
{
  try {
    if(argc != 5) {
      std::cout << "Please give 4 arguments: N nWorkerProcs nThreads cacheInGB" << std::endl;
      return -1;
    }
    long int N = atoi(argv[1]);
    int nWorkerProcs = atoi(argv[2]);
    int nThreads = atoi(argv[3]);
    double cacheInGB = atof(argv[4]);
    std::cout << "CMatrix::BLOCK_SIZE = " << CMatrix::BLOCK_SIZE << std::endl;
    std::cout << "CMatrix::USE_BLAS = " << CMatrix::USE_BLAS << std::endl;
    std::cout << "N = " << N << std::endl;
    std::cout << "nWorkerProcs = " << nWorkerProcs << std::endl;
    std::cout << "nThreads = " << nThreads << std::endl;
    std::cout << "cacheInGB = " << cacheInGB << std::endl;
    size_t size_of_matrix_in_bytes = N*N*sizeof(double);
    double size_of_matrix_in_GB = (double)size_of_matrix_in_bytes / 1000000000;
    std::cout << "size_of_matrix_in_GB = " << size_of_matrix_in_GB << std::endl;
    cht::extras::setNWorkers(nWorkerProcs);
    cht::setOutputMode(cht::Output::AllInTheEnd);
    cht::extras::setNoOfWorkerThreads(nThreads);

    if(cacheInGB >= 0) {
      size_t cacheMemoryUsageLimit = (size_t)(cacheInGB*1e9);
      cht::extras::setCacheSize(cacheMemoryUsageLimit);
      double cacheMemoryUsageLimit_in_GB = (double)cacheMemoryUsageLimit / 1e9;
      std::cout << "Chunk cache enabled, chunk cache size is " << cacheMemoryUsageLimit_in_GB << " GB." << std::endl;
      cht::extras::setCacheMode(cht::extras::Cache::Enabled);
    }
    else {
      std::cout << "Chunk cache is disabled." << std::endl;
      cht::extras::setCacheMode(cht::extras::Cache::Disabled);
    }

    cht::start();

    cht::ChunkID cid_baseIdx1 = cht::registerChunk<CInt>(new CInt(0));
    cht::ChunkID cid_baseIdx2 = cht::registerChunk<CInt>(new CInt(0));
    cht::ChunkID cid_n = cht::registerChunk<CInt>(new CInt(N));
    cht::ChunkID cid_matType_A = cht::registerChunk<CInt>(new CInt(1));
    cht::ChunkID cid_matType_B = cht::registerChunk<CInt>(new CInt(2));

    std::cout << "Calling executeMotherTask() for CreateMatrix for A..." << std::endl;
    cht::ChunkID cid_matrix_A = cht::executeMotherTask<CreateMatrix>(cid_n, cid_baseIdx1, cid_baseIdx2, cid_matType_A);

    std::cout << "Calling executeMotherTask() for CreateMatrix for B..." << std::endl;
    cht::ChunkID cid_matrix_B = cht::executeMotherTask<CreateMatrix>(cid_n, cid_baseIdx1, cid_baseIdx2, cid_matType_B);

    cht::resetStatistics();
    std::cout << "Calling executeMotherTask() for MatrixMultiply to compute C = A * B ..." << std::endl;
    cht::ChunkID cid_matrix_C = cht::executeMotherTask<MatrixMultiply>(cid_matrix_A, cid_matrix_B);
    cht::reportStatistics();

    int nElementsToVerify = 20;
    std::cout << "Verifying result by checking " << nElementsToVerify << " C matrix elements..." << std::endl;
    double max_abs_diff = 0;
    for(int i = 0; i < nElementsToVerify; i++) {
      int idx1 = rand() % N;
      int idx2 = rand() % N;
      cht::ChunkID cid_idx1 = cht::registerChunk<CInt>(new CInt(idx1));
      cht::ChunkID cid_idx2 = cht::registerChunk<CInt>(new CInt(idx2));      
      cht::ChunkID cid_value = cht::executeMotherTask<GetMatrixElement>(cid_matrix_C, cid_idx1, cid_idx2);
      cht::shared_ptr<CDouble const> valuePtr;
      cht::getChunk(cid_value, valuePtr);
      double value = *valuePtr;
      // Compute expected value for this C matrix element
      double value_expected = compute_product_matrix_element(N, idx2, idx1);
      double absdiff = std::fabs(value - value_expected);
      //      std::cout << "Checking C matrix element ( " << idx1 << " , " << idx2 << " ) : value = " << value << " , value_expected = " << value_expected << " , absdiff = " << absdiff << std::endl;
      if(absdiff > max_abs_diff)
	max_abs_diff = absdiff;
      cht::deleteChunk(cid_idx1);
      cht::deleteChunk(cid_idx2);
      cht::deleteChunk(cid_value);
    }
    if(max_abs_diff > 1e-8) {
      std::cout << "Error: absdiff too large, result seems wrong, max_abs_diff = " << max_abs_diff << "." << std::endl;
      return -1;
    }
    std::cout << "OK, result seems correct, max_abs_diff = " << max_abs_diff << "." << std::endl;
    
    std::cout << "Cleaning up..." << std::endl;
    cht::deleteChunk(cid_baseIdx1);
    cht::deleteChunk(cid_baseIdx2);
    cht::deleteChunk(cid_n);
    cht::deleteChunk(cid_matrix_A);
    cht::deleteChunk(cid_matrix_B);
    cht::deleteChunk(cid_matrix_C);
    cht::deleteChunk(cid_matType_A);
    cht::deleteChunk(cid_matType_B);

    // Stop cht services
    cht::stop();

    std::cout << "Done, test_matrix finished OK." << std::endl;

  } catch ( std::exception & e) {
    std::cerr << "Exception caught in test main! What: "<< e.what() << std::endl;
    return 1;
  }
  
  return 0;
}

