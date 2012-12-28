#include<stdint.h>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<set>
#include<utility>

#include<go.h>

#define OUT stdout
#define PROTOCOL_VERSION 2
#define NAME "go.cc"
#define VERSION "0.1"
#define SUCCESS "= "
#define ERROR "? "
#define TERM "\n\n"

typedef struct command {
  const char* name;
  void (*func)(char*, int, GameBoard& b);
} command;

static inline void response(const char* preamble, int id, const char* fmt, ...) {
  va_list va;
  fputs(preamble, OUT);
  if (id > 0) fprintf(OUT, "%d", id);
  va_start(va,fmt);
  vfprintf(OUT, fmt, va);
  va_end(va);
  fputs(TERM, OUT);
}

void protocol_version(char* s, int id, GameBoard& b) { response(SUCCESS, id,  "%d", PROTOCOL_VERSION); }
void name(char* s, int id, GameBoard& b) { response(SUCCESS, id, "%s", NAME); }
void version(char* s, int id, GameBoard& b) { response(SUCCESS, id, "%s", VERSION); }
void known_command(char* s, int id, GameBoard& b); // fwd declare
void list_commands(char* s, int id, GameBoard& b); // fwd declare
void quit(char* s, int id, GameBoard& b) { response(SUCCESS, id, "%s", ""); fflush(OUT); exit( 0 ); }
// board mutators. TODO: fill in stubs.
void boardsize(char* s, int id, GameBoard& b) {
  int i = BOARD; // we only support the precompiled board size because lazy.
  if (sscanf(s, "%d", &i) == 1 && i == BOARD) response(SUCCESS, id, "%s", "");
  else response(ERROR, id, "%s", "unacceptable size");
}
void clear_board(char *s, int id, GameBoard& b) { b.clear(); response(SUCCESS, id, ""); }
void komi(char *s, int id, GameBoard& b) {
  int i = 0;
  if (sscanf(s, "%d", &i) == 1) { b.set_komi(i); response(SUCCESS, id, ""); }
  else { response(ERROR, id, "invalid komi"); }
}
void play(char *s, int id, GameBoard& b) {
  char colour[128];
  char x; int y, ret = INVALID_PT;
  if (sscanf(s, "%s %c%u", colour, &x, &y) != 3) { response(ERROR, id, "Invalid input"); return; }
  if (strcasecmp(colour, "white") == 0 || strcasecmp(colour, "w"))
    ret = b.Add(X_OFFSET(x), Y_OFFSET(y), WHITE);
  if (strcasecmp(colour, "black") == 0 || strcasecmp(colour, "b"))
    ret = b.Add(X_OFFSET(x), Y_OFFSET(y), BLACK);
  if (ret == OK) response(SUCCESS, id, "");
  else response(ERROR, id, "illegal move");
}
// needed for bots.
void genmove(char *s, int id, GameBoard& b) { response(SUCCESS, id, ""); }
void final_status_list(char *s, int id, GameBoard& b) { response(SUCCESS, id, "pass"); }

static command commands[] = {
  { "protocol_version", &protocol_version },
  { "name", &name },
  { "version", &version },
  { "known_command", &known_command },
  { "list_commands", &list_commands },
  { "quit", &quit },
  { "boardsize", &boardsize },
  { "clear_board", &clear_board },
  { "komi", &komi },
  { "play", &play },
  { "genmove", &genmove },
  { "final_status_list", &final_status_list },
  { NULL, NULL }
};

void known_command(char* s, int id, GameBoard& b) {
  for (int i = 0; commands[i].name != NULL; ++i) {
    if (!strcmp(s, commands[i].name)) {
      response(SUCCESS, id, "true");
      return;
    }
  }
  response(SUCCESS, id, "false");
}

void list_commands(char *s, int id, GameBoard& b) {
  if (id > 0) fprintf(OUT, SUCCESS "%d", id);
  for (int i = 0; commands[i].name != NULL; ++i)
    fprintf(OUT, "%s ", commands[i].name);
  fputs(TERM, OUT);
}

void main_loop() {
  char buf[128], name[128], *p;
  int n, i, id, hit;
  GameBoard b;
  FOREVER {
    i = -1; hit = 0;
    if (!fgets(buf, 127, stdin)) break; // we're done.
    if ((p = strchr(buf, '#'))) *p = '\0'; // axe comments.
    p = &buf[0];
    if (sscanf(p, "%d%n", &id, &n) == 1) p += n; // get id
    if (sscanf(p, " %s %n", &name[0], &n) != 1) continue;
    p += n;
    for (int j = 0; commands[j].name != NULL; ++j) {
      if (strcmp(name, commands[j].name) == 0) {
        (*commands[j].func)(p, id, b); hit = 1; break;
      }
    }
    if (!hit) response(ERROR, id, "unknown command");
  }
}

int main() {
  main_loop();
  return 0;
}
