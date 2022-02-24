
// versione senza item aging
// pensata per immagazzinare dati che variano poco
// le collisioni sono storate in lista linkata (closed hashing? odio il termine)
// non essendo openhash (odio pure questo), l'hopping non serve
// quindi essenzialmente la ricerca è lineare, ma ogni lista
// è grande 1/256-esimo di quanto sarebbe senza hashing
// per fare la prova, si può forzare la fn di hash
// a generare sempre lo stesso slot


#include <stdio.h>
#include <stdlib.h>
#include <dos.h>


#define STR(S) XSTR(S)
#define XSTR(S) #S
//#define FATAL(TXT) perror(__FILE__ ":" STR(__LINE__) " " TXT ),exit(1)
//#define LOG(TXT)    puts(__FILE__ ":" STR(__LINE__) " " TXT )
#define LOG(FMT,...)   fprintf(stdout,__FILE__ ":" STR(__LINE__) " " FMT , ##__VA_ARGS__ )
#define FATAL(FMT,...) LOG(FMT,##__VA_ARGS__),exit(1)
#define COUNT(ARR) (sizeof(ARR)/sizeof(ARR[0]))



union KEY { 
  struct { short x,y; } xy;
  unsigned long u32;
  unsigned char u8[4];
};


struct ITEM {
  union KEY key;
  struct ITEM *next;  // 0=fine catena
  void *payload;  // libero di farci qualunque cosa
};


static struct ITEM *table[256]={0}; // array di ptr. 0=no chiavi
static long giri;


static inline int key_is_equal( union KEY a , union KEY b ){
  return a.u32 == b.u32;
}


static struct ITEM *linklist_search( struct ITEM **head , union KEY key ){
  struct ITEM *walk;
  if(!head)return 0;
  walk = *head;
  while(walk){
    giri++;
    if(key_is_equal(walk->key,key)) return walk;
    walk = walk->next;
  }
  return 0;
}


static struct ITEM *linklist_insert( struct ITEM **head , union KEY key ){
  // head insert PERMETTE CHIAVI DOPPIE
  struct ITEM *item;
  if(!head) return 0;

  item = (struct ITEM *)malloc(sizeof(struct ITEM));
  if(!item) return 0; // out of mem
  item->key = key;
  item->next = 0;
  item->payload = 0;

  if(*head) item->next = *head;

  *head = item;
  return item;
}


static void linklist_drop( struct ITEM **head , union KEY key ){
  // ricerca basica in lista linkata
  struct ITEM *walk;
  struct ITEM *prev=0;
  if(!head)return;
  walk = *head; // per comodità 
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



static void linklist_print( struct ITEM *head ){
  while(head){
    LOG("head %04X:%04X key %d:%d\n"
      ,FP_SEG(head),FP_OFF(head)
      ,head->key.xy.x,head->key.xy.y
    );
    head = head->next;
  }
}

static int linklist_len( struct ITEM *head ){
  int len = 0;
  while(head){
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


// good hash
unsigned char slot_from_key( union KEY key );
#pragma aux slot_from_key = \
"    rol al , 2     " \
"    add al , ah    " \
"    rol al , 2     " \
"    add al , bl    " \
"    rol al , 2     " \
"    add al , bh    " \
parm  [ bx ax ] \
modify [ ax ] \
value [ al ];

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


// cmq son abbastanza equivalenti ... NO!
// discorso complicato ... per un set di chiavi largo, diciamo 20k²=400mil
// sono equivalenti, ma perche siamo in regime di collisione costante
// per un set piccolo invece, diciamo 20²=400
// la rol genera una distribuzione quasi uniforme
// mentre la sum assolutamente no
// quindi rol non solo è migliore in generale
// ma nello specifico dell'engine, in cui le isole son proprio
// definite da gruppi di chiavi vicine, è addirittura ottimale





struct ITEM *hash_search( union KEY key ){
  unsigned char slot = slot_from_key( key );
  struct ITEM *item = linklist_search( &table[slot] , key );
  if(item) LOG("found     key %d:%d\n",key.xy.x,key.xy.y);
  else     LOG("not found key %d:%d\n",key.xy.x,key.xy.y);
  return item;
}



struct ITEM *hash_insert( union KEY key ){
  unsigned char slot = slot_from_key( key );
  struct ITEM *item = linklist_search( &table[slot] , key );
  if(!item)    item = linklist_insert( &table[slot] , key );
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
  return item;
}



void hash_drop( union KEY key ){
  unsigned char slot = slot_from_key( key );
  linklist_drop( &table[slot] , key );
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
    LOG("slot %3d len %3d ",slot,len);
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
//  hash_search( key );
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
//  { union KEY key={.xy.x=111,.xy.y=111}; item = hash_insert( key ); }
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
//  hash_print();
//
//  { union KEY key={.xy.x=222,.xy.y=222}; item = hash_insert( key ); }
//  LOG("item %04X:%04X key %d:%d\n"
//    ,FP_SEG(item),FP_OFF(item)
//    ,item->key.xy.x,item->key.xy.y
//  );
//  hash_print();
//
//  { union KEY key={.xy.x=333,.xy.y=333}; item = hash_insert( key ); }
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
  LOG("insert\n");
  giri=0;
  for(i=0;i<1000;i++){
    union KEY key;
    key.xy.x=10-rand()%21;
    key.xy.y=10-rand()%21;
    hash_insert( key );
  }
  LOG("giri %ld\n",giri);
  hash_print();
  LOG("search\n");
  giri=0;
  for(i=0;i<2000;i++){
    union KEY key;
    key.xy.x=10-rand()%21;
    key.xy.y=10-rand()%21;
    hash_search( key );
  }
  LOG("giri %ld\n",giri);
//  for(i=0;i<2000;i++){
//    union KEY key;
//    key.xy.x=10-rand()%21;
//    key.xy.y=10-rand()%21;
//    hash_drop( key );
//  }
//  hash_print();
}


//void hash_test(){
//  { union KEY key={.xy.x=111,.xy.y=111}; hash_insert( key ); }
//  { union KEY key={.xy.x=222,.xy.y=222}; hash_insert( key ); }
//  { union KEY key={.xy.x=333,.xy.y=333}; hash_insert( key ); }
//  { union KEY key={.xy.x=111,.xy.y=111}; hash_insert( key ); }
//  hash_print();
//}

