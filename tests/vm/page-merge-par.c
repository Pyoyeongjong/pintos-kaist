#include "tests/main.h"
#include "tests/vm/parallel-merge.h"

int a;
void
test_main (void) 
{
  parallel_merge ("child-sort", 123);
}
