CXX=g++
AR=ar
GTEST_INCLUDES=-I/home/rni/gmock-svn/gtest/include
GTEST_LIBS=-pthread
INCLUDES=-I. -I/usr/include $(GTEST_INCLUDES)  -I/home/ninou14fr/Downloads/boost_1_64_0/
FLAGS=-Wall -g -std=gnu++11
DEFINES=-DHAVE_CONFIG_H
LIBRARY_INCLUDES=-L/usr/lib -L. -L/home/rni/gmock-svn/

COMMON_LIBS=-lcurl

UTIL_OBJS=util/HttpRequestFactory.o util/HttpRequest.o util/OAuth.o util/OAuth2.o
DROPBOX_OBJS=DropboxAccountInfo.o DropboxMetadata.o DropboxRevisions.o \
	DropboxApi.o DropboxApi2.o
OBJS=$(UTIL_OBJS) $(DROPBOX_OBJS)

all:  libdropbox.a main
	$(CXX) $(INCLUDES) $(GTEST_INCLUDES) $(FLAGS) $(LIBRARY_INCLUDES) $(DEFINES) \
     main.o libdropbox.a $(COMMON_LIBS) $(GTEST_LIBS) -o tester
main:
	$(CXX) $(INCLUDES) $(FLAGS) $(DEFINES) -c main.cpp -o main.o

libdropbox.a: $(OBJS)
	$(AR) rcs libdropbox.a $(OBJS)

%.o: %.cpp
	$(CXX) $(INCLUDES) $(FLAGS) $(DEFINES) -c $< -o $@

util/%.o: util/%.cpp
	$(CXX) $(INCLUDES) $(FLAGS) $(DEFINES) -c $< -o $@

.PHONY : clean
clean:
	rm -f *.o util/*.o test
