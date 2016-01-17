#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include "states.h"

#define MAXSTATEWIDTH 21 // 3 bits per cell fits in 64 bits

typedef struct {
  unsigned char type;
  unsigned char color;
  unsigned char left;
  unsigned char right;
} cell;

cell edge0 = {0,0,0,0};

//possible cell types
#define EDGE  0
#define EMPTY 1
#define COLOR 1
#define BLACK 0
#define WHITE 1
#define LIBSTONE 2
#define INCOMPAT 1
#define NEEDY 4
#define HASR 2
#define HASL 1
#define SENTINEL HASL
#define SENTI(s) (int)((s) & SENTINEL)
#define CELLCHARS "#.XO"
#define ISNEEDY(x) (((x) & NEEDY) != 0)
#define ALLONES 01111111111111111111111LL
#define NSET(t,x,d) t = ((t) & (~(7LL<<(3*(x))))) | ((uint64_t)(d)<<(3*(x)))

#define NSHOWBUF 4

typedef cell bstate[MAXSTATEWIDTH]; // borderstate

int statewidth; // global to save us from passing it to every function
int thirdwidth,twothirdwidth;
uint64_t twothirdmask, thirdmask;

void setwidth(int wd)
{
  if (wd < 0 || wd > MAXSTATEWIDTH) {
    printf ("width %d out of range [0,%d]\n", wd, MAXSTATEWIDTH);
    exit(1);
  }
  statewidth = wd;
  thirdmask = (1LL << (3*(thirdwidth = statewidth/3))) - 1LL;
  twothirdmask = (1LL << (3*(twothirdwidth = statewidth - statewidth/3))) - 1LL;
}

uint64_t startstate()
{
  return STARTSTATE;
}

int wordtostate(uint64_t s, int bump, bstate state)
{
  int stack[MAXSTATEWIDTH]; // holds pairs of (groupleftend,grouprightend)
  int sp,i,type,leftcolor;  // at most statewidth ends on stack
  cell *sti;
                                                                                
  leftcolor = SENTI(s); // sentinel
  for (sti = &state[i = sp = 0]; i < statewidth; sti++, i++) {
    sti->type = type = s & 7;
    s >>= 3;
    if (ISNEEDY(type)) {
      sti->color = leftcolor ^ COLOR; // assume opposite color
      if ((type & HASL) && i) {
        state[sti->left = stack[--sp]].right = i; // extend group
        if (sti->left == i-1) // assumption was wrong
          sti->color = leftcolor;
      } else stack[sp++] = i; // new left end
      if (type & HASR)
        stack[sp++] = i;      // new right end
      else state[sti->right = stack[--sp]].left = i; // connect ends
      if (i == bump)
        sti->color = WHITE; // color normalization
      leftcolor = sti->color;
    } else leftcolor = type & COLOR;
  }
  return sp==0;
}

char *showstate(uint64_t s, int bump)
{
  static char buffers[NSHOWBUF][MAXSTATEWIDTH+1],*buf; // +1 for '\0'
  static int bufnr = 0;
  int nc,type,i,ngroups[2];
  bstate state;
  uint64_t decode(uint64_t, int);
                                                                                
  s = decode(s, bump);
  wordtostate(s, bump, state);
  buf = buffers[bufnr++];
  if (bufnr == NSHOWBUF)
    bufnr = 0; // buffer rotation
  for (i = ngroups[0] = ngroups[1] = 0; i < statewidth; i++) {
    type = state[i].type;
    if (!ISNEEDY(type))
      buf[i] = CELLCHARS[type];
    else if (i && (type & HASL))
      buf[i] = buf[state[i].left];
    else { nc= state[i].color; buf[i] = "Aa"[nc] + ngroups[nc]++; }
  }
  buf[statewidth] = '\0';
  return buf;
}

uint64_t mustliberatecell(uint64_t t, bstate state, int x, int nc)
{
  int i = x, libtype = LIBSTONE | nc;

  do NSET(t,i,libtype);
  while ((i = state[i].left) != x);
  return t;
}

uint64_t liberatecell(uint64_t t, bstate state, int x)
{
  return ISNEEDY(state[x].type) ?
    mustliberatecell(t, state, x, state[x].color) : t;
}
                                                                                
uint64_t flipstones(uint64_t t)
{
  t ^= (((~t) >> 2) & (t >> 1) & ALLONES);
  if (t & NEEDY)
    t ^= SENTINEL;
  return t;
}

uint64_t encode(uint64_t t, int bump, bstate state)
{
  uint64_t t1;

  if (bump == statewidth)
    bump = 0;
  if (ISNEEDY(t >> (3*bump))) {
    if (state[bump].color == BLACK)
      t = flipstones(t);
  } else if ((t1 = flipstones(t)) < t)
    t = t1;
  if (bump >= twothirdwidth)
    t = (t >> 3*thirdwidth) | ((t & thirdmask) << 3*twothirdwidth);
  return t;
}

uint64_t reverse(uint64_t s)
{
  uint64_t s0, t,t1;
  bstate state;
  int i;

  for (s0=s,t=0,i=statewidth; i--; s0>>=3) {
    int pt = (s0 & 7);
    if (pt==5 || pt==6)
      pt = 5+6-pt;
    t = t<<3 | pt;
  }
  if (ISNEEDY(t)) {
    wordtostate(s, 0, state);
    if (state[statewidth-1].color == BLACK)
      t ^= (((~t) >> 2) & (t >> 1) & ALLONES);
  } else if ((t1 = flipstones(t)) < t)
    t = t1;
  return t;
}

uint64_t decode(uint64_t s, int bump)
{
  if (bump >= twothirdwidth)
    s = (s >> 3*twothirdwidth) | ((s & twothirdmask) << 3*thirdwidth);
  return s;
}

int finalstate(uint64_t s)
{
  return !(s & (NEEDY * ALLONES));
}

int expandstate(uint64_t s, int x, uint64_t *news)
{
  int nnew=0, col;
  cell left, up, edge;
  bstate state;
  uint64_t t;
                                                                                
#ifdef SHOWEXPAND
  printf("exp(s=%3llo (%s), x=%d, new)\n",0*s, showstate(s,x), x);
#endif
  s = decode(s, x);
  wordtostate(s, x, state);
  edge = edge0;
  left = x ? state[x-1] : edge;
  // extend border with liberty at (x,y)
  t = liberatecell(s, state, x);
  if (x > 0)
    t = liberatecell(t, state, x-1);
  NSET(t,x,EMPTY);
  news[nnew++] = encode(t, x+1, state);
  for (col=0; col<2; col++) {
    t = s; up = state[x];
    // extend border with stone at (x,y)
    if (ISNEEDY(up.type) && up.color != col) {
      if (up.left >= up.right) { // at either end
        if (up.left == x)        // singleton string
          continue;              // don't deprive last liberty
        t ^= up.left<x ? (uint64_t)HASR<<(3*up.left)
                       : (uint64_t)HASL<<(3*up.right);
      }
      up = edge;
    }
    if (left.type == EMPTY || left.type == (LIBSTONE|col)) {
      if (ISNEEDY(up.type)) // don't liberate edge shielded opposite
        t = mustliberatecell(t, state, x, col);
      NSET(t,x,LIBSTONE|col);
    } else if (up.type == EMPTY || up.type == (LIBSTONE|col)) {
      if (ISNEEDY(left.type) && left.color == col)
        t = mustliberatecell(t, state, x-1, col);
      NSET(t,x,(LIBSTONE|col));
    } else {
      if (!(ISNEEDY(up.type))) {
        NSET(t,x,up.type = NEEDY);
        up.left = x;
      }
      if (ISNEEDY(left.type) && left.color == col) {
        t |= ((uint64_t)HASL) << 3 * (up.left<x ? left.right : x);
        t |= ((uint64_t)HASR) << 3 * (left.right>x ? up.left: x-1);
      } else if (x == 0 && SENTI(t) == col)
        t ^= SENTINEL; // make sentinel opposite of col
    }
    news[nnew++] = encode(t, x+1, state);
  }
  return nnew;
}

#ifdef MAINSTATES
int main(int argc, char *argv[])
{
  int width,bump;
  uint64_t s,n;
  bstate state;

  assert(sizeof(uint64_t)==8);
  if (argc!=3) {
    printf("usage: %s width bump\n",argv[0]);
    exit(1);
  }
  setwidth(width = atoi(argv[1]));
  bump = atoi(argv[2]);
  for (s=n=0; s<(1L<<(3*width)); s++) {
    s = decode(s, bump);
    int ok = wordtostate(s, bump, state);
    if (!ok) continue;
    if (ISNEEDY(s >> (3*bump))) {
      if (state[bump].color == BLACK)
        continue;
    } else if (flipstones(s) < s)
      continue;
    n++;
  }
  printf("n=%jd\n", (uintmax_t)n);
  return 0;
}
#endif
