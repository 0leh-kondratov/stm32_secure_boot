#include "netconn_page.h"

#include <stdio.h>
#include <string.h>

#include "lwip/api.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "lwip/sys.h"
#include "cmsis_os2.h"
#include "app_ethernet.h"
#include "main.h"
#include "time_service.h"
#include "uart_log.h"

static struct netif *s_netif = NULL;
static uint32_t s_page_hits = 0U;
static uint8_t s_first_http_client_seen = 0U;
static char s_last_client_ip[40] = "-";
static char s_last_client_endpoint[48] = "-";
static char s_last_client_agent[96] = "-";

#define NETCONN_PAGE_BODY_MAX 1920U
#define NETCONN_PAGE_HEAD_MAX 320U
#define NETCONN_PAGE_REQ_MAX 512U
#define NETCONN_PAGE_REQ_LINE_MAX 128U
#define NETCONN_PAGE_RECV_TIMEOUT_MS 2000U

static char s_page_body[NETCONN_PAGE_BODY_MAX];
static char s_page_head[NETCONN_PAGE_HEAD_MAX];

typedef enum
{
  NETCONN_PAGE_REQ_BAD = 0,
  NETCONN_PAGE_REQ_ROOT,
  NETCONN_PAGE_REQ_HEALTH,
  NETCONN_PAGE_REQ_NOT_FOUND,
  NETCONN_PAGE_REQ_METHOD_NOT_ALLOWED,
  NETCONN_PAGE_REQ_TOO_LARGE
} netconn_page_req_t;

static void netconn_page_log_err(const char *tag, err_t err)
{
  char line[64];
  (void)snprintf(line, sizeof(line), "[HTTP] %s err=%d\r\n", tag, (int)err);
  log_puts(line);
}

static void netconn_page_capture_client(struct netconn *conn)
{
  ip_addr_t peer_ip;
  u16_t peer_port = 0U;
  char ip_buf[40];
  err_t err;

  err = netconn_peer(conn, &peer_ip, &peer_port);
  if (err != ERR_OK)
  {
    netconn_page_log_err("peer query failed", err);
    return;
  }

  if (ipaddr_ntoa_r(&peer_ip, ip_buf, sizeof(ip_buf)) == NULL)
  {
    (void)snprintf(ip_buf, sizeof(ip_buf), "?");
  }

  (void)snprintf(s_last_client_ip, sizeof(s_last_client_ip), "%s", ip_buf);
  (void)snprintf(s_last_client_endpoint, sizeof(s_last_client_endpoint), "%s:%u", ip_buf, (unsigned int)peer_port);
}

static const char *netconn_page_detect_browser(const char *ua)
{
  if (strstr(ua, "Edg/") != NULL)
  {
    return "Edge";
  }
  if (strstr(ua, "OPR/") != NULL || strstr(ua, "Opera") != NULL)
  {
    return "Opera";
  }
  if (strstr(ua, "Chrome/") != NULL && strstr(ua, "Chromium") == NULL)
  {
    return "Chrome";
  }
  if (strstr(ua, "Firefox/") != NULL)
  {
    return "Firefox";
  }
  if (strstr(ua, "Safari/") != NULL && strstr(ua, "Chrome/") == NULL)
  {
    return "Safari";
  }
  if (strstr(ua, "curl/") != NULL)
  {
    return "curl";
  }
  if (strstr(ua, "Wget/") != NULL || strstr(ua, "wget/") != NULL)
  {
    return "wget";
  }
  return "Unknown";
}

static const char *netconn_page_detect_os(const char *ua)
{
  if (strstr(ua, "Windows") != NULL)
  {
    return "Windows";
  }
  if (strstr(ua, "Android") != NULL)
  {
    return "Android";
  }
  if (strstr(ua, "iPhone") != NULL || strstr(ua, "iPad") != NULL || strstr(ua, "iOS") != NULL)
  {
    return "iOS";
  }
  if (strstr(ua, "Mac OS X") != NULL || strstr(ua, "Macintosh") != NULL)
  {
    return "macOS";
  }
  if (strstr(ua, "Linux") != NULL)
  {
    return "Linux";
  }
  return "Unknown";
}

static const char *netconn_page_detect_device(const char *ua)
{
  if (strstr(ua, "Mobile") != NULL || strstr(ua, "Android") != NULL || strstr(ua, "iPhone") != NULL)
  {
    return "Mobile";
  }
  if (strstr(ua, "iPad") != NULL || strstr(ua, "Tablet") != NULL)
  {
    return "Tablet";
  }
  return "Desktop";
}

static void netconn_page_capture_user_agent(const void *req_buf, u16_t req_len)
{
  char req[768];
  char ua_value[256];
  char line[128];
  size_t copy_len;
  const char *ua_tag;
  const char *ua_start;
  const char *ua_end;
  size_t ua_len;
  const char *browser;
  const char *os_name;
  const char *device;

  if (req_buf == NULL || req_len == 0U)
  {
    return;
  }

  copy_len = (size_t)req_len;
  if (copy_len > (sizeof(req) - 1U))
  {
    copy_len = sizeof(req) - 1U;
  }
  memcpy(req, req_buf, copy_len);
  req[copy_len] = '\0';

  ua_tag = strstr(req, "\r\nUser-Agent:");
  if (ua_tag == NULL)
  {
    ua_tag = strstr(req, "\nUser-Agent:");
  }
  if (ua_tag == NULL)
  {
    (void)snprintf(s_last_client_agent, sizeof(s_last_client_agent), "Unknown / Unknown / Unknown");
    (void)snprintf(line, sizeof(line), "[HTTP] Client %s - Agent Unknown | Unknown | Unknown\r\n", s_last_client_endpoint);
    log_puts(line);
    return;
  }

  ua_start = strstr(ua_tag, "User-Agent:");
  if (ua_start == NULL)
  {
    return;
  }
  ua_start += strlen("User-Agent:");
  while (*ua_start == ' ' || *ua_start == '\t')
  {
    ua_start++;
  }

  ua_end = strstr(ua_start, "\r\n");
  if (ua_end == NULL)
  {
    ua_end = strstr(ua_start, "\n");
  }
  if (ua_end == NULL)
  {
    ua_end = ua_start + strlen(ua_start);
  }

  ua_len = (size_t)(ua_end - ua_start);
  if (ua_len > (sizeof(ua_value) - 1U))
  {
    ua_len = sizeof(ua_value) - 1U;
  }
  memcpy(ua_value, ua_start, ua_len);
  ua_value[ua_len] = '\0';

  browser = netconn_page_detect_browser(ua_value);
  os_name = netconn_page_detect_os(ua_value);
  device = netconn_page_detect_device(ua_value);

  (void)snprintf(s_last_client_agent, sizeof(s_last_client_agent), "%s / %s / %s", browser, os_name, device);
  (void)snprintf(line, sizeof(line), "[HTTP] Client %s - Agent %s | %s | %s\r\n",
                 s_last_client_endpoint, browser, os_name, device);
  log_puts(line);
}

static netconn_page_req_t netconn_page_parse_request(const char *buf, u16_t buflen)
{
  size_t line_len = 0U;
  size_t method_len = 0U;
  size_t path_start;
  size_t path_end;
  size_t path_len;

  if (buf == NULL || buflen == 0U)
  {
    return NETCONN_PAGE_REQ_BAD;
  }
  if (buflen >= NETCONN_PAGE_REQ_MAX)
  {
    return NETCONN_PAGE_REQ_TOO_LARGE;
  }

  while (line_len < (size_t)buflen && line_len < NETCONN_PAGE_REQ_LINE_MAX)
  {
    if (buf[line_len] == '\r' || buf[line_len] == '\n')
    {
      break;
    }
    line_len++;
  }

  if (line_len < 8U || line_len >= NETCONN_PAGE_REQ_LINE_MAX)
  {
    return NETCONN_PAGE_REQ_BAD;
  }

  while (method_len < line_len && buf[method_len] != ' ')
  {
    method_len++;
  }
  if (method_len == 0U || method_len >= line_len)
  {
    return NETCONN_PAGE_REQ_BAD;
  }

  if (method_len != 3U || strncmp(buf, "GET", 3) != 0)
  {
    return NETCONN_PAGE_REQ_METHOD_NOT_ALLOWED;
  }

  path_start = method_len + 1U;
  if (path_start >= line_len || buf[path_start] != '/')
  {
    return NETCONN_PAGE_REQ_BAD;
  }

  path_end = path_start;
  while (path_end < line_len && buf[path_end] != ' ')
  {
    path_end++;
  }
  if (path_end >= line_len)
  {
    return NETCONN_PAGE_REQ_BAD;
  }

  path_len = path_end - path_start;
  if (path_len == 1U && buf[path_start] == '/')
  {
    return NETCONN_PAGE_REQ_ROOT;
  }
  if (path_len == 7U && strncmp(&buf[path_start], "/health", 7) == 0)
  {
    return NETCONN_PAGE_REQ_HEALTH;
  }

  return NETCONN_PAGE_REQ_NOT_FOUND;
}

static void netconn_page_send_index(struct netconn *conn)
{
  char time_text[40];
  size_t body_len;
  size_t head_len;
  int n;
  err_t err;
  const ip4_addr_t *ip = netif_ip4_addr(s_netif);
  const char *ip_text = ip4addr_ntoa(ip);
  const uint8_t led1_on = led1_is_on();
  const uint8_t led2_on = ethernet_led2_is_on();
  const uint8_t led3_on = ethernet_led3_is_on();
  const char *led1_text = (led1_on != 0U) ? "ON" : "OFF";
  const char *led2_text = (led2_on != 0U) ? "ON" : "OFF";
  const char *led3_text = (led3_on != 0U) ? "ON" : "OFF";
  const char *led1_dot_color = (led1_on != 0U) ? "#00FF66" : "#000000";
  const char *led2_dot_color = (led2_on != 0U) ? "#FFD400" : "#000000";
  const char *led3_dot_color = (led3_on != 0U) ? "#FF3B30" : "#000000";
  const char *led1_text_color = (led1_on != 0U) ? "#00FF66" : "#D0D0D0";
  const char *led2_text_color = (led2_on != 0U) ? "#FFD400" : "#D0D0D0";
  const char *led3_text_color = (led3_on != 0U) ? "#FF3B30" : "#D0D0D0";

  if (ip_text == NULL)
  {
    ip_text = "0.0.0.0";
  }

  time_service_now_string(time_text, sizeof(time_text));
  s_page_hits++;

  n = snprintf(s_page_body, sizeof(s_page_body),
               "<!doctype html><html><head><meta charset=\"utf-8\">"
               "<meta http-equiv=\"refresh\" content=\"10\">"
               "<style>"
               "body{background:#000;color:#fff;font-family:Arial,sans-serif;}"
               "code{background:#222;color:#fff;padding:0 4px;}"
               "a{color:#8ab4ff;}"
               ".led-dot{display:inline-block;width:12px;height:12px;border:1px solid #666;"
               "border-radius:50%%;vertical-align:middle;margin-right:8px;}"
               "</style>"
               "<title>lwip_zero</title></head><body>"
               "<h2>STM32H743 Netconn demo</h2>"
               "<p>FreeRTOS + LwIP HTTP server is running.</p>"
               "<ul>"
               "<li>IP: %s</li>"
               "<li>MAC: %02X:%02X:%02X:%02X:%02X:%02X</li>"
               "<li>Current time: %s</li>"
               "<li>Last client IP: %s</li>"
               "<li>Last client agent: %s</li>"
               "<li><span class=\"led-dot\" style=\"background:%s;\"></span>"
               "<span style=\"color:%s;\">LED1: %s</span></li>"
               "<li><span class=\"led-dot\" style=\"background:%s;\"></span>"
               "<span style=\"color:%s;\">LED2: %s</span></li>"
               "<li><span class=\"led-dot\" style=\"background:%s;\"></span>"
               "<span style=\"color:%s;\">LED3: %s</span></li>"
               "<li>Page hits: %lu</li>"
               "</ul>"
               "<p>Route <code>/health</code> returns <code>OK</code>.</p>"
               "</body></html>",
               ip_text,
               (unsigned int)s_netif->hwaddr[0], (unsigned int)s_netif->hwaddr[1],
               (unsigned int)s_netif->hwaddr[2], (unsigned int)s_netif->hwaddr[3],
               (unsigned int)s_netif->hwaddr[4], (unsigned int)s_netif->hwaddr[5],
               time_text,
               s_last_client_ip,
               s_last_client_agent,
               led1_dot_color, led1_text_color, led1_text,
               led2_dot_color, led2_text_color, led2_text,
               led3_dot_color, led3_text_color, led3_text,
               (unsigned long)s_page_hits);

  if (n < 0)
  {
    body_len = 0U;
  }
  else if ((size_t)n >= sizeof(s_page_body))
  {
    body_len = sizeof(s_page_body) - 1U;
  }
  else
  {
    body_len = (size_t)n;
  }

  n = snprintf(s_page_head, sizeof(s_page_head),
               "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html; charset=utf-8\r\n"
               "X-Content-Type-Options: nosniff\r\n"
               "X-Frame-Options: DENY\r\n"
               "Referrer-Policy: no-referrer\r\n"
               "Cache-Control: no-store, no-cache, must-revalidate\r\n"
               "Pragma: no-cache\r\n"
               "Expires: 0\r\n"
               "Connection: close\r\n"
               "Content-Length: %lu\r\n\r\n",
               (unsigned long)body_len);
  if (n < 0)
  {
    return;
  }
  if ((size_t)n >= sizeof(s_page_head))
  {
    head_len = sizeof(s_page_head) - 1U;
  }
  else
  {
    head_len = (size_t)n;
  }

  err = netconn_write(conn, s_page_head, head_len, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write header failed", err);
    return;
  }
  err = netconn_write(conn, s_page_body, body_len, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write index failed", err);
  }
}

static void netconn_page_send_health(struct netconn *conn)
{
  static const char response[] =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "Content-Length: 2\r\n\r\n"
      "OK";
  err_t err;

  err = netconn_write(conn, response, sizeof(response) - 1U, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write health failed", err);
  }
}

static void netconn_page_send_400(struct netconn *conn)
{
  static const char response[] =
      "HTTP/1.1 400 Bad Request\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "Content-Length: 11\r\n\r\n"
      "Bad Request";
  err_t err;

  err = netconn_write(conn, response, sizeof(response) - 1U, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write 400 failed", err);
  }
}

static void netconn_page_send_405(struct netconn *conn)
{
  static const char response[] =
      "HTTP/1.1 405 Method Not Allowed\r\n"
      "Allow: GET\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "Content-Length: 18\r\n\r\n"
      "Method Not Allowed";
  err_t err;

  err = netconn_write(conn, response, sizeof(response) - 1U, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write 405 failed", err);
  }
}

static void netconn_page_send_413(struct netconn *conn)
{
  static const char response[] =
      "HTTP/1.1 413 Payload Too Large\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "Content-Length: 17\r\n\r\n"
      "Payload Too Large";
  err_t err;

  err = netconn_write(conn, response, sizeof(response) - 1U, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write 413 failed", err);
  }
}

static void netconn_page_send_404(struct netconn *conn)
{
  static const char response[] =
      "HTTP/1.1 404 Not Found\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n"
      "Content-Length: 9\r\n\r\n"
      "Not Found";
  err_t err;

  err = netconn_write(conn, response, sizeof(response) - 1U, NETCONN_COPY);
  if (err != ERR_OK)
  {
    netconn_page_log_err("write 404 failed", err);
  }
}

static void netconn_page_serve(struct netconn *conn)
{
  struct netbuf *inbuf = NULL;
  void *buf = NULL;
  u16_t buflen = 0;
  netconn_page_req_t req = NETCONN_PAGE_REQ_BAD;
  err_t err;

  err = netconn_recv(conn, &inbuf);
  if (err == ERR_OK && inbuf != NULL)
  {
    netbuf_data(inbuf, &buf, &buflen);

    if (buf == NULL || buflen == 0U)
    {
      netconn_page_send_400(conn);
    }
    else
    {
      req = netconn_page_parse_request((const char *)buf, buflen);
      if (req == NETCONN_PAGE_REQ_ROOT || req == NETCONN_PAGE_REQ_HEALTH)
      {
        netconn_page_capture_user_agent(buf, buflen);
      }

      if (req == NETCONN_PAGE_REQ_ROOT)
      {
        netconn_page_send_index(conn);
      }
      else if (req == NETCONN_PAGE_REQ_HEALTH)
      {
        netconn_page_send_health(conn);
      }
      else if (req == NETCONN_PAGE_REQ_NOT_FOUND)
      {
        netconn_page_send_404(conn);
      }
      else if (req == NETCONN_PAGE_REQ_METHOD_NOT_ALLOWED)
      {
        netconn_page_send_405(conn);
      }
      else if (req == NETCONN_PAGE_REQ_TOO_LARGE)
      {
        netconn_page_send_413(conn);
      }
      else
      {
        netconn_page_send_400(conn);
      }
    }
  }
  else if (err == ERR_TIMEOUT)
  {
    /* Normal on some browsers that pre-open/idle connections. */
  }
  else if (err != ERR_CLSD)
  {
    netconn_page_log_err("recv failed", err);
  }

  if (inbuf != NULL)
  {
    netbuf_delete(inbuf);
  }
  (void)netconn_close(conn);
}

static void netconn_page_thread(void *arg)
{
  struct netconn *listener = NULL;
  struct netconn *conn = NULL;
  err_t err;
  err_t last_accept_err = ERR_OK;

  (void)arg;

  listener = netconn_new(NETCONN_TCP);
  if (listener == NULL)
  {
    log_puts("[HTTP] netconn_new failed\r\n");
    return;
  }

  err = netconn_bind(listener, NULL, 80);
  if (err != ERR_OK)
  {
    netconn_page_log_err("bind failed", err);
    (void)netconn_delete(listener);
    return;
  }

  err = netconn_listen(listener);
  if (err != ERR_OK)
  {
    netconn_page_log_err("listen failed", err);
    (void)netconn_delete(listener);
    return;
  }
  log_puts("[HTTP] Listener ready\r\n");

  while (1)
  {
    err = netconn_accept(listener, &conn);
    if (err == ERR_OK && conn != NULL)
    {
      netconn_page_capture_client(conn);
      netconn_set_recvtimeout(conn, NETCONN_PAGE_RECV_TIMEOUT_MS);
      if (s_first_http_client_seen == 0U)
      {
        s_first_http_client_seen = 1U;
        log_puts("[HTTP] First client connected\r\n");
      }
      last_accept_err = ERR_OK;
      netconn_page_serve(conn);
      (void)netconn_delete(conn);
      conn = NULL;
    }
    else if (err != last_accept_err)
    {
      netconn_page_log_err("accept failed", err);
      last_accept_err = err;
    }
  }
}

void netconn_page_start(struct netif *netif)
{
  s_netif = netif;
  (void)sys_thread_new("HTTP", netconn_page_thread, NULL, DEFAULT_THREAD_STACKSIZE * 4, osPriorityAboveNormal);
}
