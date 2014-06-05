#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "states.h"
#include "modulus.h"

#define FILENAMELEN 64

uint64_t modulus = 0LL;

#define FINALSTATE (-1LL)

void writedelta(FILE *fp, uint64_t delta, uint64_t cnt)
{
  int c;

  do {
    c = delta & 0x7f;
    if ((delta >>= 7))
      c |= 0x80;
    if (fputc(c, fp) != c) {
      printf("failed to write delta\n");
      exit(1);
    }
  } while (delta);
  if (!fwrite(&cnt, sizeof(uint64_t),1,fp)) {
    printf("failed to write count\n");
    exit(1);
  }
}

void dumpstart(char *basename)
{
  FILE *fp;
  char inname[FILENAMELEN];
  int notok;

  sprintf(inname,"%s.0.0.0",basename);
  fp = fopen(inname, "w");
  writedelta(fp, startstate(), 1LL);
  writedelta(fp, FINALSTATE-startstate(), modulus-1LL);
  notok = fclose(fp);
  if (notok) {
    printf("Failed to close %s\n",inname);
    exit(1);
  }

}

int main(int argc, char *argv[])
{
  int modidx,wd;
  char outbase[64];

  if (argc!=3) {
    printf ("usage: %s width imod\n", argv[0]);
    exit(1);
  }
  wd = atoi(argv[1]);
  modidx = atoi(argv[2]);
  if (modidx < 0 || modidx >= NMODULI) {
    printf ("modulo_index %d not in range [0,%d)\n", modidx, NMODULI);
    exit(1);
  }
  modulus = -(uint64_t)modulusdeltas[modidx];
  sprintf(outbase,"state.%d.%d.0.0",wd,modidx);
  dumpstart(outbase);
  return 0;
}
