#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "sortstate.h"
#include "modadd.h"

#define NPAIRPOW 13
#define NBLOCKPOW (32-NPAIRPOW)
#define NPAIRS ((1<<NPAIRPOW)-1)
#define NILBLOCK ((1<<NBLOCKPOW)-1)

typedef struct {
  int previous;
  statecnt pairs[NPAIRS];
} stateblock;

typedef struct {
  unsigned last : NBLOCKPOW;
  unsigned npairs : NPAIRPOW;
} locator;

locator nullocator = {NILBLOCK,NPAIRS};

struct jtset {
  locator *insert,*stream;
  stateblock *blocks;
  int nblocks;
  int nusedblocks;
  int freeblocks;
  int thrblocks;
  locator *nextloc;
  stateblock *nextblock;
  statecnt *prevpair;
  statecnt *nextpair;
  statecnt *lastpair;
  uint64_t modulus;
  int locbits;
  int locmask;
};

jtset *jtalloc(long nbytes, uint64_t mod, int locbits)
{
  jtset *jts;
  int i,nlocators;

  jts = (jtset *)calloc(1,sizeof(jtset));
  jts->modulus = mod;
  jts->locbits = locbits;
  nlocators = 1<<locbits;
  jts->locmask = nlocators-1;
  jts->nblocks = nbytes/sizeof(stateblock);
  if (jts->nblocks < nlocators) {
    printf("fewer blocks (%d) than locators (%d); use more memory\n", jts->nblocks,nlocators);
    exit(1);
  }
  jts->thrblocks = 4*jts->nblocks/5;
  jts->blocks = (stateblock *)calloc(jts->nblocks, sizeof(stateblock));
  if (jts->blocks == NULL) {
    printf("Failed to allocate %d * %d.\n", jts->nblocks, (int)sizeof(stateblock));
    exit(1);
  }
  jts->nusedblocks = 0;
  jts->blocks[0].previous = NILBLOCK;
  for (i=1; i< jts->nblocks; i++)
    jts->blocks[i].previous = i-1;
  jts->freeblocks = jts->nblocks-1;
  jts->insert = (locator *)calloc(nlocators, sizeof(locator));
  jts->stream = (locator *)calloc(nlocators, sizeof(locator));
  if (jts->insert == NULL || jts->stream == NULL) {
    printf("Failed to allocate %d * %d bytes.\n", nlocators, (int)sizeof(locator));
    exit(1);
  }
  for (i=0; i< nlocators; i++)
    jts->insert[i] = jts->stream[i] = nullocator;
  return jts;
}

int jtempty(jtset *jts)
{
  return (jts->nusedblocks == 0);
}

int jtfull(jtset *jts)
{
  return (jts->nusedblocks >= jts->thrblocks);
}

int jtsize(jtset *jts)
{
  int sum=0;
  locator *bin;
  int prev;

  for (bin = jts->insert; bin <= &jts->insert[jts->locmask]; bin++)
    if (bin->last != NILBLOCK) {
      sum += bin->npairs;
      for (prev=jts->blocks[bin->last].previous; prev != NILBLOCK; prev=jts->blocks[prev].previous)
        sum += NPAIRS;
    }
  return sum;
}

void freeblock(jtset *jts, stateblock *block)
{
//printf("%lx.freeblock(%d->%d)\n", jts, (int)(block - jts->blocks), block->previous);
  block->previous = jts->freeblocks;
  jts->freeblocks = block - jts->blocks;
  jts->nusedblocks--;
}

int getblock(jtset *jts)
{
  int oldfree = jts->freeblocks;

//printf("%lx.getblock(%d->%d)\n", jts, oldfree, jts->blocks[oldfree].previous);
  if (oldfree == NILBLOCK) {
    printf("Out of free blocks.\n");
    exit(1);
  }
  jts->freeblocks = jts->blocks[oldfree].previous;
  jts->nusedblocks++;
//printf("%lx->freeblocks = %d\n", jts, jts->freeblocks);
  return oldfree;
}

statecnt *jtinsertat(jtset *jts, statecnt *sb, locator *loc)
{
  int i,free;
  stateblock *fb;
  statecnt *ret;

//printf("%lx.insertat(%llo,%o)\n", jts, sb->state, (int)(loc-jts->insert));
  if ((i=loc->npairs)==NPAIRS) {
//printf("nusedblocks = %d\n", jts->nusedblocks);
    fb = &jts->blocks[free = getblock(jts)];
    fb->previous = loc->last;
    loc->last = free;
    loc->npairs = 0;
  } else fb = &jts->blocks[loc->last];
  ret = &fb->pairs[loc->npairs++];
  *ret = *sb;
  return ret;
}

statecnt *jtinsert(jtset *jts, statecnt *sb)
{
  return jtinsertat(jts, sb, &jts->insert[sb->state & jts->locmask]);
}

void sortblock(jtset *jts, stateblock *block, int npairs, locator *pairmap1, int shift)
{
  int i;
  statecnt *sb;

//printf("sorting block %d\n", (int)(block-jts->blocks));
  if (block->previous != NILBLOCK)
    sortblock(jts, &jts->blocks[block->previous], NPAIRS, pairmap1, shift);
  for (i=0; i<npairs; i++) {
    sb = &block->pairs[i];
    jtinsertat(jts, sb, &pairmap1[sb->state >> shift & jts->locmask]);
  }
  freeblock(jts,block);
}

void sortstates(jtset *jts, int keywidth)
{
  int digit;
  locator *bin, *tmp;

//printf("sorting all blocks\n");
  for (digit=jts->locbits; digit<keywidth; digit+=jts->locbits) {
    tmp = jts->stream; jts->stream = jts->insert;  jts->insert = tmp;
    for (bin=jts->stream; bin <= &jts->stream[jts->locmask]; bin++)
      if (bin->last != NILBLOCK) {
        sortblock(jts, &jts->blocks[bin->last],bin->npairs, jts->insert, digit);
        *bin = nullocator;
    }
  }
//printf("all blocks sorted\n");
}

void jtstartforat(jtset *jts, locator *bin)
{
  stateblock *block;
  int prev,next;

  jts->nextloc = bin;
  prev = bin->last;
  next = NILBLOCK;
  for (;;) {
    block = &jts->blocks[prev];
    prev = block->previous;
    block->previous = next;
    if (prev == NILBLOCK) break;
    next = block - jts->blocks;
  }
  jts->nextblock = block;
  jts->lastpair = (jts->nextpair = block->pairs) +
                  (next != NILBLOCK ? NPAIRS : bin->npairs);
}

statecnt *jtnext1(jtset *jts)
{
  locator *bin;
  stateblock *block;
  int next;

//printf("%lx.jtnext1()\n", jts);
  if (jts->nextpair < jts->lastpair) {
//printf("%lx.jtnext1()a==%llo\n", jts,jts->nextpair->state);
    return jts->nextpair++;
  }
  next = (block = jts->nextblock)->previous;
  freeblock(jts, block);
  if (next != NILBLOCK) {
    jts->nextblock = block = &jts->blocks[next];
    jts->lastpair = (jts->nextpair = block->pairs) +
      (block->previous!=NILBLOCK ? NPAIRS : jts->nextloc->npairs);
//printf("%lx.jtnext1()b==%llo\n", jts,jts->nextpair->state);
    return jts->nextpair++;
  }
  bin = jts->nextloc;
  *bin = nullocator;
  while (++bin <= &jts->stream[jts->locmask]) 
    if (bin->last != NILBLOCK) {
      assert(bin->npairs > 0);
      jtstartforat(jts, bin);
//printf("%lx.jtnext1()c==%llo\n", jts,jts->nextpair->state);
      return jts->nextpair++;
    }
//printf("%lx.jtnext1()d==NULL\n", jts);
  return NULL;
}

void jtstartfor(jtset *jts, int keywidth)
{
  locator *bin, *tmp;

//printf("%lx.jtstartfor()\n", jts);
  sortstates(jts, keywidth);
  tmp = jts->stream; jts->stream = jts->insert;  jts->insert = tmp;
  for (bin=jts->stream; bin <= &jts->stream[jts->locmask]; bin++) {
    if (bin->last != NILBLOCK) {
      assert(bin->npairs > 0);
      jtstartforat(jts, bin);
      jts->prevpair = jtnext1(jts);
      return;
    }
  }
//printf("warning: empty for\n");
  jts->prevpair = NULL;
}

statecnt *jtnext(jtset *jts)
{
  statecnt *sb, *ret;

//printf("%lx.jtnext()\n", jts);
  if (jts->prevpair == NULL)
    return NULL;
  while ((sb = jtnext1(jts)) != NULL) {
    if (sb->state == jts->prevpair->state) {
      modadd(jts->modulus, &jts->prevpair->cnt, sb->cnt);
      continue;
    }
    ret = jts->prevpair;
    jts->prevpair = sb;
    assert(ret->state != 0LL);
//printf("%lx.jtnext()=%llo\n", jts, ret->state);
    return ret;
  }
  ret = jts->prevpair;
  jts->prevpair = sb;
//printf("%lx.jtnext()=%llo\n", jts, ret->state);
  return ret;
}

void jtfree(jtset *jts)
{
  free(jts->insert);
  free(jts->stream);
  free(jts->blocks);
  free(jts);
}
