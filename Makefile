CPP = g++
CPPFLAGS = -O2 -I/usr/include/antlr4-runtime

LD = $(CPP)
LDFLAGS = -O2
LIBS = -lantlr4-runtime -lpthread

OBJS = \
.antlr/DistCLexer.o \
.antlr/DistCParser.o \
.antlr/DistCParserBaseVisitor.o \
.antlr/DistCParserVisitor.o \
DistCLexerBase.o \
DistCParserBase.o \
SymbolTable.o \
main.o

.PHONY: all clean

all: antlr4 main

antlr4:
	mkdir -p .antlr
	antlr4 -Dlanguage=Cpp -no-listener -no-visitor -o .antlr -lib .antlr DistCLexer.g4
	antlr4 -Dlanguage=Cpp -no-listener -visitor -o .antlr -lib .antlr DistCParser.g4

main: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)

%.o: %.cpp
	$(CPP) $(CPPFLAGS) -c $^ -o $@

clean:
	rm -rf .antlr main