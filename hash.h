
#ifndef _hash_h_
#define _hash_h_

union KEY { 
  struct { short x,y; } xy;
  unsigned long u32;
  unsigned char u8[4];
};

struct ITEM {
  union KEY key;
  void *payload;  // 0=cella libera
};


//void *hash_set( union KEY key , void *payload );
//// crea la chiave ed associa il payload
//// che deve essere gia allocato via malloc
//// ritorna sempre il parametro payload
//// associare la chiave a 0 equivale a eliminarla
//// sotto il cofano viene chiamata la free sul payload
//#define hash_drop( union KEY key ) hash_set(key,0)

//void *hash_get( union KEY key );
//// ritorna il payload associato
//// oppure 0 se la chiave non ha payload associati

//void hash_compact();
//// libera le chiavi senza 
//// 

struct ITEM *hash_search( union KEY key );
struct ITEM *hash_insert( union KEY key );
void hash_drop( union KEY key );

void hash_test();

#endif // _hash_h_

