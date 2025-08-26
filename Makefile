CFLAGS := -I./include
LDFLAGS := -luv -lamqpcpp -lpthread

publisher_uv.exe: examples/libuv/publisher.cc 
	g++ ${CFLAGS} $+ -o $@ ${LDFLAGS}
subsripber_uv.exe: examples/libuv/suscripber.cc 
	g++ ${CFLAGS} $+ -o $@ ${LDFLAGS}

clean:
	rm -vf *.exe
