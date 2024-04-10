SHELL := /bin/bash

all: *.cpp
	$(CXX) mytests.cpp -std=c++17 -g -O3 -Werror -Wall -Wextra -pthread -pedantic -rdynamic -o mytests

run_tests:
	./testrun.sh

clang_format:
	diff -u <(cat *.cpp *.h tests/*.cpp) <(clang-format *.cpp *.h tests/*.cpp)

clang_format_fix:
	clang-format -i *.cpp *.h tests/*.cpp

clang_tidy:
	clang-tidy *.cpp tests/*.cpp -checks=-*,clang-analyzer-*,-clang-analyzer-cplusplus* -- -I tests -I . -std=c++17 -g -O3 -Werror -Wall -Wextra -pthread -pedantic

clean:
	rm -f tests/mytests

