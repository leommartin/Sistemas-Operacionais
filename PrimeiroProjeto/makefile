CFLAGS = -Wall
 
testa_fila: testafila.c queue.c queue.h
	gcc -Wall testafila.c queue.c -o testa_fila
 
clean:
	-rm -f $(objs) *~
 
purge: clean
	-rm -f testa_fila
