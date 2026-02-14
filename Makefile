CXX := g++
CXXFLAGS := -Wall -std=c++17

TARGET := key_value_store
TEST_EXE := test

ALL: ${TARGET}

${TARGET}: main.o
			${CXX} ${CXXFLAGS} main.o -o ${TARGET}

main.o : main.cpp
		${CXX} ${CXXFLAGS} -c main.cpp

clean: 
		rm -f *.o ${TARGET} ${TEST_EXE}

del:
		rm -f *.txt ${TARGET} ${TEST_EXE}
${TEST_EXE}: test.cpp
	${CXX} ${CXXFLAGS} test.cpp -o ${TEST_EXE}

.PHONY: test

run_test: ${TEST_EXE}
	./${TEST_EXE}

run: ${TARGET}
	  ./${TARGET}