cpmRun: diskSimulator.o  cpmfsys.o fsysdriver.o 
	gcc -o cpmRun diskSimulator.o cpmfsys.o fsysdriver.o 

diskSimulator.o: diskSimulator.c diskSimulator.h
	gcc -c -std=c99 diskSimulator.c

cpmfsys.o: cpmfsys.h cpmfsys.c 
	gcc -c -std=c99 cpmfsys.c  

fsysdriver.o: fsysdriver.c
	gcc -c -std=c99 fsysdriver.c 

all: 
	cpmRun

clean: 
	rm *.o 

