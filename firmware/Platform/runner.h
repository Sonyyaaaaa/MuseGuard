#ifndef MUSEGUARD_RUNNER_H
#define MUSEGUARD_RUNNER_H
void runner_start(void);
void runner_poll(void);
#ifdef MG_FREERTOS
void runner_sensor_step(void);
void runner_service_step(void);
void mg_rtos_start(void);
void mg_rtos_report(void);
void mg_state_unlock(void);
void mg_state_lock(void);
int mg_rtos_printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
void mg_rtos_i2c_timeout(unsigned long flag, unsigned long isr, unsigned long cr2);
#endif
#endif
