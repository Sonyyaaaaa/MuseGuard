#include "exhibit.h"
uint32_t exhibit_command(exhibit_t *state, museguard_t *detector, char command, uint32_t now)
{
  if (command == 'a' && !state->access) {
    state->access=true; state->opened_at=now; return MG_ACCESS_BEGIN;
  }
  if (command == 'g' && state->access) { state->access=false; return MG_ACCESS_END; }
  if (command == 'k') { detector->alarm=false; return MG_ACK; }
  return 0;
}
uint32_t exhibit_tick(exhibit_t *state, uint32_t now)
{
  if (state->access && (uint32_t)(now-state->opened_at)>=60000U) {
    state->access=false; return MG_ACCESS_END;
  }
  return 0;
}
void exhibit_apply(const exhibit_t *state, museguard_t *detector, bool previous_alarm, uint32_t events)
{
  /* Opening access never clears a previous alarm; shocks remain alarms. */
  if (state->access && !(events & MG_SHOCK)) { detector->alarm=previous_alarm; }
}
