
#ifndef _hash_h_
#define _hash_h_

#include <stdint.h>

struct KEY { 
  int16_t x,y,z;
};

struct ITEM {
  KEY key;
  void *payload;  // 0=cella libera o chiave negativa
};


ITEM *hash_search( KEY key );
void hash_insert( KEY key , void *payload );

#endif // _hash_h_

