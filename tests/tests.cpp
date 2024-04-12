#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <sanitizer.h>
#include <pthread.h>
#include<unistd.h>
#include <ctime>
#include <cstdlib>


TEST_CASE("No deadlock philosophers") { 
    Graph graph;
    MutexList list(&graph);

    list.add(1);
    list.add(2);
    list.add(3);

    CHECK_UNARY_FALSE(graph.is_deadlock_detected(3)); 

    list.add(1);
    CHECK_UNARY(graph.is_deadlock_detected(1)); 
}
