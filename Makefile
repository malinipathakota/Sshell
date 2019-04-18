all: sshell

sshell: sshell.o sshell.c
	gcc -Wall -Werror -o sshell sshell.o

clean:
	rm -f sshell sshell.o