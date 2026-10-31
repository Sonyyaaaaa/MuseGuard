#include "event_store.h"
#include <string.h>
#define RECORD_MAGIC 0x4D470301U
#define RECORD_COMMIT 0xC017CA5EU
_Static_assert(sizeof(mg_record_t)==MG_STORE_RECORD,"record layout");
uint32_t mg_crc(const void *bytes, uint32_t length)
{
  const uint8_t *p=bytes; uint32_t crc=~0U;
  while (length--) {
    crc^=*p++;
    for (unsigned bit=0;bit<8;++bit) { crc=(crc>>1)^((0U-(crc&1U))&0xEDB88320U); }
  }
  return ~crc;
}
bool mg_record_valid(const mg_record_t *r)
{
  return r->magic==RECORD_MAGIC && r->commit==RECORD_COMMIT && r->crc==mg_crc(r,24);
}
static bool blank(const mg_record_t *r)
{
  const uint8_t *p=(const uint8_t *)r;
  for (unsigned i=0;i<sizeof(*r);++i) { if(p[i]!=255) return false; }
  return true;
}
bool mg_store_read(const mg_store_t *s,uint32_t slot,mg_record_t *r)
{
  return slot<MG_STORE_BYTES/MG_STORE_RECORD &&
         s->io.read(s->io.context,slot*MG_STORE_RECORD,r,sizeof(*r));
}
bool mg_store_open(mg_store_t *s,mg_storage_t io)
{
  if(!s || !io.read || !io.program8 || !io.erase) return false;
  *s=(mg_store_t){.io=io}; uint32_t newest=0; bool found=false;
  for(uint32_t slot=0;slot<MG_STORE_BYTES/MG_STORE_RECORD;++slot) {
    mg_record_t r;
    if(!mg_store_read(s,slot,&r)) return false;
    if(mg_record_valid(&r)) {
      ++s->valid;
      if(!found || (int32_t)(r.sequence-s->sequence)>0) {
        found=true; s->sequence=r.sequence; newest=slot*MG_STORE_RECORD;
      }
    } else if(!blank(&r)) ++s->corrupt;
  }
  /* Never reuse a torn slot. Search only after the newest record in its page. */
  uint32_t page=found ? newest/MG_STORE_PAGE : 0;
  s->next=found ? newest+MG_STORE_RECORD : page*MG_STORE_PAGE;
  while(s->next<(page+1)*MG_STORE_PAGE) {
    mg_record_t r;
    if(!io.read(io.context,s->next,&r,sizeof(r))) return false;
    if(blank(&r)) return true;
    s->next+=MG_STORE_RECORD;
  }
  return true;
}
bool mg_store_append(mg_store_t *s,uint32_t time,uint32_t events,uint32_t observation)
{
  if(s->next==MG_STORE_PAGE || s->next==MG_STORE_BYTES) {
    uint32_t destination=s->next==MG_STORE_PAGE ? MG_STORE_PAGE : 0;
    if(!s->io.erase(s->io.context,destination)) return false;
    s->next=destination;
  }
  mg_record_t r={RECORD_MAGIC,s->sequence+1,time,events,observation,0,0,RECORD_COMMIT};
  r.crc=mg_crc(&r,24);
  uint32_t address=s->next;
  s->next+=MG_STORE_RECORD; /* even a failed programming attempt consumes the slot */
  for(uint32_t i=0;i<sizeof(r);i+=8) {
    if(!s->io.program8(s->io.context,address+i,(const uint8_t *)&r+i)) return false;
  }
  mg_record_t check;
  if(!s->io.read(s->io.context,address,&check,sizeof(check)) ||
     memcmp(&r,&check,sizeof(r)) || !mg_record_valid(&check)) return false;
  s->sequence=r.sequence;
  return true;
}
