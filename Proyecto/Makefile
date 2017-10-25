all: EchoServer_seq EchoServer_conc EchoClient

CFLAGS=-include /usr/include/errno.h

EchoServer_seq: EchoServer_seq.c

EchoServer_conc: EchoServer_conc.c

EchoClient: EchoClient.c

clean:
	rm -f *.o EchoServer_seq EchoServer_conc EchoClient
