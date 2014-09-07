CC = g++
STD = c++11
OPTIMIZE = -O3
-include local.mk
CFLAGS = -std=$(STD) -Wall -Wextra $(DEBUG) $(OPTIMIZE)
LFLAGS = $(DEBUG)
EXEC = predictors
OBJS = predictors.o main.o

OUTPUTS = $(addprefix outputs/, $(addsuffix _output.txt, short_trace1 short_trace2 short_trace3))

default: exec
test: $(OUTPUTS)
exec: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LFLAGS) -o $(EXEC) $(OBJS) 
	
predictors.o: predictors.h predictors.cc predictors.cpp
	$(CC) $(CFLAGS) -c predictors.cpp

main.o: metatemplates.h predictors.h predictors.cc main.cpp
	$(CC) $(CFLAGS) -c main.cpp

$(OUTPUTS): tester.py $(EXEC)
	./tester.py

clean:
	rm -rf $(EXEC) $(OBJS) outputs
