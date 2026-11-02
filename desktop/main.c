#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include <glib-unix.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static GPid child;
static WebKitWebView *view;
static int port, elapsed;
static gboolean loaded;
static void stop(void) {
  if (child > 0) { kill(child, SIGTERM); waitpid(child, NULL, 0); child = 0; }
  gtk_main_quit();
}
static gboolean signal_cb(gpointer data) { (void)data; stop(); return G_SOURCE_REMOVE; }
static gboolean poll_server(gpointer data) {
  (void)data;
  if (waitpid(child, NULL, WNOHANG) == child) {
    child = 0;
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "服务已退出，请查看 ~/.dsh/desktop-app.log");
    gtk_dialog_run(GTK_DIALOG(d)); gtk_widget_destroy(d); stop(); return G_SOURCE_REMOVE;
  }
  if (loaded) return G_SOURCE_CONTINUE;
  int s = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in a = { .sin_family = AF_INET, .sin_port = htons(port), .sin_addr.s_addr = htonl(INADDR_LOOPBACK) };
  int ok = s >= 0 && connect(s, (struct sockaddr *)&a, sizeof(a)) == 0;
  if (s >= 0) close(s);
  if (ok) { char *url = g_strdup_printf("http://127.0.0.1:%d/", port); webkit_web_view_load_uri(view, url); g_free(url); loaded = TRUE; }
  else if ((elapsed += 300) >= 90000) {
    GtkWidget *d = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "服务启动超时，请查看 ~/.dsh/desktop-app.log");
    gtk_dialog_run(GTK_DIALOG(d)); gtk_widget_destroy(d); stop(); return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}
int main(int argc, char **argv) {
  gtk_init(&argc, &argv);
  gchar *exe = g_file_read_link("/proc/self/exe", NULL);
  if (!exe) return 1;
  gchar *bin = g_path_get_dirname(exe), *root = g_path_get_dirname(bin);
  gchar *node = g_build_filename(root, "node", "bin", "node", NULL);
  gchar *dsh = g_build_filename(root, "dsh", NULL);
  gchar *entry = g_build_filename(dsh, "apps", "cli", "lib", "bin.js", NULL);
  gchar *home = g_build_filename(g_get_home_dir(), ".dsh", NULL);
  g_mkdir_with_parents(home, 0700);
  gchar *log = g_build_filename(home, "desktop-app.log", NULL);
  int s = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in a = { .sin_family = AF_INET, .sin_addr.s_addr = htonl(INADDR_LOOPBACK) };
  if (s < 0 || bind(s, (struct sockaddr *)&a, sizeof(a)) < 0) return 1;
  socklen_t n = sizeof(a); if (getsockname(s, (struct sockaddr *)&a, &n) < 0) return 1;
  port = ntohs(a.sin_port); close(s);
  char p[16]; snprintf(p, sizeof(p), "%d", port);
  child = fork();
  if (child < 0) return 1;
  if (!child) { if (!freopen(log, "a", stdout) || !freopen(log, "a", stderr) || chdir(dsh)) _exit(126); execl(node, node, entry, "web", "--port", p, NULL); _exit(127); }
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "DeepSeek Harness");
  gtk_window_set_default_size(GTK_WINDOW(window), 1440, 920);
  view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));
  webkit_web_view_load_html(view, "<body style='background:#222;color:white;font:20px sans-serif'>正在启动 DeepSeek Harness…</body>", NULL);
  g_signal_connect_swapped(window, "destroy", G_CALLBACK(stop), NULL);
  g_unix_signal_add(SIGTERM, signal_cb, NULL); g_unix_signal_add(SIGINT, signal_cb, NULL);
  g_timeout_add(300, poll_server, NULL); gtk_widget_show_all(window); gtk_main();
  g_free(exe); g_free(bin); g_free(root); g_free(node); g_free(dsh); g_free(entry); g_free(home); g_free(log);
  return 0;
}
