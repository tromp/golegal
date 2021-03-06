#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include "states.h"
#include "partition.h"
#include "sortstate.h"
#include "outstream.h"
#include "modadd.h"

#define MAXCPUS 32
#define FINALSTATE ((uint64_t)-1LL)
#define FILENAMELEN 96

struct goout {
  int width;
  uint64_t modulus;
  int bump;
  int ncpus;
  int cpuid;
  uint64_t parts[MAXCPUS];
  char outbase[FILENAMELEN];
  char outname[FILENAMELEN];
  char tmpoutname[FILENAMELEN];
  uint64_t bufstate;
  FILE *fpdelta;
  uint64_t nneedy;
  uint64_t nlegal;
  uint64_t cumneedy;
  uint64_t cumlegal;
};

void setpartition(goout *go)
{
  int i,dmin,d,t;
  long size,part,cum,lim,tot;

  assert(go->ncpus <= MAXCPUS);
  go->parts[go->ncpus-1] = FINALSTATE;
  if (go->ncpus == 1)
    return;
  allowall();
  tot = bordercnt(go->width,go->bump);
  dmin = go->width*2/3;
  part = tot/go->ncpus;
  printf("width=%d bump=%d tot=%jd part=%jd\n", go->width,go->bump,(uintmax_t)tot,(uintmax_t)part);
  for (i=1; i<go->ncpus; i++) {
    go->parts[i-1] = 0;
    lim = tot * i/go->ncpus; // find (roughly) border-class of rank lim
    cum = 0;
    allowall();
    for (d=go->width; --d>=dmin ; ) {  // fix (octal) digits, from most to least significant
      for (t=0; t<8; t++) {
        setborder(go->bump,d,t);
        size = bordercnt(go->width,go->bump);
        // printf("d=%d t=%d size=%jd cum=%jd lim=%jd\n", d,t,(uintmax_t)size,(uintmax_t)cum,(uintmax_t)lim);
        if (cum+size > lim)
          break;
        cum += size;
      }
      go->parts[i-1] |= (uint64_t)t << (3*d);
      if (d==dmin && cum+size-lim < lim-cum) {
        cum += size;
        go->parts[i-1] += 1LL << (3*d);
      }
      // printf("  parts[%d]=%jd\n", i-1, (uintmax_t)(go->parts[i-1]));
    }
    // printf("parts[%d]=%jo, off by %jd\n", i-1, (uintmax_t)(go->parts[i-1]), (uintmax_t)(cum-lim));
  }
}

goout *goinit(int width, int modidx, uint64_t modulus, int y, int bump, int ncpus,int cpuid)
{
  goout *go;
  char formatname[FILENAMELEN];
  int i,rc;

  go = (goout *)calloc(1,sizeof(goout));
  if (go == NULL) {
    printf("Failed to allocate goout record of size %d\n", (int)sizeof(goout));
    exit(1);
  }
  go->width = width;
  go->modulus = modulus;
  go->bump = bump;
  go->ncpus = ncpus;
  go->cpuid = cpuid;
  go->cumneedy = go->cumlegal = 0LL;
  setpartition(go);
  sprintf(go->outbase,"%d.%d/yx.%02d.%02d",width,modidx,y,bump);
  for (i=0; i<ncpus; i++) {
    sprintf(formatname,"%s/fromto.%d.%d", go->outbase, cpuid, i);
    rc = mkdir(formatname, 0755);
    if (rc && errno==EEXIST) {
      // possible determination of lastest processed state
    }
  }
  return go;
}

void opendelta(goout *go) // assumes outname has been set appropriately
{
  sprintf(go->tmpoutname,"%s.tmp", go->outname);
  go->fpdelta = fopen(go->tmpoutname, "w");
  if (!go->fpdelta) {
    perror("Failed to open for writing");
    puts(go->tmpoutname);
    exit(1);
  }
  go->bufstate = go->nneedy = go->nlegal = 0LL;
}

void writedelta(goout *go, uint64_t delta)
{
  int c;

  do {
    c = delta & 0x7f;
    if ((delta >>= 7LL))
      c |= 0x80;
    if (fputc(c, go->fpdelta) != c) {
      printf("failed to write delta\n");
      exit(1);
    }
  } while (delta);
}

void writerecord(goout *go, uint64_t state, uint64_t cnt)
{
  uint64_t delta = state - go->bufstate;

  assert(!go->bufstate || delta > 0);
  writedelta(go, delta);
  if (!fwrite(&cnt, sizeof(uint64_t),1,go->fpdelta)) {
    printf("failed to write count\n");
    exit(1);
  }
  modadd(go->modulus, finalstate(state) ? &go->nlegal : &go->nneedy, cnt);
  go->bufstate = state;
}

void closedelta(goout *go)
{
  uint64_t nsum,cnt;

  nsum = go->nneedy;
  modadd(go->modulus, &nsum, go->nlegal);
  cnt = nsum ? go->modulus - nsum : 0LL;
  writedelta(go, FINALSTATE - go->bufstate);
  if (!fwrite(&cnt, sizeof(uint64_t),1,go->fpdelta)) {
    printf("failed to write checksum\n");
    exit(1);
  }
  if (fclose(go->fpdelta)) {
    perror("Failed to close");
    puts(go->tmpoutname);
    exit(1);
  }
  if (rename(go->tmpoutname, go->outname)) {
    perror("Failed to rename");
    puts(go->tmpoutname);
    exit(1);
  }
  modadd(go->modulus, &go->cumneedy, go->nneedy);
  modadd(go->modulus, &go->cumlegal, go->nlegal);
}

void dumpstates(goout *go, jtset *jts, int iteration, uint64_t state)
{
  char formatname[FILENAMELEN];
  statecnt *sc;
  uint64_t prevstate;
  int i;

  sprintf(formatname,"%s/fromto.%d.%%d/%d.%jo", go->outbase, go->cpuid, iteration, (uintmax_t)state);
  jtstartfor(jts, 3*go->width);
  sc = jtnext(jts);
  prevstate = 0;
  for (i=0; i<go->ncpus; i++) {
    sprintf(go->outname,formatname, i);
    opendelta(go);
    while (sc != NULL && sc->state < go->parts[i]) {
      assert(sc->state==STARTSTATE || sc->state > prevstate);
      prevstate = sc->state;
      writerecord(go, sc->state, sc->cnt);
      sc = jtnext(jts);
    }
    closedelta(go);
  }
  assert(sc == NULL);
  assert(jtempty(jts));
}

uint64_t needywritten(goout *go)
{
  return go->cumneedy;
}

uint64_t legalwritten(goout *go)
{
  return go->cumlegal;
}

#ifdef MAINOUTSTREAM
#include "modulus.h"
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
  int modidx,y,x,nextx,tsizelen;
  long msize;
  char c,*tsizearg;
  int width, noutfiles=0;
  jtset *jts;
  statecnt sb;
  goout *go;
  uint64_t modulus;
  int ncpus, cpuid;

  if (argc!=8) {
    printf("usage: %s width modulo_index y x ncpus cpuid maxtreesize[kKmMgG]\n", argv[0]);
    exit(1);
  }
  width = atoi(argv[1]);
  modidx = atoi(argv[2]);
  if (modidx < 0 || modidx >= NMODULI) {
    printf("modulo_index %d not in range [0,%d)\n", modidx, NMODULI);
    exit(1);
  }
  modulus = -(uint64_t)modulusdeltas[modidx];
  y = atoi(argv[3]);
  x = atoi(argv[4]);
  nextx = (x+1) % width;
  ncpus = atoi(argv[5]);
  if (ncpus < 1 || ncpus > MAXCPUS) {
    printf("#cpus %d not in range [0,%d]\n", ncpus, MAXCPUS);
    exit(1);
  }
  cpuid = atoi(argv[6]);
  if (cpuid < 0 || cpuid >= ncpus) {
    fprintf(stderr, "cpuid %d not in range [0,%d]\n", ncpus, ncpus-1);
    exit(1);
  }
  go = goinit(width, modidx, modulus, y+!nextx, nextx, ncpus, cpuid);

  exit(0);

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
  if (msize < 100000LL) {
    printf("memsize %jd too small for comfort.\n", (uintmax_t)msize);
    exit(1);
  }

  jts = jtalloc(msize,modulus,13);
  while (fread(&sb.state,sizeof(uint64_t),2,stdin)==2) {
    jtinsert(jts, &sb);
    if (jtfull(jts))
      dumpstates(go, jts, noutfiles++, sb.state);
  }
  dumpstates(go, jts, noutfiles++, FINALSTATE);
  jtfree(jts);

  if (nextx==0)
    printf("legal(%dx%d) %% %ju = %ju\n",y+1,width,(uintmax_t)modulus,(uintmax_t)legalwritten(go));
  return 0;
}
#endif
