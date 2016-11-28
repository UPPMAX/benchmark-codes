#include "chunks_and_tasks.h"
#include "CMatrix.h"

struct MatrixAdd: public cht::Task {
  cht::ID execute(CMatrix const &, CMatrix const &);
  CHT_TASK_INPUT((CMatrix, CMatrix));
  CHT_TASK_OUTPUT((CMatrix));
  CHT_TASK_TYPE_DECLARATION;
};
