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

typedef int (*pthread_mutex_lock_func_t)(pthread_mutex_t*);

static thread_local std::list<uintptr_t> locked_mutex; 

static std::multimap<uintptr_t, uintptr_t> adjacency_list; 
static std::map<uintptr_t, std::list<uintptr_t> > adjacency_list2; 

static int get_tid();
void add_adjacency_list2(uintptr_t from, uintptr_t to);
void print_result();
void print_result2();
void add_mutex(uintptr_t mutex);
void remove_mutex(uintptr_t mutex);
void print_stack_trace();

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
    print_result2();

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



void add_mutex(uintptr_t mutex) {
  //std::cout << "add mutex stack -> 0x" << std::hex << mutex << std::endl;

  if(!locked_mutex.empty()) {
    uintptr_t from = locked_mutex.back();
    // std::cout << "back stack -> 0x" << std::hex << from << " add mutex stack -> 0x" << std::hex << mutex << std::endl;
    adjacency_list.insert( {from, mutex} );
    add_adjacency_list2(from, mutex);
  }
  locked_mutex.push_back(mutex);
}

void remove_mutex(uintptr_t mutex) {
  locked_mutex.remove(mutex);
}

void add_adjacency_list2(uintptr_t from, uintptr_t to) {
  std::cout << "add adjacency list: from -> 0x" << std::hex << from << " to -> 0x" << std::hex << to << std::endl;

  adjacency_list2[from].push_back(to);
}


void print_result2() {
  std::cout << "adjacency list 2 " << adjacency_list2.size() << std::endl << "================" << std::endl;
  
  for(const auto& entity : adjacency_list2) {
    std::cout << "0x" << std::hex << entity.first << " -> ";
    
    for(const auto& item : entity.second) {
      std::cout << " 0x" << std::hex << item;
    }

    std::cout << std::endl;
  }
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