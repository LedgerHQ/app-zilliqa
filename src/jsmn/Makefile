jsmn.exe : main.c ../txn_json_decode.c ../jsmn.h
	gcc -include txn_json_decode.h -ggdb -gdwarf-4 -Wall main.c ../txn_json_decode.c -o jsmn.exe

clean :
	-rm jsmn.exe

test : jsmn.exe
	./test.sh

