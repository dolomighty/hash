

#include <stdio.h>
#include <dos.h>
#include <malloc.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include "hash.h"


#define STR(S) XSTR(S)
#define XSTR(S) #S
//#define FATAL(TXT) perror(__FILE__ ":" STR(__LINE__) " " TXT ),exit(1)
//#define LOG(TXT)    puts(__FILE__ ":" STR(__LINE__) " " TXT )
#define LOG(FMT,...)   printf(__FILE__ ":" STR(__LINE__) " " FMT , ##__VA_ARGS__ )
#define FATAL(FMT,...) LOG(FMT,##__VA_ARGS__),exit(1)
#define COUNT(ARR) (sizeof(ARR)/sizeof(ARR[0]))


// https://www.tutorialspoint.com/data_structures_algorithms/table_program_in_c.htm
// la insert permetteva di inserir chiavi duplicate, fixata
// cablata per una hashtable di 256 celle (ma non credo ci sian benefici)
// hashing fn abbastanza isotropa
// ricerca slot per collisioni via hop variabile


struct ITEM table[256]={0}; 
int available = 256;
unsigned short timestamp = 0;



static void timestamp_rescale(){
  int i;
  timestamp >>= 8;
  for( i=0; i<COUNT(table); i++ ){
    if(!table[i].payload)continue;
    table[i].timestamp >>= 8;
  }
}

static inline unsigned short timestamp_gen(){
  if(timestamp>=50000) timestamp_rescale();
  return ++timestamp;
}





//// no hash
//static inline unsigned char slot_from_key( union KEY key ){
//  return 0;
//}

//// bad hash
//static inline unsigned char slot_from_key( union KEY key ){
//  return key.u8[0]+key.u8[1]+key.u8[2]+key.u8[3];
//}



// good hash
static inline unsigned char slot_from_key( union KEY key );
#pragma aux slot_from_key = \
"   rol  al , 2   " \
"   add  al , ah  " \
"   rol  al , 2   " \
"   add  al , bl  " \
"   rol  al , 2   " \
"   add  al , bh  " \
parm [ bx ax ] \
modify [ ax ] \
value [ al ];



static inline unsigned char hop_from_slot( unsigned char slot ){
  return slot*2|1;
}

static inline int equal_keys( union KEY a , union KEY b ){
  return a.u32 == b.u32;
}



struct ITEM *hash_search( union KEY key ){

  unsigned char slot = slot_from_key(key);
  unsigned char hop = hop_from_slot(slot);

  // cerca partendo da slot
  while( table[slot].payload ){
    if(equal_keys(table[slot].key,key)){
      // trovato
      table[slot].timestamp = timestamp_gen();
      return &table[slot]; 
    }
    // ritentiamo
    LOG("hash_search: key %d,%d collision with slot %d\n"
      ,key.xy.x,key.xy.y
      ,slot
    );
    slot+=hop;
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
    LOG("hash_insert: hash table full\n");
    return 0;
  }

  slot = slot_from_key(key);
  hop = hop_from_slot(slot);   

  // cicliamo sulle celle usate partendo da slot
  while( table[slot].payload ){
    if(equal_keys(table[slot].key,key)){
      // la key c'è già, aggiorniamo
      LOG("hash_insert: update key %d,%d\n",key.xy.x,key.xy.y);
      free(table[slot].payload);
      table[slot].payload=0;
      break;
    } 
    LOG("hash_insert: key %d,%d collision with slot %d (%d,%d)\n"
      ,key.xy.x,key.xy.y
      ,slot
      ,table[slot].key.xy.x,table[slot].key.xy.y
    );
    // go to next cell
    slot+=hop;
  }

  payload = malloc(payload_size);
  if(!payload){
    LOG("!payload\n");
    return 0;
  }

  LOG("payload @ %04X:%04X\n",FP_SEG(payload),FP_OFF(payload));

  available --;
  table[slot].key = key;
  table[slot].payload = payload;
  table[slot].timestamp = timestamp_gen();
  return &table[slot];
}



void hash_drop( union KEY key ){

  unsigned char slot = slot_from_key(key);
  unsigned char hop = hop_from_slot(slot);

  while(1){
    if( table[slot].payload && equal_keys(table[slot].key,key)){
      LOG("hash_drop: freeing slot %d\n",slot);
      free( table[slot].payload );
      table[slot].payload = 0;
      available ++;
      return;
    }
    slot+=hop;
  }      
  LOG("hash_drop: key %d,%d non trovata\n",key.xy.x,key.xy.y);
}




static struct ITEM *hash_find( union KEY key , unsigned int payload_size 
  , int (*init_payload_callback)( struct ITEM *item ) ){
  
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
    LOG("hash_find: hash table full\n");
    return 0;
  }

  slot = slot_from_key(key);
  hop = hop_from_slot(slot);   

  // cicliamo sulle celle usate partendo da slot
  while( table[slot].payload ){
    if(equal_keys(table[slot].key,key)){
      // la key c'è già, ritorniamola
      // perche nel nostro specifico caso
      // se la cella è nella hash, allora è valida
      // dobbiamo solo aggiornare l'età
      table[slot].timestamp = timestamp_gen();
      return &table[slot];
    } 
    LOG("hash_find: key %d,%d collision with slot %d (%d,%d)\n"
      ,key.xy.x,key.xy.y
      ,slot
      ,table[slot].key.xy.x,table[slot].key.xy.y
    );
    // go to next cell
    slot+=hop;
  }

  // key non trovata, creiamola

  payload = malloc(payload_size);
  if(!payload){
    LOG("!malloc payload\n");
    return 0;
  }

  table[slot].key = key;
  table[slot].timestamp = timestamp_gen();
  table[slot].payload = payload;
  if(init_payload_callback && !init_payload_callback(&table[slot])){
    LOG("!init_payload_callback\n");
    free(payload);
    return 0;
  }

  LOG("new payload @ %04X:%04X\n",FP_SEG(payload),FP_OFF(payload));

  available --; // ok, chiave e payload accettati
  return &table[slot];
}







static void display(){
  int i;
  for( i=0 ; i<256 ; i++ ){
    putchar( table[i].payload ? 'x' : '.' );
    if(i%64==63) putchar('\n');
  }
  LOG("free %d%%\n",available*100/256);
}



static struct ITEM *hash_oldest_item(){
  int i;
  int slot = -1;
  int min  = -1;
  for( i=0; i<COUNT(table); i++ ){
    if(!table[i].payload)continue;
//    LOG("%d ",table[i].timestamp);
    if( min>=0 && min<table[i].timestamp )continue;
    min = table[i].timestamp;
    slot = i;
  }
//  LOG("oldest %d\n",min);
  return &table[slot];
}







int init_payload( struct ITEM *item ){
  // riempiamo il payload
  // qui caricherei da disco
  memset(item->payload,rand(),128*128);
  return 1;
}


void *get_payload( int x , int y ){

  // la vera get nell'engine 
  // o cmq molto simile

  union KEY key;
  static struct ITEM *item=0;

  key.xy.x = x&~127;
  key.xy.y = y&~127;

  if( item && equal_keys(item->key,key)){
    LOG("key %d,%d in local cache\n",key.xy.x,key.xy.y);
    return item->payload;
  }

  // la cache è invalida, troviamo o creaiamo il nuovo item
  item=0;
  while(!item){
    item = hash_find( key , 128*128 , init_payload );
    if(item)break;
    // hashtable piena/out of mem payload
    // proviamo a liberare spazio
    hash_drop(hash_oldest_item()->key);
  }

  LOG("key %d,%d locally cached\n",key.xy.x,key.xy.y);
  display();

  return item->payload;
}




//void hash_test(){
//  // valutiamo la bonta di slot_from_key()
//  // in pratica inseriamo in tabella
//  // e contiamo le collisioni
//  // meno sono, meglio è
//  int n;
//  long wasted=0;
//  for(n=0;n<1000;n++){
//    unsigned char slot;
//    union KEY key;
//    key.xy.x = 10-rand()%21;
//    key.xy.y = 10-rand()%21;
//    slot = slot_from_key(key);
//
//    if( !table[slot].payload ){
//      table[slot].key = key;
//      table[slot].payload = hash_test;
//      available--;
//      continue;
//    }
//
//    wasted++;
////    if(!equal_keys(table[slot].key,key)) wasted++;
//  }
//  LOG("wasted %ld\n",wasted);
//  display();
//}


void hash_test(){
  int n;
  int x = 0;
  int y = 0;
  for(n=0;n<1000;n++){
    x += 100-rand()%201;
    y += 100-rand()%201;
    LOG("req pixel %d,%d\n",x,y);
    get_payload(x,y);
  }
}


