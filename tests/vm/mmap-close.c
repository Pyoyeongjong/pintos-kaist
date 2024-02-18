/* Verifies that memory mappings persist after file close. */

#include <string.h>
#include <syscall.h>
#include "tests/vm/sample.inc"
#include "tests/arc4.h"
#include "tests/lib.h"
#include "tests/main.h"

#define ACTUAL ((void *) 0x10000000)

void
test_main (void)
{
  int handle;
  void *map;

  //printf(" actual=%x",ACTUAL);

  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  CHECK ((map = mmap(ACTUAL, 4096, 0, handle, 0)) != MAP_FAILED, "mmap \"sample.txt\"");

  //printf(" actual=%c",*(char *)ACTUAL);

  close (handle);
    //_close

  if (memcmp (ACTUAL, sample, strlen (sample)))
    fail ("read of mmap'd file reported bad data");

  munmap (map);

  //printf(" end ");
}
