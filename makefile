CC = g++
SRC = .
OBJ = .
BIN = .

CPPFLAGS=-std=c++17 -g2 -fpermissive
LDFLAGS=-g2
LDLIBS=-mwindows -lsdl2 -lOpenAL32 -lglew32 -lopengl32 -lfreeglut -lavcodec -lavformat -lswscale -lavutil -lsnappy ./rtmidi-master/.libs/librtmidi.dll.a ../nfd.lib -lole32 -liconv -lws2_32 -lboost_thread-mt -lboost_system-mt -lboost_filesystem-mt -lfreetype -ljpeg -lstrmiids ../Uuid.lib ../shcore.lib -lavdevice -loleaut32

$(BIN)/start.exe: $(OBJ)/loopstation.o $(OBJ)/node.o $(OBJ)/start.o
	$(CC) $(LDFLAGS) -o $(BIN)/start.exe $(OBJ)/loopstation.o $(OBJ)/node.o $(OBJ)/start.o $(LDLIBS) 

loopstation.o: $(SRC)/loopstation.h $(SRC)/loopstation.cpp
	$(CC) $(CPPFLAGS) -c $(SRC)/loopstation.cpp

node.o: $(SRC)/node.h $(SRC)/node.cpp
	$(CC) $(CPPFLAGS) -c $(SRC)/node.cpp

start.o: $(SRC)/start.cpp
	$(CC) $(CPPFLAGS) -c $(SRC)/start.cpp
