#include <stdlib.h>

typedef struct goout goout;

goout *goinit(int width, uint64_t modulus, int bump, int ncpus,int cpuid);
void dumpstates(goout *go, jtset *jts, char *outbase, int iteration, uint64_t state);
void hidefiles(goout *go, char *basename, int extension);
uint64_t needywritten(goout *go);
uint64_t legalwritten(goout *go);
void opendelta2(goout *go, char *fname);
void writerecord(goout *go, uint64_t state, uint64_t cnt);
void closedelta(goout *go);
