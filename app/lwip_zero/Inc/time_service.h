#ifndef LWIP_ZERO_TIME_SERVICE_H
#define LWIP_ZERO_TIME_SERVICE_H

#include <stddef.h>
#include <stdint.h>

void time_service_init(void);
void time_service_start(void);
void time_service_set_epoch(uint32_t epoch_sec);
uint8_t time_service_is_synced(void);
uint32_t time_service_now_epoch(void);
void time_service_now_string(char *out, size_t out_len);

#endif /* LWIP_ZERO_TIME_SERVICE_H */
