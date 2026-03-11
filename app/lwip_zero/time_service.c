#include "time_service.h"

#include <stdio.h>

#include "main.h"
#include "lwip/apps/sntp.h"
#include "lwip/tcpip.h"
#include "uart_log.h"

static volatile uint8_t s_sntp_started = 0U;
static volatile uint8_t s_time_synced = 0U;
static volatile uint32_t s_epoch_base = 0U;
static volatile uint32_t s_tick_base_ms = 0U;

static void time_service_sntp_start_cb(void *arg)
{
  (void)arg;
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();
  log_puts("[TIME] SNTP started (pool.ntp.org)\r\n");
}

static uint8_t time_service_is_leap(uint32_t year)
{
  if ((year % 400U) == 0U)
  {
    return 1U;
  }
  if ((year % 100U) == 0U)
  {
    return 0U;
  }
  return ((year % 4U) == 0U) ? 1U : 0U;
}

static void time_service_epoch_to_utc(uint32_t epoch_sec,
                                      uint32_t *year,
                                      uint32_t *month,
                                      uint32_t *day,
                                      uint32_t *hour,
                                      uint32_t *minute,
                                      uint32_t *second)
{
  static const uint8_t month_days[12] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
  uint32_t days = epoch_sec / 86400U;
  uint32_t rem = epoch_sec % 86400U;
  uint32_t y = 1970U;
  uint32_t m = 0U;
  uint32_t mdays;

  *hour = rem / 3600U;
  rem %= 3600U;
  *minute = rem / 60U;
  *second = rem % 60U;

  while (1)
  {
    uint32_t year_days = (time_service_is_leap(y) != 0U) ? 366U : 365U;
    if (days < year_days)
    {
      break;
    }
    days -= year_days;
    y++;
  }

  while (m < 12U)
  {
    mdays = month_days[m];
    if (m == 1U && time_service_is_leap(y) != 0U)
    {
      mdays = 29U;
    }
    if (days < mdays)
    {
      break;
    }
    days -= mdays;
    m++;
  }

  *year = y;
  *month = m + 1U;
  *day = days + 1U;
}

void time_service_init(void)
{
  s_sntp_started = 0U;
  s_time_synced = 0U;
  s_epoch_base = 0U;
  s_tick_base_ms = HAL_GetTick();
}

void time_service_start(void)
{
  err_t err;

  if (s_sntp_started != 0U)
  {
    return;
  }

  s_sntp_started = 1U;
  err = tcpip_callback(time_service_sntp_start_cb, NULL);
  if (err != ERR_OK)
  {
    s_sntp_started = 0U;
    log_puts("[TIME] SNTP start callback failed\r\n");
  }
}

void time_service_set_epoch(uint32_t epoch_sec)
{
  char line[96];

  s_epoch_base = epoch_sec;
  s_tick_base_ms = HAL_GetTick();
  s_time_synced = 1U;

  (void)snprintf(line, sizeof(line), "[TIME] SNTP sync epoch=%lu\r\n", (unsigned long)epoch_sec);
  log_puts(line);
}

uint8_t time_service_is_synced(void)
{
  return s_time_synced;
}

uint32_t time_service_now_epoch(void)
{
  uint32_t elapsed_ms;
  uint32_t base_epoch;
  uint32_t base_tick;

  base_epoch = s_epoch_base;
  base_tick = s_tick_base_ms;
  elapsed_ms = HAL_GetTick() - base_tick;
  return base_epoch + (elapsed_ms / 1000U);
}

void time_service_now_string(char *out, size_t out_len)
{
  uint32_t epoch_now;
  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t hour;
  uint32_t minute;
  uint32_t second;

  if (out == NULL || out_len == 0U)
  {
    return;
  }

  if (s_time_synced == 0U)
  {
    (void)snprintf(out, out_len, "UNSYNCED");
    return;
  }

  epoch_now = time_service_now_epoch();
  time_service_epoch_to_utc(epoch_now, &year, &month, &day, &hour, &minute, &second);
  (void)snprintf(out, out_len, "%04lu-%02lu-%02lu %02lu:%02lu:%02lu UTC",
                 (unsigned long)year, (unsigned long)month, (unsigned long)day,
                 (unsigned long)hour, (unsigned long)minute, (unsigned long)second);
}
