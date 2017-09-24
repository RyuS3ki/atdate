Simple echo TCP client and servers.


In this directory:

* EchoClient.c - Client

* EchoServer_seq.c - Sequential server

* EchoServer_conc.c - Concurrent server


To build:

make clean; make


To run:

./EchoClient host port

./EchoServer_seq port

./EchoServer_conc [port]
