#include "dropblox_ai.h"

using namespace json;
using namespace std;

//----------------------------------
// Block implementation starts here!
//----------------------------------

Block::Block(Object& raw_block) {
  center.i = (int)(Number&)raw_block["center"]["i"];
  center.j = (int)(Number&)raw_block["center"]["j"];
  size = 0;

  Array& raw_offsets = raw_block["offsets"];
  for (Array::const_iterator it = raw_offsets.Begin(); it < raw_offsets.End(); it++) {
    size += 1;
  }
  for (int i = 0; i < size; i++) {
    offsets[i].i = (Number&)raw_offsets[i]["i"];
    offsets[i].j = (Number&)raw_offsets[i]["j"];
  }

  translation.i = 0;
  translation.j = 0;
  rotation = 0;
}

void Block::left() {
  translation.j -= 1;
}

void Block::right() {
  translation.j += 1;
}

void Block::up() {
  translation.i -= 1;
}

void Block::down() {
  translation.i += 1;
}

void Block::rotate() {
  rotation += 1;
}

void Block::unrotate() {
  rotation -= 1;
}

// The checked_* methods below perform an operation on the block
// only if it's a legal move on the passed in board.  They
// return true if the move succeeded.
//
// The block is still assumed to start in a legal position.
bool Block::checked_left(const Board& board) {
  left();
  if (board.check(*this)) {
    return true;
  }
  right();
  return false;
}

bool Block::checked_right(const Board& board) {
  right();
  if (board.check(*this)) {
    return true;
  }
  left();
  return false;
}

bool Block::checked_up(const Board& board) {
  up();
  if (board.check(*this)) {
    return true;
  }
  down();
  return false;
}

bool Block::checked_down(const Board& board) {
  down();
  if (board.check(*this)) {
    return true;
  }
  up();
  return false;
}

bool Block::checked_rotate(const Board& board) {
  rotate();
  if (board.check(*this)) {
    return true;
  }
  unrotate();
  return false;
}

void Block::do_command(const string& command) {
  if (command == "left") {
    left();
  } else if (command == "right") {
    right();
  } else if (command == "up") {
    up();
  } else if (command == "down") {
    down();
  } else if (command == "rotate") {
    rotate();
  } else {
    throw Exception("Invalid command " + command);
  }
}

void Block::do_commands(const vector<string>& commands) {
  for (int i = 0; i < commands.size(); i++) {
    do_command(commands[i]);
  }
}

void Block::reset_position() {
  translation.i = 0;
  translation.j = 0;
  rotation = 0;
}

int Block::getLowest(){
  int lowest = 0;
  int y = translation.j;
  for (int i = 0; i < size; i++){
      if(translation.j+ offsets[i].j> y){
         lowest = i;
         y = translation.j+ offsets[i].j;
      }
  }
  return lowest; 
}

//----------------------------------
// Board implementation starts here!
//----------------------------------

Board::Board() {
  rows = ROWS;
  cols = COLS;
}

Board::Board(Object& state) {
  rows = ROWS;
  cols = COLS;

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      bitmap[i][j] = ((int)(Number&)state["bitmap"][i][j] ? 1 : 0);
    }
  }

  // Note that these blocks are NEVER destructed! This is because calling
  // place() on a board will create new boards which share these objects.
  //
  // There's a memory leak here, but it's okay: blocks are only constructed
  // when you construct a board from a JSON Object, which should only happen
  // for the very first board. The total memory leaked will only be ~10 kb.
  block = new Block(state["block"]);
  for (int i = 0; i < PREVIEW_SIZE; i++) {
    preview.push_back(new Block(state["preview"][i]));
  }
}

// Returns true if the `query` block is in valid position - that is, if all of
// its squares are in bounds and are currently unoccupied.
bool Board::check(const Block& query) const {
  Point point;
  for (int i = 0; i < query.size; i++) {
    point.i = query.center.i + query.translation.i;
    point.j = query.center.j + query.translation.j;
    if (query.rotation % 2) {
      point.i += (2 - query.rotation)*query.offsets[i].j;
      point.j +=  -(2 - query.rotation)*query.offsets[i].i;
    } else {
      point.i += (1 - query.rotation)*query.offsets[i].i;
      point.j += (1 - query.rotation)*query.offsets[i].j;
    }
    if (point.i < 0 || point.i >= ROWS ||
        point.j < 0 || point.j >= COLS || bitmap[point.i][point.j]) {
      return false;
    }
  }
  return true;
}

// Resets the block's position, moves it according to the given commands, then
// drops it onto the board. Returns a pointer to the new board state object.
//
// Throws an exception if the block is ever in an invalid position.
Board* Board::do_commands(const vector<string>& commands) {
  block->reset_position();
  if (!check(*block)) {
    throw Exception("Block started in an invalid position");
  }
  for (int i = 0; i < commands.size(); i++) {
    if (commands[i] == "drop") {
      return place();
    } else {
      block->do_command(commands[i]);
      if (!check(*block)) {
        throw Exception("Block reached in an invalid position");
      }
    }
  }
  // If we've gotten here, there was no "drop" command. Drop anyway.
  return place();
}

// Drops the block from whatever position it is currently at. Returns a
// pointer to the new board state object, with the next block drawn from the
// preview list.
//
// Assumes the block starts out in valid position.
// This method translates the current block downwards.
//
// If there are no blocks left in the preview list, this method will fail badly!
// This is okay because we don't expect to look ahead that far.
Board* Board::place() {
  Board* new_board = new Board();

  while (check(*block)) {
    block->down();
  }
  block->up();

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      new_board->bitmap[i][j] = bitmap[i][j];
    }
  }

  Point point;
  for (int i = 0; i < block->size; i++) {
    point.i = block->center.i + block->translation.i;
    point.j = block->center.j + block->translation.j;
    if (block->rotation % 2) {
      point.i += (2 - block->rotation)*block->offsets[i].j;
      point.j +=  -(2 - block->rotation)*block->offsets[i].i;
    } else {
      point.i += (1 - block->rotation)*block->offsets[i].i;
      point.j += (1 - block->rotation)*block->offsets[i].j;
    }
    new_board->bitmap[point.i][point.j] = 1;
  }
  Board::remove_rows(&(new_board->bitmap));

  new_board->block = preview[0];
  for (int i = 1; i < preview.size(); i++) {
    new_board->preview.push_back(preview[i]);
  }

  return new_board;
}

// A static method that takes in a new_bitmap and removes any full rows from it.
// Mutates the new_bitmap in place.
void Board::remove_rows(Bitmap* new_bitmap) {
  int rows_removed = 0;
  for (int i = ROWS - 1; i >= 0; i--) {
    bool full = true;
    for (int j = 0; j < COLS; j++) {
      if (!(*new_bitmap)[i][j]) {
        full = false;
        break;
      }
    }
    if (full) {
      rows_removed += 1;
    } else if (rows_removed) {
      for (int j = 0; j < COLS; j++) {
        (*new_bitmap)[i + rows_removed][j] = (*new_bitmap)[i][j];
      }
    }
  }
  for (int i = 0; i < rows_removed; i++) {
    for (int j = 0; j < COLS; j++) {
      (*new_bitmap)[i][j] = 0;
    }
  }
}

int Board::getBottomEmptyRow(){
    for(int i = ROWS-1; i>=0; i--){
       for(int j = 0; j< COLS; j++){
          if (bitmap[i][j] == 0){
              return i;
          }
       }
    }
    return 0;
}


int* Board::getDestination(int row){
  int* destination = new int[3];
  destination[0] = -1;
  int col = 0;
  for (int j = 0; j < COLS; ++j) {
    if (!bitmap[row][j]) {
      col = j;
    }
    Block* current= new Block(*this->block);
    for(int rotate_num = 0; rotate_num< 4; rotate_num++){
      current->rotate();
      Block* temp = new Block(*current);
      int lowest = temp->getLowest();
      int x = j - (temp->center.j+temp->offsets[lowest].j);
      int y = (this->getBottomEmptyRow())-(temp->center.j+temp->offsets[lowest].i);
      temp->center.i = temp->center.i + y;
      temp->center.j = temp->center.j + x;

      if(this->check(*temp)){
        destination[2] = rotate_num;
        destination[1] = (temp->translation).j;
        destination[0] = (temp->translation).i;
      }
    }
    if(destination[0]!=-1){
      return destination;
    }
  }
  return NULL;
}

// Asserts: point is legal
// Nothing is between block and location
void Board::place_block(vector<string> * moves,
                          const int rotation,
                          const int i,
                          const int j) {
  Block *block = this->block;
  for (int k = 0; k < rotation; ++k) {
    block->rotate();
    moves->push_back("rotate");
  }
  const int blockJ = block->center.j;
  if (blockJ < j) {
    for (int k = 0; k < j - blockJ; ++k) {
      block->right();
      moves->push_back("right");
    }
  } else {
    for (int k = 0; k < blockJ - j; ++k) {
      block->left();
      moves->push_back("left");
    }
  }
}


 

vector<string> Board::generate() {
  vector<vector<string> > commands;
  vector<string> c;
  vector<string> c2;
  commands.push_back(c);

  for (int j = 0; j < 6; ++j) {
    c.push_back("left");
    c2.push_back("right");

    for (int r = 0; r < 3; ++r) {
      commands.push_back(c);
      commands.push_back(c2);
      c.push_back("rotate");
      c2.push_back("rotate");
    }

  }
  int minimum = 999999999;
  int index = 0;
  for (int i = 0; i < commands.size(); ++i) {
    Board* b = do_commands(commands[i]);
    int score = b->score();
    if (score < minimum) {
      minimum = score;
      index = i;
    }
  }
  return commands[index];
}

int Board::score(){
  // Find number of holes
  for (int i = ROWS - 1; i > 0; --i) {
    int all_zeros = 0;
    for (int j = 0; j < COLS; ++j) {
    }
  }
  return 0;
}

int main(int argc, char** argv) {
  // Construct a JSON Object with the given game state.
  istringstream raw_state(argv[1]);
  Object state;
  Reader::Read(state, raw_state);

  // Construct a board from this Object.
  Board board(state);

  // Make some moves!
  vector<string> moves;
//  board.place_block(&moves,0,4,0);
  /*
  while (board.check(*board.block)) {
    board.block->right();
    moves.push_back("right");
  }
  */
  int* x = board.getDestination(board.getBottomEmptyRow());
  board.place_block(&moves, x[0], x[1], x[2]);
  // Ignore the last move, because it moved the block into invalid
  // position. Make all the rest.
  for (int i = 0; i < moves.size() - 1; i++) {
    cout << moves[i] << endl;
  }
}
