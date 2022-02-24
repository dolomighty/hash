
#ifndef _hash_h_
#define _hash_h_

union KEY { 
  struct { short x,y; } xy;
  unsigned long u32;
  unsigned char u8[4];
};

struct ITEM {
  union KEY key;
  struct ITEM *next;  // 0=fine catena
  unsigned int timestamp;
  // payload
  unsigned char cmap[16][16];
  unsigned char hmap[16][16];
};


struct ITEM *hash_get( union KEY key );
struct ITEM *hash_set( union KEY key );
void hash_drop( union KEY key );

void hash_test();

#endif // _hash_h_

