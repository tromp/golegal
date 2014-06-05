#include <stdint.h>

typedef struct {
  uint64_t state;
  uint64_t cnt;
} statecnt;

typedef struct jtset jtset;

jtset *jtalloc(long nbytes, uint64_t mod, int lockbits);
int jtempty(jtset *jts);
int jtfull(jtset *jts);
int jtsize(jtset *jts);
statecnt *jtinsert(jtset *jts, statecnt *sb);
void jtstartfor(jtset *jts, int keywidth);
statecnt *jtnext(jtset *jts);
void jtfree(jtset *jts);
