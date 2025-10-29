
test.out:test.o
	g++ -o $@ $^ -std=c++11

test.o:test.s
	g++ -o $@ -c $^

test.s:test.i
	g++ -o $@ -S $^

test.i:test.cc
	g++ -o  $@ -E $^


.PHONY:clean
clean:
	rm -rf test.out test.o test.s test.i
