#include "MatrixMultiply.h"
#include "MatrixAdd.h"
#include "CreateMatrixFromIds.h"
#include <cstring>

extern "C"
void dgemm_(const char *ta,const char *tb,
	    const int *n, const int *k, const int *l,
	    const double *alpha,const double *A,const int *lda,
	    const double *B, const int *ldb,
	    const double *beta, double *C, const int *ldc);

CHT_TASK_TYPE_IMPLEMENTATION((MatrixMultiply));
cht::ID MatrixMultiply::execute(CMatrix const & A, CMatrix const & B) {
  int nA = A.n;
  int nB = B.n;
  if(nA != nB || nA < CMatrix::BLOCK_SIZE)
    throw std::runtime_error("Error in MatrixMultiply::execute: (nA != nB || nA < CMatrix::BLOCK_SIZE).");
  int n = nA;
  if(n <= CMatrix::BLOCK_SIZE) {
    assert(n == CMatrix::BLOCK_SIZE);
    // Lowest level
    CMatrix* C = new CMatrix();
    C->n = n;
    C->elements.resize(n*n);
    memset(&C->elements[0], 0, n*n*sizeof(double));
    if(CMatrix::USE_BLAS == 1) {
      // Use BLAS
      double alpha = 1.0;
      double beta = 0;
      dgemm_("T", "T", &n, &n, &n, &alpha,
	     &A.elements[0], &n, &B.elements[0], &n,
	     &beta, &C->elements[0], &n);
    }
    else {
      // Do not use BLAS
      for(int i = 0; i < n; i++)
	for(int k = 0; k < n; k++)
	  for(int j = 0; j < n; j++) {
	    double Aik = A.elements[i*n+k];
	    double Bkj = B.elements[k*n+j];
	    C->elements[j*n+i] += Aik * Bkj;
	  }
    } // end else not using BLAS
    return registerChunk(C, cht::persistent);
  }
  else {
    for(int i = 0; i < 4; i++) {
      if(A.children[i] == cht::CHUNK_ID_NULL)
	throw std::runtime_error("Error in MatrixMultiply::execute: CHUNK_ID_NULL found for A.");
      if(B.children[i] == cht::CHUNK_ID_NULL)
	throw std::runtime_error("Error in MatrixMultiply::execute: CHUNK_ID_NULL found for B.");
    }
    // Not lowest level
    cht::ID childTaskIDs[4];
    for(int i = 0; i < 2; i++)
      for(int j = 0; j < 2; j++) {
	cht::ID childTaskIDsForSum[2];
	for(int k = 0; k < 2; k++)
	  childTaskIDsForSum[k] = registerTask<MatrixMultiply>(A.children[i*2+k], B.children[k*2+j]);
	childTaskIDs[j*2+i] = registerTask<MatrixAdd>(childTaskIDsForSum[0], childTaskIDsForSum[1]);
      }
    cht::ChunkID cid_n = registerChunk( new CInt(n) );
    return registerTask<CreateMatrixFromIds>(cid_n, childTaskIDs[0], childTaskIDs[1], childTaskIDs[2], childTaskIDs[3], cht::persistent);
  }
} // end execute
