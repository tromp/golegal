CFLAGS = -Wall -O3 -m64
GFLAGS = -Wall -g -m64


all:   	start legal legalg tar NIC

start:	start.c outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(CFLAGS) -o start start.c states.c sortstate.c outstream.c partition.c modadd.c

instream:	instream.c instream.h modadd.h modulus.h Makefile
	cc -DMAININSTREAM $(CFLAGS) -o instream instream.c modadd.c

outstream:	outstream.c sortstate.c sortstate.h partition.c partition.h modadd.h modulus.h Makefile
	cc -DMAINOUTSTREAM $(CFLAGS) -o outstream outstream.c sortstate.c partition.c states.c modadd.c

partition:	partition.c partition.h states.c states.h sortstate.c sortstate.h modadd.h Makefile
	cc -DMAINPARTITION $(CFLAGS)  -o partition partition.c states.c sortstate.c modadd.c

legal:	legal.c instream.c instream.h outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(CFLAGS)  -o legal legal.c states.c sortstate.c instream.c outstream.c partition.c modadd.c

legalg:	legal.c instream.c instream.h outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(GFLAGS)  -o legalg legal.c states.c sortstate.c instream.c outstream.c partition.c modadd.c

merge:	merge.c instream.c instream.h outstream.c modadd.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile
	cc $(CFLAGS)  -o merge merge.c states.c sortstate.c instream.c outstream.c partition.c modadd.c

NIC:	NIC.hs
	ghc --make -main-is NIC NIC.hs

clean: 
	rm *.o start legal legalg NIC.hi

tar:	start.c legal.c instream.c instream.h outstream.c outstream.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile slegal modadd.c modadd.h CRT.hs NIC.hs README
	tar -zcf legal.tgz start.c legal.c instream.c instream.h outstream.c outstream.h modulus.h partition.c partition.h states.c states.h sortstate.c sortstate.h Makefile slegal modadd.c modadd.h CRT.hs NIC.hs README
