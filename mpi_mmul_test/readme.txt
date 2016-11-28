This program performs matrix-matrix multiplication for a quad-tree
matrix representatoin, implemented using the Chunks and Tasks
programming model.

To build it you need a Chunks and Tasks runtime library
implementation. The Makefile is currently set up to use CHT-MPI
version 1.1 which can be downloaded and built like this:

wget http://chunks-and-tasks.org/source/tarfiles/cht-mpi-1.1.tar.gz
tar -xzf cht-mpi-1.1.tar.gz
cd cht-mpi-1.1
make

The above can be done using the prepare_cht_mpi.sh script.

A BLAS library is also needed, OpenBLAS can be fetched and built using
the prepare_openblas.sh script.
