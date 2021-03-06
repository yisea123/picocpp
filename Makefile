CC=gcc
CFLAGS=-Wall -pedantic -g -DUNIX_HOST -DVER=1.0 -std=C++11
LIBS=-lm -lreadline

TARGET	= picoc
SRCS	= picoc.cpp table.cpp lex.cpp parse.cpp expression.cpp heap.cpp type.cpp \
	variable.cpp clibrary.cpp platform.cpp include.cpp debug.cpp \
	platform/platform_unix.cpp platform/library_unix.cpp \
	cstdlib/stdio.cpp cstdlib/math.cpp cstdlib/string.cpp cstdlib/stdlib.cpp \
	cstdlib/time.cpp cstdlib/errno.cpp cstdlib/ctype.cpp cstdlib/stdbool.cpp \
	cstdlib/unistd.cpp
OBJS	:= $(SRCS:%.c=%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

test:	all
	(cd tests; make test)

clean:
	rm -f $(TARGET) $(OBJS) *~

count:
	@echo "Core:"
	@cat picoc.h interpreter.h picoc.cpp table.cpp lex.cpp parse.cpp expression.cpp platform.cpp heap.cpp type.cpp variable.cpp include.cpp debug.cpp | grep -v '^[ 	]*/\*' | grep -v '^[ 	]*$$' | wc
	@echo ""
	@echo "Everything:"
	@cat $(SRCS) *.h */*.h | wc

.PHONY: clibrary.cpp

picoc.o: picoc.cpp picoc.h
table.o: table.cpp interpreter.h platform.h
lex.o: lex.cpp interpreter.h platform.h
parse.o: parse.cpp picoc.h interpreter.h platform.h
expression.o: expression.cpp interpreter.h platform.h
heap.o: heap.cpp interpreter.h platform.h
type.o: type.cpp interpreter.h platform.h
variable.o: variable.cpp interpreter.h platform.h
clibrary.o: clibrary.cpp picoc.h interpreter.h platform.h
platform.o: platform.cpp picoc.h interpreter.h platform.h
include.o: include.cpp picoc.h interpreter.h platform.h
debug.o: debug.cpp interpreter.h platform.h
platform/platform_unix.o: platform/platform_unix.cpp picoc.h interpreter.h platform.h
platform/library_unix.o: platform/library_unix.cpp interpreter.h platform.h
cstdlib/stdio.o: cstdlib/stdio.cpp interpreter.h platform.h
cstdlib/math.o: cstdlib/math.cpp interpreter.h platform.h
cstdlib/string.o: cstdlib/string.cpp interpreter.h platform.h
cstdlib/stdlib.o: cstdlib/stdlib.cpp interpreter.h platform.h
cstdlib/time.o: cstdlib/time.cpp interpreter.h platform.h
cstdlib/errno.o: cstdlib/errno.cpp interpreter.h platform.h
cstdlib/ctype.o: cstdlib/ctype.cpp interpreter.h platform.h
cstdlib/stdbool.o: cstdlib/stdbool.cpp interpreter.h platform.h
cstdlib/unistd.o: cstdlib/unistd.cpp interpreter.h platform.h
