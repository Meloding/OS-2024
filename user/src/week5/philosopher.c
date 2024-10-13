#include "philosopher.h"

// TODO: define some sem if you need

int sems[PHI_NUM];

void init() {
  // init some sem if you need
  // TODO();
  for(int i = 0; i < PHI_NUM; i++){
    sems[i] = sem_open(1);
  }
}

void philosopher(int id) {
  // implement philosopher, remember to call `eat` and `think`
  while (1) {
    // TODO();
    int l = id, r = (id + 1) % PHI_NUM;
    if (l < r) {
      l ^= r;
      r ^= l;
      l ^= r;
    }
    sem_p(sems[l]);
    sem_p(sems[r]);
    
    eat(id);

    sem_v(sems[l]);
    sem_v(sems[r]);
    
    think(id);
  }
}
