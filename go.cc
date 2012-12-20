#include<cstdio>
#include<queue>
#include<set>
#include<utility>
#include<stdint.h>

#define BOARD 19
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define FOREVER for(;;)
#define TO_INDEX(x,y) ((y)*BOARD+(x))
typedef struct BitBoard {
  uint64_t b[6];
  BitBoard() { this->clear(); }
  BitBoard(const BitBoard& p) {
    b[0] = p.b[0]; b[1] = p.b[1]; b[2] = p.b[2];
    b[3] = p.b[3]; b[4] = p.b[4]; b[5] = p.b[5];
  }
  uint64_t& operator[](const int i) { return b[i]; }
  bool operator==(const BitBoard& p) const {
    return b[0] == p.b[0] && b[1] == p.b[1] && b[2] == p.b[2] && b[3] == p.b[3]
      && b[4] == p.b[4] && b[5] == p.b[5];
  }
  bool empty() { return b[0] && b[1] && b[2] && b[3] && b[4] && b[5]; }
  bool testbit(int i) { return b[i/64] & (static_cast<uint64_t>(1) << (i%64)); }
  void setbit(int i) { b[i/64] |= (static_cast<uint64_t>(1) << (i%64)); }
  void clearbit(int i) { b[i/64] &= ~(static_cast<uint64_t>(1) << (i%64)); }
  void clear() { b[0] = 0; b[1] = 0; b[2] = 0; b[3] = 0; b[4] = 0; b[5] = 0; }
  bool testboard(int x, int y) { return this->testbit(y*BOARD + x); }
  void setboard(int x, int y) { this->setbit(y*BOARD + x); }
  void clearboard(int x, int y) { this->clearbit(y*BOARD + x); }
} BitBoard;

// Disjoint set structure. Implements union-find w/rank + path compression.
// Layout:
//   1 unremovable head for all empty stones
//   set of white chains - chains live/die together, so removal from set is not an issue.
//   set of black chains - addition to set/membership is trivial.
typedef struct Chain {
  int parent;
  int colour;
  Chain() : rank(0), parent(-1) {}
} Chain;

typedef struct BoardState {
  BitBoard white;
  BitBoard black;
  BoardState() { white.clear(); black.clear(); }
  BoardState(const BoardState& g) { white = g.white; black = g.black; }
  bool operator==(const BoardState& p) const {
    return white == p.white && black == p.black;
  }
  bool operator<(const BoardState& p) const { return true; }
} BoardState;

typedef struct GameBoard {
  BoardState state;
  std::set<Chain*> w_chains;
  std::set<Chain*> b_chains;
  Chain allstones[BOARD*BOARD];
  GameBoard(BoardState start) {
    state = start;
    for (int i = 0; i < BOARD*BOARD; ++i) {
      allstones[i].parent = -1;
      allstones[i].colour = EMPTY;
    }
  }
  int Find(int pos) {
    if allstones[pos].parent == pos || allstones[pos].parent == -1
      return pos
    else
      return (allstones[pos].parent = Find(allstones[pos].parent));
  }
  void Remove(int pos) {
    int p = Find(pos); if (p != -1) allstones[p].parent = -1;
  }
  int Connected(int pos1, int pos2) {
    int p = Find(pos1);
    return p == Find(pos2) && p != -1;
  }
  int Empty(int pos1) {
    return Find(pos1) == -1;
  }
  int Union(int pos1, int pos2) {
    int p1 = Find(pos1);
    int p2 = Find(pos2);
    if (p1 == p2) return p1;
    if (allstones[p1].parent < allstones[p2].parent) { allstones[p2].parent = p1; return p1; }
    else { allstones[p1].parent = p2; return p2; }
  }
  int Add(int x, int y, int colour) {
    if (allstones[pos1].colour) { return 0; } // error, occupied cell
    // merge with chains on NSEW adjacencies
    if (x > 0) {
      int pos = TO_INDEX(x,y);
      if (allstones[TO_INDEX(x-1,y)].colour == colour) {
        Union(pos, TO_INDEX(x-1,y));
      } (allstones[TO_INDEX(x-1,y)].colour != EMPTY) {

      }
    }
    if (x < BOARD-1 && allstones[TO_INDEX(x+1,y)].colour == colour)
      Union(TO_INDEX(x,y), TO_INDEX(x+1,y));
    if (y > 0 && allstones[TO_INDEX(x,y-1)].colour == colour)
      Union(TO_INDEX(x,y), TO_INDEX(x,y-1));
    if (y < BOARD-1 && allstones[TO_INDEX(x,y+1)].colour == colour)
      Union(TO_INDEX(x,y), TO_INDEX(x,y+1));
  }
} GameBoard;

// Flood fill to get liberty count.
int liberties(int x, int y, BitBoard p1, BitBoard p2) {
  BitBoard visited;
  visited.clear();
  int start_node = y*BOARD+x;
  std::queue<int> nodes;
  nodes.push(start_node);
  int liberties = 0;
  while(!nodes.empty()) {
    int current = nodes.front();
    nodes.pop();
    int xx = current%BOARD, yy = current/BOARD;
    if (!visited.testboard(xx, yy)) {
      visited.setboard(xx, yy);
      if (xx > 0) {
        if (p1.testboard(xx-1, yy))
          nodes.push(yy*BOARD + xx-1);
        else if (!p2.testboard(xx-1, yy))
          ++liberties;
      }
      if (yy > 0) {
        if (p1.testboard(xx, yy-1))
          nodes.push((yy-1)*BOARD+xx);
        else if (!p2.testboard(xx, yy-1))
          ++liberties;
      }
      if (xx < BOARD) {
        if (p1.testboard(xx+1, yy))
          nodes.push(yy*BOARD+xx+1);
        else if (!p2.testboard(xx+1, yy))
          ++liberties;
      }
      if (yy < BOARD) {
        if (p1.testboard(xx, yy+1))
          nodes.push((yy+1)*BOARD+xx);
        else if (!p2.testboard(xx, yy+1))
          ++liberties;
      }
    }
  }
  return liberties;
}

// clear dead stones from board.
// TODO: cache already computed squares when we clear chains.
void eval_board(GameBoard& b) {
  for (int i = 0; i < BOARD; ++i) {
    for (int j = 0; j < BOARD; ++j) {
      if (!liberties(i, j, b.state.white, b.state.black)) {
        b.state.white.clearboard(i, j);
        Chain* p = b.allstones[i][j].Find();
        p->parent = &b.empty;
      } else if (!liberties(i, j, b.state.black, b.state.white)) {
        b.state.black.clearboard(i, j);
        Chain* p = b.allstones[i][j].Find();
        p->parent = &b.empty;
      }
    }
  }
}

void make_move(uint8_t p, GameBoard& gb, std::set<BoardState>& past) {
  char x;
  unsigned int y;
  BitBoard p1, p2;
  p1 = p ? gb.state.white : gb.state.black;
  p2 = p ? gb.state.black : gb.state.white;
  while (1) {
    if (p) fputs("White to move: ", stdout);
    else fputs("Black to move: ", stdout);
    if (scanf("%c%u", &x, &y) != 2) continue;
    int xx = static_cast<int>(x-'A');
    int yy = static_cast<int>(y)-1;
    if (xx < 0 || xx > 18 || yy < 0 || yy > 18) {
      fprintf(stderr, "ILLEGAL MOVE: Invalid point\n");
      continue;
    }
    if (p1.testboard(xx, yy) || p2.testboard(xx, yy) || !liberties(xx, yy, p1, p2)) {
      fprintf(stderr, "ILLEGAL MOVE: Point %c%u is already occupied or has no liberties!\n", x, y);
      continue;
    }
    GameBoard t(gb);
    if (p) {
      t.state.white.setboard(xx, yy);
    } else {
      t.state.black.setboard(xx, yy);
    }
    eval_board(t);
    if (past.find(t.state) != past.end()) {
      fprintf(stderr, "ILLEGAL MOVE: Repeated board state\n");
      continue;
    }
    past.insert(t.state);
    gb = t;
    return;
  }
}

const int ESC = 27;
void ANSI_goto_column(int col) { printf("%c[%dG", ESC, col); }
void ANSI_double_thick() { printf("%c#6", ESC); }
void ANSI_set_colour(int c) { printf("%c[%dm", ESC, c); }
void ANSI_clear_colour() { printf("%c[m", ESC); }
void ANSI_cursor_down(int x) { printf("%c[%dB", ESC, x); }
void ANSI_cursor_move(int r, int c) { printf("%c[%d;%dH", ESC, r, c); }
void ANSI_clear_screen(void) { printf("%c[2J", ESC); }

#define EMPTY_AA  43
#define WHITE_AA 103
#define BLACK_AA  30
void print_state(BoardState& b) {
  //*
  ANSI_clear_screen();
  ANSI_cursor_move(5, 5);
  printf("   ABCDEFGHIKLMNOPQRST");
  ANSI_double_thick();
  ANSI_cursor_down(1);
  // */
  for (int i = 0; i < BOARD; ++i) {
    //*
    ANSI_goto_column(5);
    ANSI_double_thick();
    printf("%2d ", BOARD-i);
    // */
    for (int j = 0; j < BOARD; ++j) {
      int colour = b.white.testboard(j, BOARD-i-1) ? WHITE_AA :
        b.black.testboard(j, BOARD-i-1) ? BLACK_AA : EMPTY_AA;
      if (colour != EMPTY_AA) {
        /*
        if (colour == BLACK) {
          fprintf(stderr, "BLACK: %d %d\n", j, BOARD-i-1);
        } else {
          fprintf(stderr, "WHITE: %d %d\n", j, BOARD-i-1);
        }
        // */
      }
      //*
      ANSI_set_colour(colour);
      printf(" ");
      ANSI_clear_colour();
      // */
    }
    //*
    printf(" %2d ", BOARD-i);
    ANSI_cursor_down(1);
    // */
  }
  //*
  ANSI_goto_column(5);
  printf("   ABCDEFGHIKLMNOPQRST");
  ANSI_cursor_down(1);
  ANSI_goto_column(5);
  // */
}

int main(int argc, char** argv) {
  BoardState start;
  GameBoard b(start);
  std::set<BoardState> past_states;
  past_states.insert(start);
  print_state(start);
  FOREVER {
    make_move(1, b, past_states);
    print_state(b.state);
    make_move(0, b, past_states);
    print_state(b.state);
  }
  return 0;
}
