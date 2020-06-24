src=$(wildcard *.cc)
obj=$(patsubst %.cc,%.o,$(src))

ALL:test.out

test.out:$(obj)
	g++ $^ -o  $@ -lpthread

%.o:%.cc
	g++ -c -g  -std=c++11 $< -o $@ -lpthread 

clean:
	-rm -rf $(obj) test.out