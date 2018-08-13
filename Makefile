# the output file will be re-created whenever the file 'main.o' or 'parse.o' is changed
output: main.o parse.o
	# Recompile both files in executable file 'output'
	gcc main.o parse.o -o output -lreadline -lpthread

main.o: main.c header.h
	# Compile the file 'main.c' whenever 'main.c' or 'header.h' is changed
	gcc -c -lpthread main.c

parse.o: parse.c header.h
	gcc -c parse.c

# It deletes all the '* .o' files as well as the 'output'
clean:
	rm *.o output
