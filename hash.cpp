
// versione senza item aging
// pensata per immagazzinare dati che variano poco
// le collisioni sono storate in lista linkata (closed hashing? odio il termine)
// non essendo openhash (odio pure questo), l'hopping non serve
// quindi essenzialmente la ricerca è lineare, ma ogni lista
// è grande 1/256-esimo di quanto sarebbe senza hashing
// per fare la prova, si può forzare la fn di hash
// a generare sempre lo stesso slot

// aggiungiamo uno speedup MRU 
// in pratica, una ricerca (get) andata a buon fine
// sposta l'elemento in testa alla lista
// rendendo cosi le ricerche successive della stessa chiave più veloci
// questo meccanismo fa si che elementi richiesti più spesso migrino in testa
// ed elementi richiesti meno spesso in coda
// accelerando quindi i tempi di ricerca

// ora, nell'ottica di usare questa hash come backbone per una
// struttura di accelerazione a griglia fissa per il raytracing
// aggiungo anche MRU su chiavi non presenti. funza cosi:
// la ricerca che non va a buon fine crea una "chiave negativa"
// ovvero una chiave che nella hash fisicamente esiste
// ma che ha un valore particolare per indicare "questa chiave non esiste"
// questa chiave viene poi trattata come le altre chiavi 
// (che a questo punto chiameremo positive) implementando cosi l'MRU

// ora che anche la ricerca potenzialmente modifica la hashtable
// bisogna considerare la cosa in un futuro scenario multiprocesso
// ed assicurare coerenza e performance


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hash.h"


#define LOG_NOBR(FMT,...)   fprintf(stdout,"%s:%d(%s) " FMT , __FILE__ , __LINE__ , __func__ , ##__VA_ARGS__ )
#define LOG(FMT,...)        LOG_NOBR(FMT "\n", ##__VA_ARGS__ )
#define FATAL(FMT,...) LOG(FMT,##__VA_ARGS__),exit(1)
#define COUNT(ARR) (sizeof(ARR)/sizeof(ARR[0]))




struct SLOT {
  struct ITEM *head;  // 0=nessuna chiave in lista
//  // per future versioni multiproc
//  int mutex;  
//  int users_count;
  // statistiche di questo bin
  int neg_count;
  int key_count;
};


static struct SLOT slots[256]={0}; // array di ptr. 0=no chiavi





static inline int key_is_equal( union KEY a , union KEY b ){
  return ( a.xyz.x == b.xyz.x && a.xyz.y == b.xyz.y && a.xyz.z == b.xyz.z );
}


static struct ITEM *linklist_head_insert( struct SLOT *slot , union KEY key ){
  // linklist_head_insert PERMETTE CHIAVI DOPPIE
  // la univocità delle chiavi è implementata a monte
  // di conseguenza, linklist_head_insert produce sempre un item valido

  struct ITEM *item;
  assert(slot);   // metti che qualcuno si dimentichi di indicare la testa della lista ...

  item = (struct ITEM *)malloc(sizeof(struct ITEM));  // la malloc è il prossimo passo da ottimizzare
  if(!item) FATAL("out of mem");

  item->key = key;
  item->next = 0;
  item->payload = 0;  // chiave negativa di default

  if(slot->head) item->next = slot->head;

  slot->head = item;
  slot->key_count++;
  return item;
}






static struct ITEM *linklist_search( struct SLOT *slot , union KEY key ){
  // cerca la chiave, applicando una logica MRU
  // se la trova, sposta l'item in testa alla lista e lo ritorna
  // se non la trova, inserisce una chiave vuota e la ritorna
  struct ITEM *prev;
  struct ITEM *walk;
  assert(slot);   // metti che qualcuno si dimentichi di indicare la testa della lista ...
  if(!slot->head) return linklist_head_insert(slot,key);
  walk = slot->head;
  prev = slot->head;
  while(walk){
    if(key_is_equal(walk->key,key)){
      if(prev==walk) return walk;
      // la logica MRU è tutta qui: sposta in testa l'item
      prev->next = walk->next;
      walk->next = slot->head;
      slot->head = walk;
      //////////////////////////////////////////
      return walk;
    }
    prev = walk;
    walk = walk->next;
  }
  // chiave non presente, creiamo in testa una chiave negativa
  return linklist_head_insert(slot,key);
}






//static void linklist_print( struct ITEM *head ){
//  while(head){
//    LOG("head %04X:%04X key %d:%d"
//      ,FP_SEG(head),FP_OFF(head)
//      ,head->key.xyz.x,head->key.xyz.y
//    );
//    head = head->next;
//  }
//}


//static int linklist_len( struct ITEM *head ){
//  int len = 0;
//  while(head){
//    len++;
//    head = head->next;
//  }
//  return len;
//}




//static inline uint8_t slot_from_key( union KEY key ){
//  // disattiva hashing
//  // tutte le chiavi sono perciò in una sola linklist
//  return 0;
//}


//static inline uint8_t slot_from_key( union KEY key ){
//  // pearson's hash
//  // https://en.wikipedia.org/wiki/Pearson_hashing
//  const uint8_t perm[256]={
//    // seq 0 255 | shuf | sed 's/$/,/' > perm.h
//    #include "perm.h"
//  };
//  register uint8_t hash = 0;
//  hash = perm[ hash ^ key.u8[0]];
//  hash = perm[ hash ^ key.u8[1]];
//  hash = perm[ hash ^ key.u8[2]];
//  hash = perm[ hash ^ key.u8[3]];
//  return hash;
//}


// pearson's hash - macro style
// https://en.wikipedia.org/wiki/Pearson_hashing
const uint8_t perm[256]={
  // seq 0 255 | shuf | sed 's/$/,/' > perm.h
  #include "perm.h"
};
#define slot_from_key( key ) perm[perm[perm[perm[perm[perm[key.u8[0]]^key.u8[1]]^key.u8[2]]^key.u8[3]]^key.u8[4]]^key.u8[5]];
// ha praticamente la stessa velocità di good hash sotto
// probabilmente perche non c'è call e il compilatore ottimizzata via registri




struct ITEM *hash_search( union KEY key ){
  uint8_t slot = slot_from_key( key );
  struct ITEM *item = linklist_search( &slots[slot] , key );
  // linklist_search ritorna sempre un item
  // che potrebbe esser una chiave positiva o negativa
  // dipende da item->payload:
  // !=0 chiave positiva
  // ==0 nuova chiave o chiave negativa
  // la differenza la fa il contesto: qui stiamo cercando una chiave
  // ci interessa sapere se c'è ed ha payload valido, e la ritorniamo
  // se c'è ma ha payload nullo, diciamo che la chiave richiesta non c'è
  if(item->payload) LOG("valued key %d:%d",key.xyz.x,key.xyz.y);
  else              LOG("null key %d:%d",key.xyz.x,key.xyz.y);
  // se poi fuori da questa fn settiamo il payload a zero
  // otteniamo una chiave negativa
  return item;
}



struct ITEM *hash_insert( union KEY key ){
  uint8_t slot = slot_from_key( key );
  // linklist_search ritorna sempre una chiave positiva o negativa
  // quindi alloca e tutto. sta poi al chiamante riempire il payload in modo sensato
  return linklist_search( &slots[slot] , key );
}

// alla fine insert e search sono IDENTICHE
// tutto dipende sal payload associato



static void hash_print(){
  int slot=0;
  int y;
  for( y=0 ; y<4 ; y++ ){
    int x;
    for( x=0 ; x<64 ; x++ , slot++ ){
      putchar( slots[slot].head ? 'x' : '.' );
    }
    putchar('\n');
  }
  for( slot=0 ; slot<COUNT(slots) ; slot++ ){
    int len = slots[slot].key_count;
    LOG_NOBR("slot %3d keys %3d neg %3d ",slot,len,slots[slot].neg_count);
    for(;len>0;len--) putchar('.');
    putchar('\n');
  }
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
    struct ITEM *item;
    union KEY key;
    key.xyz.x=0;
    key.xyz.y=0;
    key.xyz.z=0;
    item = hash_insert( key );
    item->payload = (void*)main;   // ora la chiave è positiva
  }
  for(i=0;i<10000;i++){
    struct ITEM *item;
    union KEY key;
    key.xyz.x=rand();
    key.xyz.y=1;
    key.xyz.z=1;
    item = hash_insert( key );
    item->payload = (void*)main;   // ora la chiave è positiva
  }
  rdtsc_stop=rdtsc();
//  hash_print();
  LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);

  LOG("search");
  for(i=0;i<3;i++){
    union KEY key;
    key.xyz.x=0; 
    key.xyz.y=0;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  LOG("search");
  for(i=0;i<3;i++){
    union KEY key;
    key.xyz.x=-1;  // ho solo messo chiavi positive
    key.xyz.y=-1;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  LOG("search");
  for(i=0;i<3;i++){
    union KEY key;
    key.xyz.x=0; 
    key.xyz.y=0;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  LOG("search");
  for(i=0;i<3;i++){
    union KEY key;
    key.xyz.x=-1;  // ho solo messo chiavi positive
    key.xyz.y=-1;
    rdtsc_start=rdtsc();
    hash_search( key );
    rdtsc_stop=rdtsc();
    LOG("lap %" PRIu64 "" ,rdtsc_stop-rdtsc_start);
  }

  hash_print();
  return 0;
}

