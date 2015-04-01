#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "states.h"
#include "modulus.h"
#include "sortstate.h"
#include "outstream.h"

#define FINALSTATE ((uint64_t)-1LL)

int main(int argc, char *argv[])
{
  int wd,modidx;
  goout *go;
  jtset *jts;
  statecnt sc;
  char outbase[64];
  uint64_t modulus;

  if (argc!=3) {
    printf ("usage: %s width imod\n", argv[0]);
    exit(1);
  }
  wd = atoi(argv[1]);
  modidx = atoi(argv[2]);
  printf("%s %d %d\n", argv[0], wd, modidx);
  if (modidx < 0 || modidx >= NMODULI) {
    printf ("modulo_index %d not in range [0,%d)\n", modidx, NMODULI);
    exit(1);
  }
  modulus = -(uint64_t)modulusdeltas[modidx];
  sprintf(outbase,"%d.%d/yx.00.00",wd,modidx);
  go = goinit(wd, modulus, 0, 1, 0);
  jts = jtalloc(65536, modulus, 1);
  sc.state = startstate();
  sc.cnt = 1L;
  jtinsert(jts, &sc);
  dumpstates(go, jts, outbase, 0, FINALSTATE);
  return 0;
}
