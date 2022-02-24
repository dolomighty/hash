
#include <stdio.h>
#include <dos.h>
#include <malloc.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>

#define COUNT(ARR) (sizeof(ARR)/sizeof(ARR[0]))

// https://www.tutorialspoint.com/data_structures_algorithms/hash_table_program_in_c.htm
// la insert permetteva di inserir chiavi duplicate, fixata
// cablata per una hashtable di 256 celle (ma non credo ci sian benefici)
// hashing fn abbastanza isotropa
// ricerca slot per collisioni via hop variabile

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


struct ITEM hash_table[256]={0}; 
int available = 256;
unsigned short timestamp = 0;



void timestamp_rescale(){
  int i;
  timestamp >>= 8;
  for( i=0; i<COUNT(hash_table); i++ ){
    if(!hash_table[i].payload)continue;
    hash_table[i].timestamp >>= 8;
  }
}

inline unsigned short timestamp_gen(){
  if(timestamp>=50000) timestamp_rescale();
  return ++timestamp;
}



unsigned char hash_slot_from_key( union KEY key ){
  return key.u8[0]+key.u8[1]+key.u8[2]+key.u8[3];
//  return key.u8[0]^key.u8[1]^key.u8[2]^key.u8[3];

//  asm mov cl , byte ptr key+0
//  asm rol cl , cl
//  asm add cl , byte ptr key+1
//  asm rol cl , cl
//  asm add cl , byte ptr key+2
//  asm rol cl , cl
//  asm add cl , byte ptr key+3
//  asm rol cl , cl
//  asm mov byte ptr key+0 , cl
//  return key.u8[0];

//  asm mov al , byte ptr key+0
//  asm rol al , 2
//  asm add al , byte ptr key+1
//  asm rol al , 2
//  asm add al , byte ptr key+2
//  asm rol al , 2
//  asm add al , byte ptr key+3
//  asm rol al , 2
//  asm mov byte ptr key+0 , al
//  return key.u8[0];   // 748 collisioni su 1000 chiavi range xy +-200

  // cmq tutti questi algo sono grossomodo equivalenti in quanto a collisioni
  // anche perche lo spazio degli slot della hastable è ristretto
}




long wasted = 0;
char coll_flag = 0;



struct ITEM *hash_search( union KEY key ){

  unsigned char slot = hash_slot_from_key(key);
  unsigned char hop = 1+slot*2;
  if(hop==0)hop++;

  // cerca partendo da slot
  while( hash_table[slot].payload ){
    if( hash_table[slot].key.u32 == key.u32 ){
      // trovato
      hash_table[slot].timestamp = timestamp_gen();
      return &hash_table[slot]; 
    }
    // ritentiamo
    coll_flag = 1;
    wasted ++;
    printf("hash_search: key %d,%d collision with slot %d\n",key.xy.x,key.xy.y,slot);
    slot=slot+hop;
  }        
  // item non trovato
  return 0;
}


struct ITEM *hash_insert( union KEY key , unsigned int payload_size ){
  
  // ritorna l'item associato alla chiave
  // tenta di allocare spazio per il payload
  // qualunque errore, return 0

  unsigned char slot;
  unsigned char hop;
  void *payload;

  // questo check evita loop infiniti nella gestione delle collisioni
  if(!available){
    printf("hash_insert: hash table full\n");
    return 0;
  }

  slot = hash_slot_from_key(key);
  hop = 1+slot*2;   
  if(hop==0)hop++;

  // cicliamo sulle celle usate partendo da slot
  while( hash_table[slot].payload ){
    if( hash_table[slot].key.u32 == key.u32 ){
      // la key c'è già, aggiorniamo
      printf("hash_insert: update key %d,%d\n",key.xy.x,key.xy.y);
      free(hash_table[slot].payload);
      break;
    } 
    coll_flag = 1;
    wasted ++;
    printf("hash_insert: key %d,%d collision with slot %d (%d,%d)\n"
      ,key.xy.x,key.xy.y
      ,slot
      ,hash_table[slot].key.xy.x,hash_table[slot].key.xy.y
    );
    // go to next cell
    slot=slot+hop;
  }

  payload = malloc(payload_size);
  if(!payload){
    printf("!payload\n");
    return 0;
  }

  printf("payload @ %04X:%04X\n",FP_SEG(payload),FP_OFF(payload));

  available --;
  hash_table[slot].key = key;
  hash_table[slot].payload = payload;
  hash_table[slot].timestamp = timestamp_gen();
  return &hash_table[slot];
}



void hash_delete( union KEY key ){
  // get the hash 
  unsigned char slot = hash_slot_from_key(key);
  
  unsigned char hop = 1+slot*2;
  if(hop==0)hop++;

  // cerca tra le celle quella richiesta, partendo da slot
  // essenzialmente, facciamo una ricerca lineare
  while(1){
    printf("hash_delete: checking slot %d\n",slot);
    if( hash_table[slot].payload && hash_table[slot].key.u32 == key.u32 ){
      printf("hash_delete: freeing\n");
      free( hash_table[slot].payload );
      hash_table[slot].payload = 0;
      available ++;
      return;
    }
    // go to next cell
    slot=slot+hop;
  }      
//  printf("hash_delete: key %d,%d non trovata\n",key.xy.x,key.xy.y);
}




struct ITEM *hash_find( union KEY key , unsigned int payload_size ){
  
  // ritorna l'item associato alla chiave, se c'è gia
  // se non c'è, prova ad:
  // inserire la chiave (se non ce la fa, ciao)
  // allocare payload_size (se non ce la fa, ciao)
  // ritorna ITEM*

  unsigned char slot;
  unsigned char hop;
  void *payload;

  // questo check evita loop infiniti nella gestione delle collisioni
  if(!available){
    printf("hash_insert: hash table full\n");
    return 0;
  }

  slot = hash_slot_from_key(key);
  hop = 1+slot*2;   
  if(hop==0)hop++;

  // cicliamo sulle celle usate partendo da slot
  while( hash_table[slot].payload ){
    if( hash_table[slot].key.u32 == key.u32 ){
      // la key c'è già, aggiorniamo
      printf("hash_insert: update key %d,%d\n",key.xy.x,key.xy.y);
      free(hash_table[slot].payload);
      break;
    } 
    coll_flag = 1;
    wasted ++;
    printf("hash_insert: key %d,%d collision with slot %d (%d,%d)\n"
      ,key.xy.x,key.xy.y
      ,slot
      ,hash_table[slot].key.xy.x,hash_table[slot].key.xy.y
    );
    // go to next cell
    slot=slot+hop;
  }

  payload = malloc(payload_size);
  if(!payload){
    printf("!payload\n");
    return 0;
  }

  printf("payload @ %04X:%04X\n",FP_SEG(payload),FP_OFF(payload));

  available --;
  hash_table[slot].key = key;
  hash_table[slot].payload = payload;
  hash_table[slot].timestamp = timestamp_gen();
  return &hash_table[slot];
}







void display(){
  int i;
  for( i=0 ; i<256 ; i++ ){
    putchar( hash_table[i].payload ? 'x' : '.' );
    if(i%64==63) printf("\n");
  }
  printf("free %d%% wasted %ld\n",available*100/256,wasted);
}



struct ITEM *hash_oldest_item(){
  int i;
  int slot = -1;
  int min  = -1;
  for( i=0; i<COUNT(hash_table); i++ ){
    if(!hash_table[i].payload)continue;
//    printf("%d ",hash_table[i].timestamp);
    if( min>=0 && min<hash_table[i].timestamp )continue;
    min = hash_table[i].timestamp;
    slot = i;
  }
//  printf("oldest %d\n",min);
  return &hash_table[slot];
}



//int main(){
//  // valutiamo la bonta di hash_slot_from_key()
//  int n;
//  for(n=0;n<1000;n++){
//    union KEY key;
//    key.xy.x = 100-rand()%201;
//    key.xy.y = 100-rand()%201;
//    unsigned char slot = hash_slot_from_key(key);
//
//    if( !hash_table[slot].payload ){
//      hash_table[slot].key.u32 = key.u32;
//      hash_table[slot].payload = main;
//      continue;
//    }
//
//    if( hash_table[slot].key.u32 != key.u32 ) wasted++;
//  }
//  printf("wasted %ld\n",wasted);
//  return 0;
//}




//void *get_payload( int x , int y ){
//
//  // la vera get nell'engine 
//  // o cmq molto simile
//
//  static struct ITEM *cache = 0;  // prev call cache
//  union KEY key;
//  struct ITEM *item;
//  struct ITEM *oldest;
//
//  key.xy.x = x/128;
//  key.xy.y = y/128;
//
//  if( cache && cache->key.u32 == key.u32 ){
//    printf("cella %d,%d in cache\n",key.xy.x,key.xy.y);
//    return cache->payload;
//  }
//
//  item = hash_search( key );
//  if(item){
//    cache = item;
//    return cache->payload;
//  }
//
//  // non abbiamo la cella, va caricata.
//  // eventualmente, liberiamo spazio
//
//  while(!item){
//    item = hash_insert( key , 128*128 );
//    if(item)break;
//    // hashtable piena/out of mem payload
//    // proviamo a liberare spazio
//    oldest = hash_oldest_item();
//    hash_delete(oldest->key);
//  }
//  cache = item;  // cache
//
//  display();
//
//  // riempiamo il payload
//  memset(cache->payload,rand(),128*128);
//
//  return cache->payload;
//}



void *get_payload( int x , int y ){

  // la vera get nell'engine 
  // o cmq molto simile

  static struct ITEM *cache = 0;  // prev call cache
  union KEY key;
  struct ITEM *item;
  struct ITEM *oldest;

  key.xy.x = x/128;
  key.xy.y = y/128;

  if( cache && cache->key.u32 == key.u32 ){
    printf("cella %d,%d in cache\n",key.xy.x,key.xy.y);
    return cache->payload;
  }

  while(!item){
    item = hash_find( key , 128*128 );
    if(item)break;
    // hashtable piena/out of mem payload
    // proviamo a liberare spazio
    oldest = hash_oldest_item();
    hash_delete(oldest->key);
  }
  cache = item;  // cache

  display();

  // riempiamo il payload
  memset(cache->payload,rand(),128*128);

  return cache->payload;
}



int main(){
  int n;
  int x = 0;
  int y = 0;
  for(n=0;n<1000;n++){
    void *payload;
    printf("req pixel %d,%d\n",x,y);
    payload = get_payload(x,y);
    x += 100-rand()%201;
    y += 100-rand()%201;
  }
  return 0;
}

/*

performances per 100 turni e range chiavi xy +-200

blocco 64x64      free 44% wasted 2443
blocco 128x128    free 85% wasted 307

nel caso dello steaming voxel engine, avendo blocchi di 128x128
direi che ci son pochi tempi morti e molta flessibilità

*/

