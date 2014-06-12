CFLAGS = -Wall -O3 -m64
GFLAGS = -Wall -g -m64

all:   	start legal legalg tar

start:	start.c states.c modulus.h Makefile
	cc $(CFLAGS) -o start start.c states.c

instream:	instream.c instream.h modadd.h modulus.h Makefile
	cc -DMAININSTREAM $(CFLAGS) -o instream instream.c modadd.c

outstream:	outstream.c sortstate.c sortstate.h partition.c partition.h modadd.h modulus.h Makefile
	cc -DMAINOUTSTREAM$(CFLAGS) -o outstream outstream.c sortstate.c partition.c states.c modadd.c

partition:	partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc -DMAINPARTITION $(CFLAGS)  -o partition partition.c states.c sortstate.c

legal:	legal.c instream.c instream.h outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(CFLAGS)  -o legal legal.c states.c sortstate.c instream.c outstream.c partition.c modadd.c

legalg:	legal.c instream.c instream.h outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(GFLAGS)  -o legalg legal.c states.c sortstate.c instream.c outstream.c partition.c modadd.c

merge:	merge.c instream.c instream.h outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(CFLAGS)  -o merge merge.c states.c sortstate.c instream.c outstream.c partition.c modadd.c

tar:	start.c legal.c instream.c instream.h outstream.c outstream.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile slegal modadd.c modadd.h CRT.hs NIC.hs README
	tar -zcf legal.tgz start.c legal.c instream.c instream.h outstream.c outstream.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile slegal modadd.c modadd.h CRT.hs NIC.hs README
