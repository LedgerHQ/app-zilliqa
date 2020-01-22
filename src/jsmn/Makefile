jsmn.exe : main.c ../jsmn.c ../txn_msg.h
	gcc -ggdb -gdwarf-4 -Wall main.c ../jsmn.c -o jsmn.exe

clean :
	-rm jsmn.exe

test : jsmn.exe
	./test.sh

