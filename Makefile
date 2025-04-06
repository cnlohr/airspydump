all : airspydump

airspydump : airspydump.c
	gcc -o $@ $^ -lairspy

clean :
	rm -rf airspydump
