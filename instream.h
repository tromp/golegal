#include <stdint.h>

#define MAXCPUS 32
#define FINALSTATE ((uint64_t)-1LL)
#define FILENAMELEN 96

typedef struct {
  FILE *fp;
  uint64_t state;
  uint64_t cnt;
  uint64_t cumcnt;
  char fname[FILENAMELEN];
} statebuf;

typedef struct goin goin;

uint64_t nstreams(goin *gin);
uint64_t totalread(goin *gin);
void deletemin(goin *gin);
statebuf *minstream(goin *gin);
goin *openstreams(char *inbase, int incpus, int ncpus, int cpuid, uint64_t modulus);
