#include "Topics.h"
#include <cstdlib>

Topics::Topics(char* init){
  list = (char**) malloc(10 * sizeof(char*));
  list[0] = init;
  this->size = 1;
}

Topics::~Topics(void) {}

void Topics::addTopic(char* item) {
  char** buff = (char**) malloc((size + 1) * sizeof(char*));
  for (int i = 0; i < size; i++) {
    buff[i] = list[i];
  }
  buff[size] = item;
  list = buff;
  size = size + 1;
}

int Topics::getSize() {
  return this->size;
}

char** Topics::getTopics() {
  return this->list;
}
