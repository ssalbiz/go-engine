#include<stdint.h>
#include<cstdio>
#include<cstdlib>
#include<set>
#include<utility>

#include<go.h>

int make_move(int p, GameBoard& gb) {
  char x;
  unsigned int y;
  FOREVER {
    if (p == WHITE) fputs("White to move: ", stdout);
    else fputs("Black to move: ", stdout);
    if (scanf("%c%u", &x, &y) != 2) continue;
    // allow a pass.
    if (x == '-') {if (gb.passes >= 2) {return GAME_OVER;} else {return OK; }}
    else gb.passes = 0;
    // allow a resignation.
    if (x == 'r') return RESIGN;
    int xx = X_OFFSET(x);
    int yy = Y_OFFSET(y);
    int ret = gb.Add(xx, yy, p);
    switch (ret) {
      case OK: return OK;
      case INVALID_PT: fprintf(stderr, "ILLEGAL MOVE: Invalid point\n"); break;
      case OCCUPIED: fprintf(stderr, "ILLEGAL MOVE: OCCUPIED SQUARE\n"); break;
      case SUICIDE: fprintf(stderr, "ILLEGAL MOVE: SUICIDE SQUARE\n"); break;
      case SUPERKO: fprintf(stderr, "ILLEGAL MOVE: SUPERKO VIOLATION\n"); break;
    }
  }
}

void ANSI_cursor_move(int r, int c) { printf("%c[%d;%dH", ESC, r, c); }
void ANSI_goto_column(int col) { printf("%c[%dG", ESC, col); }
void ANSI_double_thick() { printf("%c#6", ESC); }
void ANSI_set_colour(int c) { printf("%c[%dm", ESC, c); }
void ANSI_clear_colour() { printf("%c[m", ESC); }
void ANSI_cursor_down(int x) { printf("%c[%dB", ESC, x); }
void ANSI_clear_screen(void) { printf("%c[2J", ESC); }

#define EMPTY_AA  43
#define WHITE_AA 107
#define BLACK_AA  40
void print_state(GameBoard& b) {
  int blacks = 0, whites = 0;
  ANSI_clear_screen();
  ANSI_cursor_move(5, 5);
  printf("   ABCDEFGHIKLMNOPQRST");
  ANSI_double_thick();
  ANSI_cursor_down(1);
  for (int i = 0; i < BOARD; ++i) {
    ANSI_goto_column(5);
    ANSI_double_thick();
    printf("%2d ", BOARD-i);
    for (int j = 0; j < BOARD; ++j) {
      int colour = EMPTY_AA;
      int pos = b.Find(TO_INDEX(j, BOARD-i-1));
      colour = b.allstones[pos].colour == BLACK ? BLACK_AA : b.allstones[pos].colour == WHITE ? WHITE_AA : EMPTY_AA;
      if (colour != EMPTY_AA) {
        if (colour == BLACK_AA) { blacks++; } else { whites++; }
      }
      ANSI_set_colour(colour);
      printf(" ");
      ANSI_clear_colour();
    }
    printf(" %2d ", BOARD-i);
    ANSI_cursor_down(1);
  }
  ANSI_goto_column(5);
  printf("   ABCDEFGHIKLMNOPQRST");
  ANSI_cursor_down(1);
  printf(" B: %d W: %d HASH: %lu", blacks, whites, b.hash);
  ANSI_cursor_down(1);
  ANSI_goto_column(5);
}

std::pair<int, int> score(GameBoard& b) {
  std::pair<int, int> scores(0, 0);
  return scores;
}

void print_result(std::pair<int, int>& p) {
  fprintf(stderr, "GAME OVER: WHITE: %d, BLACK: %d\n", p.first, p.second);
}

static inline void dispute_msg() {
  fprintf(stderr, "DISPUTE: resume play.\n");
}

// Agree on dead stones.
void end_game(GameBoard& b) {
  fprintf(stderr, "Select stones to remove");
  char x;
  int y, pos;
  std::set<int> stones;
  FOREVER {
    if (scanf("%c%d", &x, &y) != 1) continue;
    pos = TO_INDEX(X_OFFSET(x), Y_OFFSET(y));
    if (x >= 0 && b.allstones[pos].colour != EMPTY) stones.insert(pos);
    else break;
  }
  FOREVER {
    if (scanf("%c%d", &x, &y) != 1) continue;
    pos = TO_INDEX(X_OFFSET(x), Y_OFFSET(y));
    if (X_OFFSET(x) >= 0) {
      if (stones.find(pos) == stones.end()) { dispute_msg(); return; }
      stones.erase(pos);
    }
  }
  if (!stones.empty()) { dispute_msg(); return; }
  // Kill relevant stones.
  for (std::set<int>::iterator i = stones.begin(); i != stones.end(); ++i) {
    b.Remove(*i);
  }
  std::pair<int, int> scores = score(b);
  print_result(scores);
  exit( 0 ); // we're done here.
}

int main(int argc, char** argv) {
  GameBoard b;
  print_state(b);
  FOREVER {
    if (make_move(WHITE, b) == GAME_OVER) end_game(b);
    print_state(b);
    if (make_move(BLACK, b) == GAME_OVER) end_game(b);
    print_state(b);
  }
  return 0;
}
