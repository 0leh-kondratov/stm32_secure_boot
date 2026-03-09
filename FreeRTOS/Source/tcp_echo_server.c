/**
  ******************************************************************************
  * @file    tcp_echo_server.c
  * @brief   TCP echo server on port 7 using LwIP Netconn API. Echoes received
  *          data with prefix "[STM32H7]: ".
  ******************************************************************************
  */
#include "lwip/api.h"
#include "cmsis_os2.h"
#include "tcp_echo_server.h"
#include <string.h>

#define TCP_ECHO_PORT    7
#define TCP_ECHO_PREFIX  "[STM32H7]: "
#define PREFIX_LEN       (sizeof(TCP_ECHO_PREFIX) - 1)
#define RECV_BUF_SIZE    256
#define ECHO_BUF_SIZE    (RECV_BUF_SIZE + PREFIX_LEN)

static void tcp_server_thread(void *arg);
static const osThreadAttr_t tcp_server_attr = {
  .name = "tcp_echo",
  .stack_size = 512 * 4,
  .priority = osPriorityNormal,
};

void tcp_server_thread_init(void)
{
  osThreadNew(tcp_server_thread, NULL, &tcp_server_attr);
}

static void tcp_server_thread(void *arg)
{
  struct netconn *conn = NULL, *newconn = NULL;
  err_t err;
  struct netbuf *inbuf = NULL;
  void *data;
  u16_t len;
  char echo_buf[ECHO_BUF_SIZE];

  (void)arg;

  conn = netconn_new(NETCONN_TCP);
  if (!conn) return;

  netconn_bind(conn, NULL, TCP_ECHO_PORT);
  netconn_listen(conn);

  memcpy(echo_buf, TCP_ECHO_PREFIX, PREFIX_LEN);

  for (;;)
  {
    err = netconn_accept(conn, &newconn);
    if (err != ERR_OK) continue;

    do
    {
      err = netconn_recv(newconn, &inbuf);
      if (err == ERR_OK && inbuf != NULL)
      {
        netbuf_data(inbuf, &data, &len);
        if (len > (u16_t)RECV_BUF_SIZE) len = (u16_t)RECV_BUF_SIZE;
        memcpy(echo_buf + PREFIX_LEN, data, len);
        netbuf_delete(inbuf);
        inbuf = NULL;

        netconn_write(newconn, echo_buf, PREFIX_LEN + len, NETCONN_COPY);
      }
    } while (err == ERR_OK);

    if (inbuf != NULL)
      netbuf_delete(inbuf);
    netconn_close(newconn);
    netconn_delete(newconn);
    newconn = NULL;
  }
}
