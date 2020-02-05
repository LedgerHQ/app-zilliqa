CFLAGS=-ggdb -gdwarf-4 -Wall
LDFLAGS=

jsmn.exe : main.c ../txn_json_decode.c ../jsmn.h txn_json_decode.h
	gcc ${CFLAGS} main.c -c -o main.o
	gcc ${CFLAGS} -include txn_json_decode.h ../txn_json_decode.c -c -o txn_json_decode.o
	gcc ${LDFLAGS} main.o txn_json_decode.o -o jsmn.exe 

clean :
	-rm jsmn.exe *.o

test : jsmn.exe
	./test.sh

