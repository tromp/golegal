#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "states.h"
#include "sortstate.h"
#include "instream.h"
#include "outstream.h"
#include "modulus.h"
#include "modadd.h"

// ok for width 19 since 3*19 = 57 <= 5*LOCBITS
#define LOCBITS 12

int main(int argc, char *argv[])
{
  int incpus,ncpus,cpuid,modidx,width,y,x,nextx,tsizelen;
  uint64_t msize,modulus,nin;
  char c,*tsizearg,inbase[64];
  uint64_t laststate, nnewillcnt, newstates[3]; 
  int i,nnew,noutfiles;
  statebuf *mb;
  statecnt sn;
  jtset *jts;
  goin *gin;
  goout *go;

  assert(sizeof(uint64_t)==8);
  if (argc!=9 && argc!=10) {
    printf("usage: %s width modulo_index y x incpus ncpus cpuid maxtreesize[kKmMgG] [lastfile]\n",argv[0]);
    exit(1);
  }
  setwidth(width = atoi(argv[1]));
  modidx = atoi(argv[2]);
  if (modidx < 0 || modidx >= NMODULI) {
    printf ("modulo_index %d not in range [0,%d)\n", modidx, NMODULI);
    exit(1);
  }
  modulus = -(uint64_t)modulusdeltas[modidx];
  y = atoi(argv[3]);
  x = atoi(argv[4]);
  nextx = (x+1) % width;
  incpus = atoi(argv[5]);
  ncpus = atoi(argv[6]);
  if (ncpus < 1 || ncpus > MAXCPUS) {
    printf ("#cpus %d not in range [0,%d]\n", ncpus, MAXCPUS);
    exit(1);
  }
  cpuid = atoi(argv[7]);
  if (cpuid < 0 || cpuid >= ncpus) {
    printf("cpuid %d not in range [0,%d]\n", ncpus, ncpus-1);
    exit(1);
  }
  tsizelen = strlen(tsizearg = argv[8]);
  if (!isdigit(c = tsizearg[tsizelen-1]))
    tsizearg[tsizelen-1] = '\0';
   msize = atol(tsizearg);
  if (c == 'k' || c == 'K')
    msize *= 1000LL;
  if (c == 'm' || c == 'M')
    msize *= 1000000LL;
  if (c == 'g' || c == 'G')
    msize *= 1000000000LL;
  if (msize < 1000000LL) {
    printf("memsize %ld too small for comfort.\n", msize);
    exit(1);
  }
  if (argc > 9) {
    assert(sscanf(argv[9],"%d.%lo", &noutfiles, &laststate) == 2);
    noutfiles++;
    printf("skipping %d output files and states up to %lo\n", noutfiles, laststate);
  } else {
    noutfiles = 0;
    laststate = 0L;
  }

  sprintf(inbase,"%d.%d/yx.%02d.%02d",width,modidx,y,x); 
  gin = openstreams(inbase, incpus, ncpus, cpuid, modulus, laststate);
  go = goinit(width, modidx, modulus, y+!nextx, nextx, ncpus, cpuid);

  nnewillcnt = nin = 0LL;
  jts = jtalloc(msize, modulus, LOCBITS);
  if (nstreams(gin)) {
    for (; (mb = minstream(gin))->state != FINALSTATE; nin++,deletemin(gin)) {
      sn.cnt = mb->cnt;
      // printf("expanding %llo\n", mb->state);
      nnew = expandstate(mb->state, x, newstates);
      for (i=0; i<nnew; i++) {
        sn.state = newstates[i];
        //printf("inserting %llo\n", sn.state);
        jtinsert(jts, &sn);
      }
      if (nnew < 3) // nnew == 2
        modadd(modulus, &nnewillcnt, mb->cnt);
      if (jtfull(jts))
        dumpstates(go, jts, noutfiles++, mb->state);
    }
  }
  dumpstates(go, jts, noutfiles++, FINALSTATE);
  jtfree(jts);

  printf("(%d,%d) size %lu xsize %lu mod ",y,x,nin,totalread(gin));
  if (modulus)
    printf("%lu",modulus);
  else printf("18446744073709551616");

  printf("\nnewillegal %lu ",nnewillcnt);
  printf(       "needy %lu ", needywritten(go));
  printf(       "legal %lu ",legalwritten(go));
  printf("at (%d,%d)\n",y+(nextx==0),nextx);

  return 0;
}
