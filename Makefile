CXX = clang++
CXXFLAGS = -I. -O2
ANALYZE = --analyze

.PHONY: clean analyze all

all: ansi gtp

ansi: ansi.cc go.h
	$(CXX) $(CXXFLAGS) ansi.cc -o ansi 

clean:
	rm -f *.o a.out gtp ansi *plist debug

gtp: gtp.cc go.h
	$(CXX) $(CXXFLAGS) gtp.cc -o gtp
