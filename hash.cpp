
// hashtab 
// versione con item aging
// pensata per immagazzinare dati che variano poco, o non variano proprio
// le collisioni sono storate in lista linkata (closed hashing? odio il termine)
// non essendo openhash (odio pure questo), l'hopping non serve
// in ogni caso, la ricerca è lineare, ma ogni lista
// è grande 1/256-esimo di quanto sarebbe senza hashing
// per fare la prova, si può forzare la fn di hash
// a generare sempre lo stesso slot

// probabilmente, il modo migliore di implementare l'ageing
// è attraverso una priority queue, penso ad uno heap. 
// alla fine, quando si butta via un item è quello col timestamp minore,
// ed un min-heap è la struttura che esegue la cosa nel minor tempo.

// una lista linkata sarebbe anche meglio, perche il pop è O(1)
// e visto che quando si inserisce un item lo si carica da disco
// l'operazione di sorted insert è ordini di grandezza più veloce
// dell'import da disco, quindi non si noterà mai



#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include "hash.h"


#define STR(S) XSTR(S)
#define XSTR(S) #S
//#define FATAL(TXT) perror(__FILE__ ":" STR(__LINE__) " " TXT ),exit(1)
//#define LOG(TXT)    puts(__FILE__ ":" STR(__LINE__) " " TXT )
#define LOG(FMT,...)   printf(__FILE__ ":" STR(__LINE__) ":" __func__ ":" FMT "\n" , ##__VA_ARGS__ )
#define FATAL(FMT,...) printf(__FILE__ ":" STR(__LINE__) ":" __func__ ":" FMT "\n" , ##__VA_ARGS__ ),exit(1)
#define COUNT(ARR) (sizeof(ARR)/sizeof(ARR[0]))



static struct ITEM *table[256]={0}; // array di ptr. 0=no chiavi
static long giri; 
static unsigned int timestamp = 0;






void timestamp_rescale(){
  int i;
  timestamp >>= 8;
  for( i=0; i<COUNT(table); i++ ){
    struct ITEM *walk = table[i];
    while(walk){
      walk->timestamp >>= 8;
      walk = walk->next;
    }
  }
}

inline unsigned short timestamp_gen(){
  if(timestamp>=50000) timestamp_rescale();
  return ++timestamp;
}




static inline int key_is_equal( union KEY a , union KEY b ){
  return a.u32 == b.u32;
}


static struct ITEM *linklist_search( struct ITEM **head , union KEY key ){
  struct ITEM *walk;
  if(!head)return 0;
  walk = *head;
  while(walk){
    giri++;
    if(key_is_equal(walk->key,key)){
      walk->timestamp = timestamp_gen();
      return walk;
    }
    walk = walk->next;
  }
  return 0;
}


static struct ITEM *linklist_insert( struct ITEM **head , union KEY key ){
  // OKKIO head insert PERMETTE CHIAVI DOPPIE
  struct ITEM *item;
  if(!head) return 0;

  LOG("key %d:%d",key.xy.x,key.xy.y);

  item = (struct ITEM *)malloc(sizeof(struct ITEM));
  if(!item){
    LOG("malloc item");
    return 0; 
  }
  item->key = key;
  item->next = 0;

  if(*head) item->next = *head;

  *head = item;
  item->timestamp = timestamp_gen();
  return item;
}


static void linklist_drop( struct ITEM **head , union KEY key ){
  // ricerca basica in lista linkata
  struct ITEM *walk;
  struct ITEM *prev=0;
  if(!head)return;
  walk = *head;
  while(walk){
    giri++;
    if(key_is_equal(walk->key,key)){
      if(!prev) *head = walk->next;
      else prev->next = walk->next;
      free(walk);
      return;
    }
    prev = walk;
    walk = walk->next;
  }
}


static struct ITEM *linklist_find_oldest( struct ITEM **head ){
  unsigned int min_timestamp;
  struct ITEM *min_item=0;
  struct ITEM *walk;
  if(!head)return 0;
  walk = *head;
  while(walk){
    giri++;
    if( !min_item || min_timestamp>walk->timestamp ){
      min_timestamp = walk->timestamp;
      min_item = walk;
    }
    walk = walk->next;
  }
  return min_item;
}


static void linklist_print( struct ITEM *head ){
  while(head){
    LOG("head %04X:%04X key %d:%d timestamp %u"
      ,FP_SEG(head),FP_OFF(head)
      ,head->key.xy.x,head->key.xy.y
      ,head->timestamp
    );
    head = head->next;
  }
}


static int linklist_len( struct ITEM *head ){
  int len = 0;
  while(head){
    giri++;
    len++;
    head = head->next;
  }
  return len;
}




//static inline unsigned char slot_from_key( union KEY key ){
//  // disattiva hashing
//  // tutte le chiavi sono perciò in una sola linklist
//  return 0;
//}

//static inline unsigned char slot_from_key( union KEY key ){
//  // bad hash
//  // la distribuzione non è nemmeno lontanamente uniforme
//  return key.u8[0]+key.u8[1]+key.u8[2]+key.u8[3];
//}


//// good hash
//unsigned char slot_from_key( union KEY key );
//#pragma aux slot_from_key = \
//"    rol al , 2     " \
//"    add al , ah    " \
//"    rol al , 2     " \
//"    add al , bl    " \
//"    rol al , 2     " \
//"    add al , bh    " \
//parm  [ bx ax ] \
//modify [ ax ] \
//value [ al ];



//// potrebbe sembrare migliore ma ... no
//unsigned char slot_from_key( union KEY key );
//#pragma aux slot_from_key = \
//"    rol al , 2     " \
//"    xor al , ah    " \
//"    rol al , 2     " \
//"    xor al , bl    " \
//"    rol al , 2     " \
//"    xor al , bh    " \
//parm  [ bx ax ] \
//modify [ ax ] \
//value [ al ];


// potrebbe sembrare migliore ma ... no
//unsigned char slot_from_key( union KEY key );
//#pragma aux slot_from_key = \
//"    rol cl , cl    " \
//"    add cl , ch    " \
//"    rol cl , cl    " \
//"    add cl , al    " \
//"    rol cl , cl    " \
//"    add cl , ah    " \
//parm  [ cx ax ] \
//modify [ cx ] \
//value [ cl ];


// good hash
unsigned char slot_from_key( union KEY key ){
  // pearson
  static unsigned char perm[256]={
    94,190,116,208,246,231,189,125,164,215,144,105,163,112,253,47,
    170,96,224,209,254,145,155,222,151,243,167,50,126,117,88,202,
    68,220,16,204,214,233,161,138,173,25,73,75,71,249,99,34,
    205,61,210,211,52,199,195,9,118,192,154,74,115,19,64,179,
    166,53,130,245,23,134,158,171,140,32,132,156,152,95,200,36,
    113,150,182,45,133,149,100,237,80,51,17,62,14,65,135,172,
    159,248,11,8,228,121,78,193,39,219,98,27,232,240,183,10,
    142,239,184,12,20,119,174,207,66,6,102,83,93,1,212,143,
    181,206,124,70,227,21,110,160,180,72,30,153,63,67,56,191,
    97,185,252,109,18,86,230,24,91,38,33,69,79,90,213,229,
    234,225,54,3,58,176,196,2,238,35,59,114,81,141,236,4,
    127,82,41,146,247,221,89,242,49,137,101,177,55,187,197,244,
    255,251,129,136,76,0,13,217,107,85,186,123,87,111,203,77,
    31,44,250,188,165,46,216,218,201,147,178,139,169,103,28,40,
    241,198,148,84,37,223,122,168,48,92,194,120,162,106,26,7,
    157,175,235,42,104,60,5,226,108,57,15,43,29,131,22,128
  };
  unsigned char h = 0;
  h = perm[h^key.u8[0]];
  h = perm[h^key.u8[1]];
  h = perm[h^key.u8[2]];
  h = perm[h^key.u8[3]];
  return h;
}


// cmq son abbastanza equivalenti ... NO!
// discorso complicato ... per un set di chiavi largo, diciamo 20k²=400mil
// sono equivalenti, ma perche siamo in regime di collisione costante
// per un set piccolo invece, diciamo 20²=400
// la rol genera una distribuzione quasi uniforme
// mentre la sum assolutamente no
// quindi rol non solo è migliore in generale
// ma nello specifico dell'engine, in cui le isole son proprio
// definite da gruppi di chiavi vicine, è addirittura ottimale





struct ITEM *hash_get( union KEY key ){
  unsigned char slot = slot_from_key( key );
  struct ITEM *item = linklist_search( &table[slot] , key );
  if(item) LOG("found key %d:%d",key.xy.x,key.xy.y);
  else     LOG("not found key %d:%d",key.xy.x,key.xy.y);
  return item;
}



struct ITEM *hash_set( union KEY key ){
  unsigned char slot = slot_from_key( key );
  struct ITEM *item = linklist_search( &table[slot] , key );
  if(!item)    item = linklist_insert( &table[slot] , key );
//  LOG("item %04X:%04X key %d:%d"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
  return item;
}



void hash_drop( union KEY key ){
  unsigned char slot = slot_from_key( key );
  linklist_drop( &table[slot] , key );
}



void hash_drop_oldest(){
  unsigned int min_timestamp;
  struct ITEM *min_item=0;
  int slot;
  for( slot=0 ; slot<COUNT(table) ; slot++ ){
    struct ITEM *item;
    giri++;
    if(!table[slot]) continue;
    item = linklist_find_oldest(&table[slot]);
    if( !min_item || min_timestamp > item->timestamp ){
      min_item = item;
      min_timestamp = item->timestamp;
    }
  }
  if(!min_item)return;
  LOG("key %d,%d timestamp %u"
    ,min_item->key.xy.x,min_item->key.xy.y
    ,min_item->timestamp
  );
  hash_drop( min_item->key );
}



void hash_print(){
  int slot=0;
  int y;
  for( y=0 ; y<4 ; y++ ){
    int x;
    for( x=0 ; x<64 ; x++ , slot++ ){
      putchar( table[slot] ? 'x' : '.' );
    }
    putchar('\n');
  }
  for( slot=0 ; slot<COUNT(table) ; slot++ ){
    int len = linklist_len(table[slot]);
    printf("slot %3d len %3d ",slot,len);
    for(;len>0;len--) putchar('.');
    putchar('\n');
//    linklist_print(table[slot]);
  }
}



//void hash_test(){
//  union KEY key={0};
//  key.xy.x = -1234;
//  key.xy.y = +4567;
//  // ricerca su hash vuota
//  hash_get( key );
//}




//void hash_test(){
//
//  struct ITEM *head = 0;
//  struct ITEM *item;
//
//  linklist_print( head );
//
//  // ricerca su lista vuota
//  { union KEY key={.xy.x=111,.xy.y=111}; item = linklist_search( &head , key ); }
//
//  LOG("item %04X:%04X\n"
//    ,FP_SEG(item),FP_OFF(item)
//  );
//
//  { union KEY key={.xy.x=111,.xy.y=111}; linklist_insert( &head , key ); }
//  { union KEY key={.xy.x=222,.xy.y=222}; linklist_insert( &head , key ); }
//  { union KEY key={.xy.x=333,.xy.y=333}; linklist_insert( &head , key ); }
//
//  linklist_print( head );
//
//  { union KEY key={.xy.x=222,.xy.y=222}; item = linklist_search( &head , key ); }
//
//  // c'è
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
//
//  // non c'è
//  { union KEY key={0}; item = linklist_search( &head , key ); }
//
//  LOG("item %04X:%04X\n"
//    ,FP_SEG(item),FP_OFF(item)
//  );
//
//  // errore
//  { union KEY key={0}; item = linklist_search( head , key ); }
//
//  LOG("item %04X:%04X\n"
//    ,FP_SEG(item),FP_OFF(item)
//  );
//}



//void hash_test(){
//
//  struct ITEM *item;
//
//  { union KEY key={.xy.x=111,.xy.y=111}; item = hash_set( key ); }
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
//  hash_print();
//
//  { union KEY key={.xy.x=222,.xy.y=222}; item = hash_set( key ); }
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
//  hash_print();
//
//  { union KEY key={.xy.x=333,.xy.y=333}; item = hash_set( key ); }
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
//  hash_print();
//}




//void hash_test(){
//  struct ITEM *head=0;
//  struct ITEM *item;
//
//  { union KEY key={.xy.x=111,.xy.y=111}; linklist_insert( &head , key ); }
//  { union KEY key={.xy.x=222,.xy.y=222}; linklist_insert( &head , key ); }
//  { union KEY key={.xy.x=333,.xy.y=333}; linklist_insert( &head , key ); }
//  linklist_print(head);
//
//  { union KEY key={.xy.x=111,.xy.y=111}; linklist_drop( &head , key ); }
////  { union KEY key={.xy.x=222,.xy.y=222}; linklist_drop( &head , key ); }
////  { union KEY key={.xy.x=333,.xy.y=333}; linklist_drop( &head , key ); }
//  { union KEY key={.xy.x=999,.xy.y=999}; linklist_drop( &head , key ); }  // non c'è
//  linklist_print(head);
//}


void hash_test(){
  int i;
  LOG("insert");
  giri=0;
  for(i=0;i<1000;i++){
    union KEY key;
    key.xy.x=10-rand()%21;
    key.xy.y=10-rand()%21;
    hash_set( key );
  }
  LOG("giri %ld",giri);
//  hash_print();
  LOG("search");
  giri=0;
  for(i=0;i<3000;i++){
    union KEY key;
    key.xy.x=10-rand()%21;
    key.xy.y=10-rand()%21;
    hash_get( key );
  }
  LOG("giri %ld",giri);
  hash_print();
//  for(i=0;i<2000;i++){
//    union KEY key;
//    key.xy.x=10-rand()%21;
//    key.xy.y=10-rand()%21;
//    hash_drop( key );
//  }
//  hash_print();
}


//void hash_test(){
//  { union KEY key={.xy.x=111,.xy.y=111}; hash_set( key ); }
//  { union KEY key={.xy.x=222,.xy.y=222}; hash_set( key ); }
//  { union KEY key={.xy.x=333,.xy.y=333}; hash_set( key ); }
//  { union KEY key={.xy.x=111,.xy.y=111}; hash_set( key ); }
//  hash_print();
//}


//void hash_test(){
//  int i;
//  for(i=0;i<100;i++){
//    union KEY key;
//    key.xy.x=100-rand()%201;
//    key.xy.y=100-rand()%201;
//    giri=0;
//    while(1){
//      struct ITEM *item = hash_set( key );
//      if(item)break;
//      hash_drop_oldest();
//    }
//    LOG("giri %ld\n",giri);
//  }
//  hash_print();
//}
