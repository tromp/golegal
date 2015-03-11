#include <unistd.h>
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

// ok for width 18 since 3*18 = 54 <= 5*LOCBITS
#define LOCBITS 11

int main(int argc, char *argv[])
{
  int ncpus,cpuid,modidx,width,y,x,nextx,tsizelen;
  uint64_t msize,modulus,totin,nin;
  char c,*tsizearg,inbase[64],outbase[64];
  uint64_t nnewillcnt, newstates[3]; 
  int i,nnew,noutfiles=0;
  statebuf *mb;
  statecnt sn;
  jtset *jts;
  goin *gin;
  goout *go;

  assert(sizeof(uint64_t)==8);
  if (argc!=8) {
    printf("usage: %s width modulo_index y x ncpus cpuid maxtreesize[kKmMgG]\n",argv[0]);
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
  ncpus = atoi(argv[5]);
  if (ncpus < 1 || ncpus > MAXCPUS) {
    printf ("#cpus %d not in range [0,%d]\n", ncpus, MAXCPUS);
    exit(1);
  }
  cpuid = atoi(argv[6]);
  if (cpuid < 0 || cpuid >= ncpus) {
    printf("cpuid %d not in range [0,%d]\n", ncpus, ncpus-1);
    exit(1);
  }
  tsizelen = strlen(tsizearg = argv[7]);
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

  sprintf(inbase,"state.%d.%d.%d.%d",width,modidx,y,x); 
  gin = openstreams(inbase, ncpus, cpuid, modulus);
  if (!nstreams(gin))
    return 0;
  go = goinit(width, modulus, nextx, ncpus, cpuid);
  sprintf(outbase,"state.%d.%d.%d.%d",width,modidx,y+(nextx==0),nextx);

  nnewillcnt = 0LL;
  jts = jtalloc(msize, modulus, LOCBITS);
  for (nin=0LL; (mb = minstream(gin))->state != FINALSTATE; nin++) {
    sn.cnt = mb->cnt;
//printf("expanding %llo\n", mb->state);
    nnew = expandstate(mb->state, x, newstates);
    for (i=0; i<nnew; i++) {
      sn.state = newstates[i];
//printf("inserting %llo\n", sn.state);
      jtinsert(jts, &sn);
    }
    if (nnew < 3) // nnew == 2
      modadd(modulus, &nnewillcnt, mb->cnt);
    if (jtfull(jts))
      dumpstates(go, jts, outbase, noutfiles++);
    deletemin(gin);
  }
  if (!jtempty(jts))
    dumpstates(go, jts, outbase, noutfiles++);
  jtfree(jts);

  hidefiles(go, outbase, noutfiles);

  totin = totalread(gin);
  printf("%d %d size %lu",y,x,nin);
  printf(" avg %1.3f mod ",totin/(double)nin);
  if (modulus)
    printf("%lu",modulus);
  else printf("18446744073709551616");

//  printf("\nnewillegal %lu ",nnewillcnt);
//  printf(       "needy %lu ", needywritten(go));
//  printf(       "legal %lu ",legalwritten(go));
//  printf("at (%d,%d)\n",y+(nextx==0),nextx);
  printf("\n");
  printf(      "%d %d %d ",width,y+(nextx==0),nextx);
  printf("newillegal %lu ",nnewillcnt);
  printf(     "needy %lu ",needywritten(go));
  printf(      "legal %lu",legalwritten(go));
  printf("\n");
  return 0;
}

