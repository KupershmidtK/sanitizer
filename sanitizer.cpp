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
void print_stack_trace();

class Graph {
  std::map<uintptr_t, std::list<uintptr_t> > adjacency_list;

  sem_t graph_mutex;

  bool isCyclicFunc(uintptr_t v, bool visited[], bool* rs);
  int getSize() { return adjacency_list.size(); }

public:
    Graph() { sem_init(&graph_mutex, 0, 1); }

    void addEdge(uintptr_t v, uintptr_t w);
    bool isCyclic();
    void print_result();
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
    mutexGraph.print_result();

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
  //std::cout << "add mutex stack -> 0x" << std::hex << mutex << std::endl;

  if(!locked_mutex.empty()) {
    uintptr_t from = locked_mutex.back();
    mutexGraph.addEdge(from, mutex);
  }
  locked_mutex.push_back(mutex);
}

void remove_mutex(uintptr_t mutex) {
  locked_mutex.remove(mutex);
}

void print_stack_trace() {
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {

    printf ("Obtained %d stack frames.\n", size);
    for (i = 0; i < size; i++)
      printf ("%s\n", strings[i]);
  }

  free (strings);
}

// ------------------------------------------------

void Graph::addEdge(uintptr_t from, uintptr_t to) {
  // std::cout << "add adjacency list: from -> 0x" << std::hex << from << " to -> 0x" << std::hex << to << std::endl;
  sem_wait(&graph_mutex);
  adjacency_list[from].push_back(to);
  sem_post(&graph_mutex);
}


void Graph::print_result() {
  std::cout << "adjacency list " << adjacency_list.size() << std::endl << "================" << std::endl;
  
  for(const auto& entity : adjacency_list) {
    std::cout << "0x" << std::hex << entity.first << " -> ";
    
    for(const auto& item : entity.second) {
      std::cout << " 0x" << std::hex << item;
    }

    std::cout << std::endl;
  }
}


bool Graph::isCyclic()
{
  sem_wait(&graph_mutex);

  int size = getSize();
  bool* visited = new bool[size];
  bool* recStack = new bool[size];
  for (int i = 0; i < size; i++) {
      visited[i] = false;
      recStack[i] = false;
  }

  // Call the recursive helper function
  // to detect cycle in different DFS trees
  for (int i = 0; i < size; i++)
      if (!visited[i] && isCyclicFunc(i, visited, recStack)) {
          sem_post(&graph_mutex);
          return true;
      }

  sem_post(&graph_mutex);
  return false;
}

bool Graph::isCyclicFunc(uintptr_t v, bool visited[], bool* recStack)
{
    if (visited[v] == false) {
        // Mark the current node as visited
        // and part of recursion stack
        visited[v] = true;
        recStack[v] = true;
 
        // Recur for all the vertices adjacent to this
        // vertex
        std::list<uintptr_t>::iterator i;
        for (i = adjacency_list[v].begin(); i != adjacency_list[v].end(); ++i) {
            if (!visited[*i]
                && isCyclicFunc(*i, visited, recStack))
                return true;
            else if (recStack[*i])
                return true;
        }
    }
 
    // Remove the vertex from recursion stack
    recStack[v] = false;
    return false;
}