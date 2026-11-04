#ifndef DSH_STARTUP_URL_H
#define DSH_STARTUP_URL_H

#include <stdio.h>
#include <string.h>

/* Accept only the authenticated loopback URL emitted by our child process. */
static int dsh_startup_url(const char *line, int port, char *url, size_t size) {
  char prefix[96];
  int n = snprintf(prefix, sizeof(prefix), "dsh web: http://127.0.0.1:%d/?token=", port);
  if (n < 0 || (size_t)n >= sizeof(prefix) || strncmp(line, prefix, (size_t)n) != 0) return 0;
  const char *token = line + n;
  size_t length = strspn(token, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-");
  if (length == 0 || (token[length] != '\n' && token[length] != '\r' && token[length] != '\0')) return 0;
  size_t url_len = (size_t)(token - line) + length - strlen("dsh web: ");
  if (url_len + 1 > size) return 0;
  memcpy(url, line + strlen("dsh web: "), url_len);
  url[url_len] = '\0';
  return 1;
}
#endif
