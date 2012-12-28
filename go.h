#pragma once
#ifndef __GO_CC
#define __GO_CC

#define BOARD 19
#define EMPTY 0
#define BLACK 1
#define WHITE 2
#define FOREVER for(;;)
#define TO_INDEX(x,y) ((y)*BOARD+(x))

#define X_OFFSET(_a) (static_cast<int>(_a-'A'))
#define Y_OFFSET(_a) (static_cast<int>(_a)-1)
#define ESC 27

typedef enum MoveStatus {
  OK,
  OCCUPIED,
  SUICIDE,
  SUPERKO,
  INVALID_PT,
  GAME_OVER,
  RESIGN,
  DISPUTE
} MoveStatus;

// Disjoint set structure. Implements union-find w/rank + path compression.
// Layout:
//   1 unremovable head for all empty stones
//   set of white chains - chains live/die together, so removal from set is not an issue.
//   set of black chains - addition to set/membership is trivial.
typedef struct Chain {
  int parent;
  int colour;
  int liberties; // empty adjacents
  int rank;
  uint64_t hash;
  Chain() : colour(EMPTY), parent(-1), liberties(0), counter(0), hash(0) {}
} Chain;

inline static int east_of(int x, int y) { return (x-1) >= 0 ? y*BOARD+x-1 : -1; }
inline static int west_of(int x, int y) { return x+1 < BOARD ? y*BOARD+x+1 : -1; }
inline static int north_of(int x, int y) { return y+1 < BOARD ? (y+1)*BOARD+x : -1; }
inline static int south_of(int x, int y) { return (y-1) >= 0 ? (y-1)*BOARD+x : -1; }

inline static int east_ofp(int x) { return (x%BOARD)-1 >= 0 ? x-1 : -1; }
inline static int west_ofp(int x) { return (x%BOARD)+1 < BOARD ? x+1 : -1; }
inline static int north_ofp(int x) { return (x/BOARD)+1 < BOARD ? x+BOARD : -1; }
inline static int south_ofp(int x) { return (x/BOARD)-1 >= 0 ? x-BOARD : -1; }

static uint64_t rand64() {
  return static_cast<uint64_t>(rand()) ^ (static_cast<uint64_t>(rand()) << 15)
                ^ (static_cast<uint64_t>(rand()) << 30)
                ^ (static_cast<uint64_t>(rand()) << 45)
                ^ (static_cast<uint64_t>(rand()) << 60);
}

typedef struct GameBoard {
  int passes;
  uint64_t hash;
  Chain allstones[BOARD*BOARD];
  // stuff for superko detection.
  uint64_t zobrist[BOARD*BOARD][2];
  std::set<uint64_t> past_states;
  void gen_zobrist() {
    for (int i = 0; i < BOARD*BOARD; ++i) {
      zobrist[i][0] = rand64();
      zobrist[i][1] = rand64();
    }
  }
  void clear() {
    hash = 0;
    passes = 0;
    past_states.insert(hash);
    for (int i = 0; i < BOARD; ++i) {
      for (int j = 0; j < BOARD; ++j) {
        allstones[TO_INDEX(j, i)].parent = TO_INDEX(j, i);
        allstones[TO_INDEX(j, i)].colour = EMPTY;
        allstones[TO_INDEX(j, i)].liberties = 0;
        if (east_of(j, i) >= 0) allstones[TO_INDEX(j, i)].liberties++;
        if (west_of(j, i) >= 0) allstones[TO_INDEX(j, i)].liberties++;
        if (south_of(j, i) >= 0) allstones[TO_INDEX(j, i)].liberties++;
        if (north_of(j, i) >= 0) allstones[TO_INDEX(j, i)].liberties++;
      }
    }
  }
  GameBoard() {
    gen_zobrist(); // generate zobrist hash seeds.
    clear();
  }
  int Find(int pos) {
    if (allstones[pos].parent == pos) return pos;
    else if (allstones[pos].rank > allstones[allstones[pos].parent].rank) {
      allstones[pos].rank = 0; allstones[pos].colour = EMPTY; return (allstones[pos].parent = pos);
    } else return (allstones[pos].parent = Find(allstones[pos].parent));
  }
  void Remove(int pos) { // reset rank, all children will disengage on next find.
    allstones[pos].colour = EMPTY;
    allstones[pos].parent = pos;
    allstones[pos].rank = 0;
    hash ^= allstones[pos].hash;
    allstones[pos].hash = 0;
  }
  int Connected(int pos1, int pos2) { return Find(pos1) == Find(pos2); }
  int Empty(int pos) { return allstones[Find(pos)].colour == EMPTY; }
  int UnionAdd(int pos1, int pos2) {
    int p1 = pos1, p2 = Find(pos2);
    if (p1 == p2) return p1;
    if (allstones[p1].rank >= allstones[p2].rank) {
      allstones[p1].rank++;
      allstones[p1].liberties += allstones[p2].liberties;
      allstones[p2].liberties = 0;
      allstones[p1].hash ^= allstones[p2].hash; allstones[p2].hash = 0;
      allstones[p2].parent = p1; return p1;
    } else {
      allstones[p2].rank++;
      allstones[p2].liberties += allstones[p1].liberties;
      allstones[p1].liberties = 0;
      allstones[p1].parent = p2; return p2;
    }
  }
  int Union(int pos1, int pos2) { return UnionAdd(Find(pos1), pos2); }
  MoveStatus Add(int x, int y, int colour) {
    if (x < 0 || x >= BOARD || y < 0 || y >= BOARD) { return INVALID_PT; }
    int pos = TO_INDEX(x,y);
    if (allstones[pos].colour != EMPTY) { return OCCUPIED; } // error, occupied cell
    int east = east_of(x,y), west = west_of(x,y);
    int south = south_of(x,y), north = north_of(x,y);
    // check for suicides.
    int save = 0;
#define CHECKED_LIBERTY_ON(dir) \
    if (dir >= 0) { \
      int dir_rep = Find(dir); \
      if (allstones[dir_rep].colour == colour) { \
        if (allstones[dir_rep].liberties > 1) { save = 1; } \
      } else if (allstones[dir_rep].liberties == 1) { save = 1; } \
    }

    if (!allstones[pos].liberties) {
      CHECKED_LIBERTY_ON(east); CHECKED_LIBERTY_ON(west);
      CHECKED_LIBERTY_ON(south); CHECKED_LIBERTY_ON(north);
      if (!save) return SUICIDE;
    }

    // Update hash and check for superko. TODO: eval board, THEN superko? not sure what is actually right.
    uint64_t new_hash = hash ^ zobrist[pos][colour-1];
    if (past_states.find(new_hash) != past_states.end()) { return SUPERKO; }
    else { hash = new_hash; allstones[pos].hash ^= zobrist[pos][colour-1]; past_states.insert(hash); }

    // merge with chains on NSEW adjacencies
#define CHECKED_UNION_ON(dir) \
    if (dir >= 0) {           \
      int dir_rep = Find(dir); \
      allstones[dir_rep].liberties--; \
      if (allstones[dir_rep].colour == colour) { UnionAdd(pos, dir_rep); } \
      else if (allstones[dir].colour != EMPTY) { if (!allstones[dir_rep].liberties) Remove(dir); } \
    }

    CHECKED_UNION_ON(east); CHECKED_UNION_ON(west);
    CHECKED_UNION_ON(south); CHECKED_UNION_ON(north);

    allstones[pos].colour = colour;
    return OK;
  }
} GameBoard;

#endif /* end of include guard: __GO_CC */
