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
#include <fcntl.h>
#include "startup-url.h"

static GPid child;
static WebKitWebView *view;
static int port, elapsed;
static gboolean loaded;
static FILE *log_file;

static gboolean read_server_output(GIOChannel *channel, GIOCondition condition, gpointer data) {
  (void)data;
  if (condition & G_IO_IN) {
    gchar *line = NULL;
    GIOStatus status;
    while ((status = g_io_channel_read_line(channel, &line, NULL, NULL, NULL)) == G_IO_STATUS_NORMAL) {
      char url[512];
      if (!loaded && dsh_startup_url(line, port, url, sizeof(url))) {
        loaded = TRUE;
        webkit_web_view_load_uri(view, url);
        if (log_file) { fputs("dsh web: authenticated local service ready\n", log_file); fflush(log_file); }
      } else if (log_file && !g_str_has_prefix(line, "dsh web: http")) {
        fputs(line, log_file); fflush(log_file);
      }
      g_free(line);
      line = NULL;
    }
    g_free(line);
  }
  return (condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}
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
  if (!loaded && (elapsed += 300) >= 90000) {
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
  int output_pipe[2];
  if (pipe(output_pipe) < 0) return 1;
  child = fork();
  if (child < 0) return 1;
  if (!child) {
    close(output_pipe[0]);
    int logfd = open(log, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (logfd < 0 || dup2(output_pipe[1], STDOUT_FILENO) < 0 || dup2(logfd, STDERR_FILENO) < 0 || chdir(dsh)) _exit(126);
    close(output_pipe[1]); close(logfd);
    execl(node, node, entry, "web", "--port", p, "--no-open", NULL);
    _exit(127);
  }
  close(output_pipe[1]);
  log_file = fopen(log, "a");
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "DeepSeek Harness");
  gtk_window_set_default_size(GTK_WINDOW(window), 1440, 920);
  view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(view));
  webkit_web_view_load_html(view, "<body style='background:#222;color:white;font:20px sans-serif'>正在启动 DeepSeek Harness…</body>", NULL);
  g_signal_connect_swapped(window, "destroy", G_CALLBACK(stop), NULL);
  g_unix_signal_add(SIGTERM, signal_cb, NULL); g_unix_signal_add(SIGINT, signal_cb, NULL);
  GIOChannel *output = g_io_channel_unix_new(output_pipe[0]);
  g_io_channel_set_close_on_unref(output, TRUE);
  g_io_channel_set_flags(output, g_io_channel_get_flags(output) | G_IO_FLAG_NONBLOCK, NULL);
  g_io_add_watch(output, G_IO_IN | G_IO_HUP | G_IO_ERR, read_server_output, NULL);
  g_io_channel_unref(output);
  g_timeout_add(300, poll_server, NULL); gtk_widget_show_all(window); gtk_main();
  if (log_file) fclose(log_file);
  g_free(exe); g_free(bin); g_free(root); g_free(node); g_free(dsh); g_free(entry); g_free(home); g_free(log);
  return 0;
}
