CXX = clang++
CXXFLAGS = -I.
.PHONY: clean

aigo: *.cc
	$(CXX) $(CXXFLAGS) $^ -o aigo 

clean:
	rm -f *.o a.out aigo *plist
