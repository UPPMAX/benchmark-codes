CHTPATH=cht-mpi-1.1/source
CHTINCL=-I$(CHTPATH)

BLAS_LIB=OpenBLAS/libopenblas.a

CC=mpiCC
CFLAGS= -O2 -std=c++11

.PHONY: test_matrix

# List all object files here (except the one for the main program)
WRK_OBJS = CInt.o CDouble.o CMatrix.o CreateMatrix.o MatrixAdd.o MatrixMultiply.o CreateMatrixFromIds.o GetMatrixElement.o

# List all header files here
HEADER_FILES = CDouble.h CInt.h CMatrix.h CreateMatrixFromIds.h CreateMatrix.h GetMatrixElement.h MatrixAdd.h MatrixElementValues.h MatrixMultiply.h

test_matrix: test_matrix_manager cht_worker

test_matrix_manager: test_matrix.o $(WRK_OBJS) $(CHTPATH)/libcht.a $(BLAS_LIB)
	$(CC) $(CFLAGS) $(CHTINCL) -o $@ $^
cht_worker: $(WRK_OBJS) $(CHTPATH)/libcht.a $(BLAS_LIB)
	$(CC) $(CFLAGS) $(CHTINCL) -o $@ $^

%.o: %.cc $(HEADER_FILES)
	$(CC) $(CFLAGS) $(CHTINCL) -c $< -o $@

clean:
	rm -f *.o test_matrix_manager cht_worker
