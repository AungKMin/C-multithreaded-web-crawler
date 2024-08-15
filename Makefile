CC = gcc       # compiler
CFLAGS_XML2 = $(shell xml2-config --cflags)
CFLAGS_CURL = $(shell curl-config --cflags)
CFLAGS = -Wall $(CFLAGS_XML2) $(CFLAGS_CURL) -std=gnu99 -g
LD = gcc       # linker
LDFLAGS = -std=gnu99 -g   # debugging symbols in build
LDLIBS_XML2 = $(shell xml2-config --libs)
LDLIBS_CURL = $(shell curl-config --libs)
LDLIBS = $(LDLIBS_XML2) $(LDLIBS_CURL) -pthread # link with "curl-config --libs" output, and pthreads

LIB_UTIL = curl_xml.o stack.o hash.o p_stack.o
SRCS   = curl_xml.c stack.c hash.c p_stack.c
OBJS_FINDPNG = findpng2.o $(LIB_UTIL)

TARGETS = findpng2

all: ${TARGETS}

findpng2: $(OBJS_FINDPNG)
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 