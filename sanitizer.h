#ifndef _SANITIZER_H_
#define _SANITIZER_H_

#include <semaphore.h>
#include <list>
#include <map>

#define RESET "\033[0m"
#define RED "\033[31m"

// TODO: write declation here
class Graph {
    int deadlock_count = 0;
  std::map<uintptr_t, std::list<uintptr_t> > adjacency_list;

  sem_t graph_mutex;
  sem_t print_mutex;

  bool is_cyclic_func(uintptr_t v, std::map<uintptr_t, bool>& visited, std::map<uintptr_t, bool>& rec_stack);
  int get_size() { return adjacency_list.size(); }

public:
    Graph() { 
      sem_init(&graph_mutex, 0, 1); 
      sem_init(&print_mutex, 0, 1); 
    }

    void add_edge(uintptr_t v, uintptr_t w);
    bool is_deadlock_detected(uintptr_t vertex);
    void print_deadlock_info();

    int get_deadlock_count() { return deadlock_count; }
};

class MutexList {
  std::list<uintptr_t> list; 
  Graph* graph = nullptr;

public:
  MutexList(Graph* graph) {
    this->graph = graph;
  };

  size_t add(uintptr_t mutex);
  size_t remove(uintptr_t mutex) ;
};

#endif