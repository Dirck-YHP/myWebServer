CXX = g++
CFLAGS = -std=c++14 -O2 -Wall -g

TARGET = server
OBJS = ../code/main.cpp ../code/buffer/*.cpp ../code/log/*.cpp \
	../code/server/*.cpp ../code/timer/*.cpp ../code/pool/*.cpp \
	../code/http/*.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o ../bin/$(TARGET)  -pthread -lmysqlclient

# clean:
# 	rm -rf ../bin/$(OBJS) $(TARGET)