CC=gcc
DOC=doxygen
DOC_FILE=Doxyfile
CFLAGS=-c -fPIC -Wall -D_GNU_SOURCE -D_DEBUG `pkg-config --cflags glib-2.0`
INCLUDES=
LDFLAGS=-D_GNU_SOURCE
LIBRARIES=`pkg-config --libs glib-2.0` -pthread -lsctp
SOURCES=src/sctp_multi_test.c src/sctp_association.c src/sctp_multi.c \
src/tb_common.c src/tb_logging.c src/tb_epoll.c \
src/sctp_socket.c
OBJECTS=$(SOURCES:.c=.o)
SO_LIB=libsctpmulti.so.1.0
SO_FLAGS=-shared -Wl,-soname,libsctpmulti.so.1
EXECUTABLE=test_multi
all: $(SOURCES) $(EXECUTABLE) $(SO_LIB) 

executable: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBRARIES) -o $@

shared: $(SOURCES) $(SO_LIB)

$(SO_LIB) : $(OBJECTS)
	$(CC) $(SO_FLAGS) $(LDFLAGS) $(OBJECTS) $(LIBRARIES) -o $@

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

check:
	./$(EXECUTABLE)
clean:
	rm -rf src/*.o libsctpmulti.so.1.0 test_multi
doxygen:
	$(DOC) $(DOC_FILE)
install:
	export LD_LIBRARY_PATH=$(DIR):$$LD_LIBRARY_PATH
