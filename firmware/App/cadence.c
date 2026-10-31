#include "cadence.h"
bool cadence_release(cadence_t *t,uint32_t now)
{
  if(!t->period || (int32_t)(now-t->due)<0) return false;
  uint32_t lag=now-t->due;
  t->released_due=t->due;
  t->skipped+=lag/t->period;
  t->due+=(lag/t->period+1)*t->period;
  if(lag>t->max_lateness)t->max_lateness=lag;
  if(lag>t->deadline)++t->late;
  if(t->count) {
    int32_t jitter=(int32_t)(now-t->previous)-(int32_t)t->period;
    if(t->count==1 || jitter<t->jitter_min)t->jitter_min=jitter;
    if(t->count==1 || jitter>t->jitter_max)t->jitter_max=jitter;
  }
  t->previous=now; ++t->count;
  return true;
}
void cadence_complete(cadence_t *t,uint32_t now)
{
  if((uint32_t)(now-t->released_due)>t->deadline)++t->completion_misses;
}
void duration_add(duration_t *s,uint32_t n)
{
  if(!s->count || n<s->min)s->min=n;
  if(n>s->max)s->max=n;
  ++s->count;s->sum+=n;
}
