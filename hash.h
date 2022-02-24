

#ifndef _hash_h_
#define _hash_h_


union KEY { 
  struct { short x,y; } xy;
  unsigned long u32;
  unsigned char u8[4];
};

struct ITEM {
  union KEY key;
  unsigned short timestamp;
  void *payload;  // 0=cella libera
};


struct ITEM *hash_search( union KEY key );
struct ITEM *hash_insert( union KEY key , unsigned int payload_size );
void hash_drop( union KEY key );
inline void *hash_get( int x , int y );

void hash_test();

#endif  // _hash_h_

