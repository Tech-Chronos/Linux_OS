.PHONY:all
all:mykill test

mykill:mykill.cc
	g++ -o $@ $^ -std=c++11
	
test:test.cc
	g++ -o $@ $^ -std=c++11

.PHONY:clean
clean:
	rm -f mykill test