#include <stdint.h>

#define STARTSTATE 0LL

uint64_t startstate();

void setwidth(int statewidth);

// rotates through NSHOWBUF output buffers
// so it can be called multiple times in printf
char *showstate(uint64_t s, int x);

// return whether s encodes a legal final state or not
int finalstate(uint64_t s);

// fill new with successor states of s
// return number of new states, up to 3
int expandstate(uint64_t s, int x, uint64_t *news);
