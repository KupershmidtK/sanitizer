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

static std::multimap<uintptr_t, uintptr_t> adjacency_list; 

static int get_tid();
void add_adjacency_list(uintptr_t from, uintptr_t to);
void print_result();
void add_mutex(uintptr_t mutex);
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

    // printf("Thread %d lock Mutex %p\n", get_tid(), (void*)mutex);

    uintptr_t m_addr = reinterpret_cast<uintptr_t>(mutex);
    // std::ostringstream address;
    // address << (void*)mutex;
    // std::string name = address.str();

    // std::cout << "Mutex " << name << std::endl;

    // uint64_t m_addr = std::stoull(name, nullptr, 16);

    add_mutex(m_addr);
    print_stack_trace();
    print_result();

    return origin_func(mutex);
    return 0;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    pthread_mutex_lock_func_t origin_func;
    origin_func = (pthread_mutex_lock_func_t)dlsym(RTLD_NEXT, "pthread_mutex_unlock");

    printf("Thread %d unlock mutex\n", get_tid() );
    return origin_func(mutex);
    return 0;
}

}

// your c++ code
int get_tid() {
  static std::atomic<int> next_id{0};
  static thread_local std::unique_ptr<int> tid_ptr;

  if (tid_ptr == nullptr) {
    tid_ptr = std::unique_ptr<int>(new int());
    *tid_ptr = ++next_id;
  }
  return *tid_ptr;
}

void add_mutex(uintptr_t mutex) {
  static thread_local std::list<uintptr_t> locked_mutex; 
  // add to list
  //std::cout << "add mutex stack -> 0x" << std::hex << mutex << std::endl;
  if(!locked_mutex.empty()) {
    uintptr_t from = locked_mutex.back();
    std::cout << "back stack -> 0x" << std::hex << from << " add mutex stack -> 0x" << std::hex << mutex << std::endl;
    adjacency_list.insert( {from, mutex} );
  }
  locked_mutex.push_back(mutex);
}

void add_adjacency_list(uintptr_t from, uintptr_t to) {
  std::cout << "add adjacency list: from -> 0x" << std::hex << from << " to -> 0x" << std::hex << to << std::endl;

  
  // adjacency_list.insert(std::make_pair(1, 2));
}

void print_result() {
  std::cout << "adjacency list " << adjacency_list.size() << std::endl << "================" << std::endl;
  
  for(const auto& entity : adjacency_list) {
    std::cout << "Mutex 0x" << std::hex << entity.first << " : 0x" << std::hex << entity.second << std::endl;
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