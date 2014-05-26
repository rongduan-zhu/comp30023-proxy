CC = gcc
CFLAGS = -Wall -Wextra -g -O0 -pipe -std=c99 -Wno-packed-bitfield-compat -Wpointer-arith -Wformat-nonliteral -Winit-self -Wshadow -Wcast-qual -Wmissing-prototypes
EXE = proxy
LIBS = -lsocket -lnsl
PSRC = proxy.c
POBJ = proxy.o

sched: $(POBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(POBJ) $(LIBS)

clean:
	/bin/rm $(POBJ)

clobber:
	/bin/rm $(POBJ) $(EXE)

