#Simple Makefile
LANG = -std=c++11
LIBS = -lcrypto -lstdc++ 
PARAMS = -Wall

all: server client

debug: clean server_debug client_debug

server_debug: server.cpp
	clang $(LANG) $(PARAMS) -g $(LIBS) -o server_debug server.cpp md5sum.cpp

client_debug: client.cpp
	clang $(LANG) $(PARAMS) -g $(LIBS) -o client_debug client.cpp md5sum.cpp

server: server.cpp
	clang $(LANG) $(PARAMS) $(LIBS) -o server server.cpp md5sum.cpp

client: client.cpp
	clang $(LANG) $(PARAMS) $(LIBS) -o client client.cpp md5sum.cpp

filecopy: filecopy.cpp
	clang $(LANG) $(PARAMS) $(LIBS) -o filecopy filecopy.cpp

clean:
	rm -f server client filecopy

clean_debug:
	rm -f server_debug client_debug

clean_all: 
	rm -f server_debug client_debug server client filecopy
