


all : main

run : main
	xterm -e "./main &> LOG ; cat LOG ; read"


ALLSRC=$(shell find . -type f -name "*.[ch]" -or -name "*.cpp")

OBS += 1hash.o
1hash.o : makefile $(ALLSRC)



OBS += main.o
main.o : makefile $(ALLSRC)


main : makefile $(OBS)
	g++ -o $@ $(OBS)






		
clean cl :
	rm -f *.o


.PHONY : all clean cl box emu dbg vb


