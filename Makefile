CXX = clang++
CXXFLAGS = -I. -O2
ANALYZE = --analyze
.PHONY: clean analyze

aigo: *.cc
	$(CXX) $(CXXFLAGS) $^ -o aigo 

debug: *.cc
	$(CXX) $(CXXFLAGS) -DDEBUG $^ -o debug 

clean:
	rm -f *.o a.out aigo *plist debug

analyze: *.cc
	$(CXX) $(CXXFLAGS) $(ANALYZE) $^ -o aigo 
