CXX = clang++
CXXFLAGS = -I.
.PHONY: clean

aigo: *.cc
	$(CXX) $^ -o aigo 

clean:
	rm -f *.o aigo *plist
