#include "../desktop/startup-url.h"
#include <assert.h>
int main(void) {
  char url[512];
  assert(dsh_startup_url("dsh web: http://127.0.0.1:43872/?token=Ab_c-012\n", 43872, url, sizeof(url)));
  assert(strcmp(url, "http://127.0.0.1:43872/?token=Ab_c-012") == 0);
  assert(!dsh_startup_url("dsh web: http://127.0.0.1:43873/?token=abc\n", 43872, url, sizeof(url)));
  assert(!dsh_startup_url("dsh web: http://evil.example:43872/?token=abc\n", 43872, url, sizeof(url)));
  assert(!dsh_startup_url("dsh web: http://127.0.0.1:43872/?token=\n", 43872, url, sizeof(url)));
  assert(!dsh_startup_url("dsh web: http://127.0.0.1:43872/?token=abc&other=1\n", 43872, url, sizeof(url)));
  assert(!dsh_startup_url("dsh web: http://127.0.0.1:43872/?token=abc\n", 43872, url, 8));
  return 0;
}
