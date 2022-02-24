

// come hash.cpp
// ma implementata via stl::map


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hash.h"
#include <map>


#define LOG_NOBR(FMT,...) fprintf(stdout,"%s:%d(%s) " FMT , __FILE__ , __LINE__ , __func__ , ##__VA_ARGS__ )
#define LOG(FMT,...) LOG_NOBR(FMT "\n", ##__VA_ARGS__ )
#define FATAL(FMT,...) LOG(FMT,##__VA_ARGS__),exit(1)
#define COUNT(ARR) (sizeof(ARR)/sizeof(ARR[0]))



struct KEY_compare {
  bool operator() ( const struct KEY &a , const struct KEY &b ) const {
    return a.x == b.x && a.y == b.y && a.z == b.z ;
  }
};




typedef std::map< struct KEY , struct ITEM , KEY_compare >          item_from_key_t;

item_from_key_t     item_from_key;





struct ITEM *hash_search( struct KEY key ){
  struct ITEM *item = &item_from_key[key];
//  LOG("item %s key %d:%d:%d payload %s",item?"√":"×",item->key.x,item->key.y,item->key.z,item->payload?"√":"×");
  if(item->payload) LOG("valued key %d:%d:%d",key.x,key.y,key.z);
  else              LOG(  "null key %d:%d:%d",key.x,key.y,key.z);
  return item;
}



void hash_insert( struct KEY key , void *payload ){
  struct ITEM *item = &item_from_key[key];
  item->key = key;
  item->payload = payload;
}



// alla fine insert e search sono IDENTICHE
// tutto dipende dal payload associato



static void hash_print(){
//  int slot=0;
//  int y;
//  for( y=0 ; y<4 ; y++ ){
//    int x;
//    for( x=0 ; x<64 ; x++ , slot++ ){
//      putchar( slots[slot].head ? 'x' : '.' );
//    }
//    putchar('\n');
//  }
//  for( slot=0 ; slot<COUNT(slots) ; slot++ ){
//    int len = slots[slot].key_count;
//    LOG_NOBR("slot %3d keys %3d neg %3d ",slot,len,slots[slot].neg_count);
//    for(;len>0;len--) putchar('.');
//    putchar('\n');
//  }
}





#include "rdtsc.h"
#include <inttypes.h>


int main(){
  int i;
  uint64_t rdtsc_start,rdtsc_stop;

  // inseriamo tot chiavi
  LOG("insert");
  rdtsc_start=rdtsc();
  {
    struct KEY key;
    key.x=0;
    key.y=0;
    key.z=0;
    hash_insert( key , (void*)main );
  }
  for(i=0;i<10000;i++){
    struct KEY key;
    key.x=rand();
    key.y=1;
    key.z=1;
    hash_insert( key , (void*)main );
  }
  rdtsc_stop=rdtsc();
//  hash_print();
  LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);

  LOG("search");
  for(i=0;i<3;i++){
    struct KEY key;
    key.x=0;
    key.y=0;
    key.z=0;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  LOG("search");
  for(i=0;i<3;i++){
    struct KEY key;
    key.x=-1;  // ho solo messo chiavi positive
    key.y=-1;
    key.z=-1;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  LOG("search");
  for(i=0;i<3;i++){
    struct KEY key;
    key.x=0;
    key.y=0;
    key.z=0;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  LOG("search");
  for(i=0;i<3;i++){
    struct KEY key;
    key.x=-1;  // ho solo messo chiavi positive
    key.y=-1;
    key.z=-1;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  hash_print();
  return 0;
}

