#include <dlfcn.h>
#include <pthread.h>
#include <execinfo.h>
#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <atomic>
#include <list>
#include <map>
#include <semaphore.h>

typedef int (*pthread_mutex_lock_func_t)(pthread_mutex_t*);

static thread_local std::list<uintptr_t> locked_mutex; 

// static std::multimap<uintptr_t, uintptr_t> adjacency_list; 
 

// static int get_tid();
void add_adjacency_list2(uintptr_t from, uintptr_t to);
void print_result();
void print_result2();
void add_mutex(uintptr_t mutex);
void remove_mutex(uintptr_t mutex);


class Graph {
  std::map<uintptr_t, std::list<uintptr_t> > adjacency_list;

  sem_t graph_mutex;

  bool isCyclicFunc(uintptr_t v, std::map<uintptr_t, bool>& visited, std::map<uintptr_t, bool>& rec_stack);
  int getSize() { return adjacency_list.size(); }

public:
    Graph() { sem_init(&graph_mutex, 0, 1); }

    void addEdge(uintptr_t v, uintptr_t w);
    bool isCyclic(uintptr_t vertex);
    void print_stack_trace();

} mutexGraph;

extern "C" {

// your c code
// linkage naming
void __attribute__((destructor)) unload() {
  // print_result();
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    pthread_mutex_lock_func_t origin_func;
    origin_func = (pthread_mutex_lock_func_t)dlsym(RTLD_NEXT, "pthread_mutex_lock");

    uintptr_t m_addr = reinterpret_cast<uintptr_t>(mutex);
    add_mutex(m_addr);
    
    // print_stack_trace();
    // mutexGraph.print_result();
    if (mutexGraph.isCyclic(m_addr)){
      std::cout << "DEADLOCK detected!!!" << std::endl;
      mutexGraph.print_stack_trace();
    }


    return origin_func(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    pthread_mutex_lock_func_t origin_func;
    origin_func = (pthread_mutex_lock_func_t)dlsym(RTLD_NEXT, "pthread_mutex_unlock");

    uintptr_t m_addr = reinterpret_cast<uintptr_t>(mutex);
    remove_mutex(m_addr);

    return origin_func(mutex);
}

}

// int get_tid() {
//   static std::atomic<int> next_id{0};
//   static thread_local std::unique_ptr<int> tid_ptr;

//   if (tid_ptr == nullptr) {
//     tid_ptr = std::unique_ptr<int>(new int());
//     *tid_ptr = ++next_id;
//   }
//   return *tid_ptr;
// }

void add_mutex(uintptr_t mutex) {
  if(!locked_mutex.empty()) {
    uintptr_t from = locked_mutex.back();
    mutexGraph.addEdge(from, mutex);
  }
  locked_mutex.push_back(mutex);
}

void remove_mutex(uintptr_t mutex) {
  locked_mutex.remove(mutex);
}

// ------------------------------------------------

void Graph::addEdge(uintptr_t from, uintptr_t to) {
  // std::cout << "add adjacency list: from -> 0x" << std::hex << from << " to -> 0x" << std::hex << to << std::endl;
  sem_wait(&graph_mutex);
  adjacency_list[from].push_back(to);
  sem_post(&graph_mutex);
  // print_result();
}


// void Graph::print_result() {
//   std::cout << "adjacency list " << adjacency_list.size() << std::endl << "================" << std::endl;
  
//   for(const auto& entity : adjacency_list) {
//     std::cout << "0x" << std::hex << entity.first << " -> ";
    
//     for(const auto& item : entity.second) {
//       std::cout << " 0x" << std::hex << item;
//     }

//     std::cout << std::endl;
//   }
// }


bool Graph::isCyclic(uintptr_t vertex)
{
  sem_wait(&graph_mutex);
    std::map<uintptr_t, bool> visited;
    std::map<uintptr_t, bool> rec_stack;
    for(const auto& vertex : adjacency_list) {
      visited[vertex.first] = false;
      rec_stack[vertex.first] = false;
    }
 
    bool result = isCyclicFunc(vertex, visited, rec_stack);
    sem_post(&graph_mutex);
    return result;
}

bool Graph::isCyclicFunc(uintptr_t v, std::map<uintptr_t, bool>& visited, std::map<uintptr_t, bool>& rec_stack)
{
    if (visited[v] == false) {
        // Mark the current node as visited
        // and part of recursion stack
        visited[v] = true;
        rec_stack[v] = true;
 
        // Recur for all the vertices adjacent to this
        // vertex
        std::list<uintptr_t> vtx = adjacency_list[v];
        std::list<uintptr_t>::iterator i;
        for (i = vtx.begin(); i != vtx.end(); ++i) {
            if (!visited[*i]
                && isCyclicFunc(*i, visited, rec_stack))
                return true;
            else if (rec_stack[*i])
                return true;
        }
    }
 
    // Remove the vertex from recursion stack
    rec_stack[v] = false;
    return false;
}

void Graph::print_stack_trace() {
  void *array[5];
  char **strings;
  int size, i;

  size = backtrace (array, 5);
  strings = backtrace_symbols (array, size);
  if (strings != NULL) {
    std::cout << "---------------------------------------\n";
    for (i = 2; i < size; i++) // skip first and second records
      std::cout << strings[i] << std::endl;
  }

  free (strings);
}