#ifdef MAINPARTITION
#include <stdio.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "sortstate.h"
#include <assert.h>

#define MAXSTATEWIDTH 21
#define EDGE  0
#define EMPTY 1
#define LIBSTONE 2
#define COLOR  1
#define BLACK  0
#define WHITE  1
#define NEEDY 8
#define HASR 4
#define NEEDYR (NEEDY|HASR)
#define HASL 2
#define NEEDYL (NEEDY|HASL)
#define NEEDYLR (NEEDY|HASL|HASR)
#define TYPEMASK (NEEDYLR|WHITE)
// to normalize color in border class, we require either of the following properties
#define NORM 16       // most significant libstone was black
#define NDYBUMP 32    // there was a needy stone at bump
#define SETEDGE 64    // consider bump-1 bump non-adjacent
#define STIFT 7       // stack shift
#define TOP1 (1<<STIFT) // empty stack
#define STACKMASK (-1<<STIFT)

int bump, borderlen;

int expandborder(uint64_t s, int x, uint64_t *news)
{
  int prev,col,ndybump,norm,znorm,se,nnew=0;
  uint64_t stack;

  prev = (s & SETEDGE) ? EDGE : s & TYPEMASK;
  norm = s & NORM;
  ndybump = s & NDYBUMP;
  stack = s & STACKMASK;
  se = (x==bump-1) ? SETEDGE : 0;
  if (prev < NEEDY)
    news[nnew++] = stack | se | ndybump | norm | EMPTY;
  for (col=0; col<2; col++) {
    if (prev < NEEDY || (prev&COLOR) != col)
      news[nnew++] = stack | ndybump | (col ? 0 : NORM) | LIBSTONE | col |
         ((x==bump-1 && (prev==EMPTY || prev==(LIBSTONE|col))) ? SETEDGE : 0);
    if (x == bump) {
      if (col == BLACK)
        continue;
      ndybump = NDYBUMP; // since col==WHITE, no need to reset for next iteration
    }
    // no HASL, so prev must not be same color (libstone or needy)
    if (prev == EDGE || (prev != EMPTY && (prev&COLOR) != col)) {
      znorm = x ? norm : col ? NORM : 0; // only black sentinel is normalized
      news[nnew++] =  stack           | se | ndybump | znorm | NEEDY  | col;
      news[nnew++] = (stack<<1|col<<STIFT) | ndybump | znorm | NEEDYR | col;
      // must be considered adjacent across bump
    }
    // top link must match col and prev is neither empty nor friendly lib nor friend without HASR
    if (stack > TOP1 && (int)(stack>>STIFT & 1) == col &&
         !((prev == EMPTY || prev == (LIBSTONE|col) ||
           (prev&(NEEDYR|COLOR)) == (NEEDY|col)))) {
      se = (x==bump-1 && ((prev&NEEDYR)==NEEDYR)) ? SETEDGE : 0;
      news[nnew++] =  stack                 | se | ndybump | norm | NEEDYLR | col;
      news[nnew++] = (stack>>1 & STACKMASK) | se | ndybump | norm | NEEDYL  | col;
    }
  }
  return nnew;
}

short allow[MAXSTATEWIDTH];

void allowall()
{
  int i;

  for (i=0; i<MAXSTATEWIDTH; i++)
    allow[i] = 0xffff;
}

void setborder(int bump, int i, int type)
{
  int ix = i;

  if (bump >= borderlen - borderlen/3 && (ix -= borderlen/3) < 0)
    ix += borderlen;
  if (type & 4) {
    if (ix==0) {
      int col = !(type & 2); // opp color of virtual libstone at -1
      int hasr = type & 1;
      allow[ix] = 1 << (8 + 2*hasr + col);
    } else allow[ix] = 3 << (2*type); // allow either color
  } else allow[ix] = 1 << type;
}

int allowed(int i, uint64_t bs)
{
  return ((allow[i]>>(bs&TYPEMASK)) & 1) && !(bs >> (STIFT+borderlen-i));
}

#define STARTBORDER (TOP1 | NORM | EDGE)

void expandat(jtset *newt, statecnt *sc, int i)
{
  statecnt sn;
  int nnew,j;
  uint64_t news[9]; 

  sn.cnt = sc->cnt;
  nnew = expandborder(sc->state, i, news);
  for (j=0; j<nnew; j++) {
    if (!allowed(i, news[j]))
      continue;
    sn.state = news[j];
    jtinsert(newt,&sn);
  }
}

uint64_t bordercnt(int len, int bmp)
{
  jtset *order;
  uint64_t tot; 
  int size,i,keybits;
  statecnt sb, *sc;

  borderlen = len;
  bump = bmp;
  keybits = 8+(len/2);
  size = 2000000;
  order = jtalloc(size,0LL,4);
  sb.state = STARTBORDER; sb.cnt = 1LL;
  jtinsert(order,&sb);
  for (i=0; i<borderlen; i++) {
    jtstartfor(order,keybits);
    while ((sc = jtnext(order)) != NULL) {
      //assert(sc->state != 0LL);
      //printf("%llo\n", sc->state);
      expandat(order, sc, i);
    }
  }
  tot = 0LL;
  jtstartfor(order,keybits);
  while ((sc = jtnext(order)) !=NULL) {
    if (sc->state & (NDYBUMP|NORM))
      tot += sc->cnt;
  }
  jtfree(order);
  return tot;
}

#ifdef MAINPARTITION
int main(int argc, char *argv[])
{
  uint64_t tot;

  if (argc!=3) {
    printf ("usage: %s borderlen bump\n", argv[0]);
    exit(1);
  }
  borderlen = atoi(argv[1]);
  bump = atoi(argv[2]);
//  printf("n #borders #nbits\n");
  allowall();
  tot = bordercnt(borderlen, bump);
  printf("%llu\n", tot);
}
#endif
