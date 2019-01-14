CC = gcc
CFLAGS = -g -Wall -Wextra -Wswitch-enum -Wwrite-strings -pedantic 

TARGET = mysh
SOURCES = cmdexecution.c cmdhiearchy.c cmdlexer.c cmdparser.c cmdparsing.c \
		  main.c myshell.c run_prompt.c run_script.c signals.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS) 
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) -lreadline

clean:
	#rm -f cmdparser.c cmdparser.h cmdlexer.c cmdlexer.h
	rm -f *.o

%.o : %.c
	$(CC) $(CFLAGS) -c $<

cmdexecution.o: cmdexecution.h cmdhiearchy.h signals.h

cmdhiearchy.o: cmdhiearchy.h

cmdlexer%h cmdlexer%c: cmdlexer.l 
	flex cmdlexer.l

cmdlexer.o: cmdparser.h cmdhiearchy.h

cmdparser%h cmdparser%c: cmdparser.y cmdlexer.c
	bison -d cmdparser.y

cmdparser.o: cmdparser.h cmdhiearchy.h cmdlexer.h

cmdparsing.o: cmdparsing.h cmdhiearchy.h cmdlexer.h cmdparser.h

main.o: main.c myshell.h

run_prompt.o: run_prompt.h cmdexecution.h cmdhiearchy.h cmdparsing.h signals.h

run_script.o: run_script.h cmdexecution.h cmdhiearchy.h cmdparsing.h

myshell.o: myshell.h cmdparser.h cmdlexer.h cmdhiearchy.h cmdexecution.h \
		   signals.h run_prompt.h run_prompt.h cmdparsing.h

signals.o: signals.h

