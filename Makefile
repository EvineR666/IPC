
all:a.o b.o

a.o:Acomu.c comu.c comu.h
	gcc -o a.o Acomu.c comu.c comu.h

b.o:Bcomu.c comu.c comu.h
	gcc -o b.o Bcomu.c comu.c comu.h

clean:
	-rm a.o b.o
