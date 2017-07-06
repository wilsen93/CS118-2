CXX=g++
CXXOPTIMIZE= -O2
CXXFLAGS= -g -Wall -pthread -std=c++11 $(CXXOPTIMIZE)
USERID=EDIT_MAKE_FILE

# Add all .cpp files that need to be compiled for your server
SERVER_FILES=server.cpp helper.h helper.cpp

# Add all .cpp files that need to be compiled for your client
CLIENT_FILES=client.cpp helper.h helper.cpp

all: server client

server: $(SERVER_FILES)
	$(CXX) $(CXXFLAGS) server.cpp helper.cpp -o server

client: $(CLIENT_FILES)
	$(CXX) $(CXXFLAGS) client.cpp helper.cpp -o client

clean:
	rm -rf *.o *~ *.gch *.swp *.dSYM server client *.tar.gz

tarball: clean
	tar -cvf $(USERID).tar.gz *
