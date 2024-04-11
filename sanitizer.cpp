#include "sanitizer.h"
#include <dlfcn.h>
#include <pthread.h>
#include <execinfo.h>
#include <iostream>

typedef int (*pthread_mutex_lock_func_t)(pthread_mutex_t*);

Graph mutexGraph;
thread_local MutexList mutexList(&mutexGraph);

extern "C" {

// your c code
// linkage naming

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    pthread_mutex_lock_func_t origin_func;
    origin_func = (pthread_mutex_lock_func_t)dlsym(RTLD_NEXT, "pthread_mutex_lock");

    uintptr_t m_addr = reinterpret_cast<uintptr_t>(mutex);
    mutexList.add(m_addr);
    
    if (mutexGraph.is_deadlock_detected(m_addr)){
      mutexGraph.print_deadlock_info();
    }


    return origin_func(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    pthread_mutex_lock_func_t origin_func;
    origin_func = (pthread_mutex_lock_func_t)dlsym(RTLD_NEXT, "pthread_mutex_unlock");

    uintptr_t m_addr = reinterpret_cast<uintptr_t>(mutex);
    mutexList.remove(m_addr);

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



// ------------------------------------------------
size_t MutexList::add(uintptr_t mutex) {
  if(!list.empty()) {
    uintptr_t from = list.back();
    if(graph) {
      graph->add_edge(from, mutex);
    }  
  }
  list.push_back(mutex);
  return list.size();
}

size_t MutexList::remove(uintptr_t mutex) {
  list.remove(mutex);
  return list.size();
}
//--------------------------------------------------

void Graph::add_edge(uintptr_t from, uintptr_t to) {
  sem_wait(&graph_mutex);
  adjacency_list[from].push_back(to);
  sem_post(&graph_mutex);
}

bool Graph::is_deadlock_detected(uintptr_t vertex)
{
  sem_wait(&graph_mutex);
    std::map<uintptr_t, bool> visited;
    std::map<uintptr_t, bool> rec_stack;
    for(const auto& vertex : adjacency_list) {
      visited[vertex.first] = false;
      rec_stack[vertex.first] = false;
    }
 
    bool result = is_cyclic_func(vertex, visited, rec_stack);
    
    if (result) deadlock_count++;

    sem_post(&graph_mutex);
    return result;
}

bool Graph::is_cyclic_func(uintptr_t v, std::map<uintptr_t, bool>& visited, std::map<uintptr_t, bool>& rec_stack)
{
    if (visited[v] == false) {
        // Mark the current node as visited and part of recursion stack
        visited[v] = true;
        rec_stack[v] = true;
 
        // Recur for all the vertices adjacent to this vertex
        std::list<uintptr_t> vtx = adjacency_list[v];
        std::list<uintptr_t>::iterator i;
        for (i = vtx.begin(); i != vtx.end(); ++i) {
            if (!visited[*i] && is_cyclic_func(*i, visited, rec_stack))
                return true;
            else if (rec_stack[*i])
                return true;
        }
    }
 
    // Remove the vertex from recursion stack
    rec_stack[v] = false;
    return false;
}

void Graph::print_deadlock_info() {
  void *array[3];
  char **strings;

  int size = backtrace (array, 3);
  strings = backtrace_symbols (array, size);
  if (strings != NULL) {
    //for (int i = 2; i < size; i++) // skip first and second records
    sem_wait(&print_mutex);
      std::cout << RED << "DEADLOCK possible in function..." << RESET << std::endl;
      std::cout << strings[2] << std::endl << std::endl;
      sem_post(&print_mutex);
  }

  free (strings);
}