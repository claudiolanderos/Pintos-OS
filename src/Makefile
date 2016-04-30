all: bochs pintos

bochs:
	cd bochs-2.4.6; ./configure --enable-gdb-stub --with-term --with-nogui; make; sudo make install

pintos:
	cd pintos/src/threads; make

clean:
	cd bochs-2.4.6; make clean
	cd pintos/src/threads; make clean

