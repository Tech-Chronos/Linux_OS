.PHONY:all
all:proc_sub mycode

proc_sub:proc_sub.cc
	g++ -o $@ $^ -std=c++11

mycode:mycode.cc
	g++ -o $@ $^ -std=c++11
.PHONY:clean
clean:
	rm -rf proc_sub