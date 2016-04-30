all: src doc

src: bochs pintos

bochs:
	cd src; make bochs

pintos:
	cd src; make pintos

doc:
	cd doc; make all

clean:
	cd src; make clean
	cd doc; make clean

