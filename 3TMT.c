#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tigr.h"

static enum RuntimeFlags {
  PRINT = 1 << 0,
  GRAPHICS = 1 << 2,
} runtimeFlags = 0;

/* Special Characters */
#define NO_READ '/'
#define SHIFT_LEFT '-'
#define SHIFT_STAY '.'
#define SHIFT_RIGHT '+'
#define BLANK '\0'
#define IO_START ((char)0xff)

typedef char boolean;
#define true (1 == 1)
#define false (!true)

struct Graphics {
  Tigr* screen;
} gfx;


enum TapeType {
  INPUT,
  HISTORY,
  OUTPUT,
  NUM_TAPES
};

typedef struct State {
  enum {
    A,
    Ap,
    B,
    Bp,
    C,
    Cp
  } stage;
  int num;
} State;

void
printState(State s) {
  const char* stageNames = "ABC";
  printf("%c", stageNames[s.stage/2]);
  if (s.stage % 2) printf("'");
  printf("%d", s.num);
}

typedef struct Value {
  enum {
    SHIFT,
    NUMBER,
    SYMBOL,
    SPECIAL
  } type;
  union {
    enum {
      SHLFT,
      SHRGT,
      SHSTY
    } shift;
    int number;
    int symbolIndex;
  } value;
} Value;

typedef struct Quintuple {
  char entry;
  char output;
  int shift;
  State entryState;
  State outState;
  int m;
} Quintuple;

typedef struct Tape {
  char* tape;
  int head;
} Tape;

typedef struct Quadruple {
  char entry[NUM_TAPES];
  char output[NUM_TAPES];
  State entryState;
  State outState;
} Quadruple;

typedef struct TuringMachine {
  Tape tapes[NUM_TAPES];
  Quadruple* quadruples;
  int quadruplesSize;
  int tapeSize;
  State state;
  State acceptState;
  boolean started;
} TuringMachine;

void
fatalError(const char* message, int exitCode) {
  fprintf(stderr, "ERROR: %s\n", message);
  exit(exitCode);
}

void
print4ple(Quadruple quad) {
  int i;
  printState(quad.entryState);
  printf("[");
  for (i = 0; i < NUM_TAPES; i++) {
    if (i == 1)
    printf("%d ", quad.entry[i]);
    else
    printf("%c ", quad.entry[i] ? quad.entry[i] : 'B');
  }
  printf("] -> [");

  for (i = 0; i < NUM_TAPES; i++) {
    if (i == 1)
    printf("%d ", quad.output[i]);
    else
    printf("%c ", quad.output[i] ? quad.output[i] : 'B');
  }
  printf("]");
  printState(quad.outState);
  printf("\n");
}

void
print5ple(Quintuple quint) {
  printf("(");
  printState(quint.entryState);
  printf(" %c) -> (", quint.entry);
  printState(quint.outState);
  printf("%c %d)\n", quint.output, quint.shift);
}


boolean
validate4ple(Quadruple quad) {
  int i;
  for (i = 0; i < NUM_TAPES; i++) {
    if (quad.entry[i] != NO_READ) {
      if (quad.output[i] == SHIFT_LEFT) return false;
      if (quad.output[i] == SHIFT_STAY) return false;
      if (quad.output[i] == SHIFT_RIGHT) return false;
    }
  }
  return true;
}

boolean
statesEqual(State a, State b) {
  return (a.stage == b.stage) && (a.num == b.num);
}

boolean
validate4pleDomainOverlap(Quadruple a, Quadruple b) {
  int i;
  if (!statesEqual(a.entryState, b.entryState)) return true;
  for (i = 0; i < NUM_TAPES; i++) {
    if ((a.entry[i] != b.entry[i]) && (a.entry[i] != NO_READ) && (b.entry[i] != NO_READ)) return true;
  }
  return false;
}

boolean
validate4pleRangeOverlap(Quadruple a, Quadruple b) {
  int i;
  if (!statesEqual(a.outState, b.outState)) return true;
  for (i = 0; i < NUM_TAPES; i++) {
    if ((a.output[i] != b.output[i]) && (a.entry[i] != NO_READ) && (b.entry[i] != NO_READ)) return true;
  }
  return false;
}

boolean
validate4pleOverlap(Quadruple a, Quadruple b, boolean* range, boolean* domain) {
  *range = validate4pleRangeOverlap(a, b);
  *domain = validate4pleDomainOverlap(a, b);
  return *range && *domain;
}

boolean
validateTuringMachine(TuringMachine tm) {
  int i, j;
  boolean overlapsInDomain, overlapsInRange;
  for (i = 0; i < tm.quadruplesSize; i++) {
    if (!validate4ple(tm.quadruples[i])) {
      printf("Invalid tuple (%d)\n", i);
      return false;
    }
  }
  for (i = 0; i < tm.quadruplesSize-1; i++) {
    for (j = i+1; j < tm.quadruplesSize; j++) {
      if (!validate4pleOverlap(tm.quadruples[i], tm.quadruples[j], &overlapsInRange, &overlapsInDomain)) {
        if (!overlapsInDomain) {
          printf("Quadruples %d and %d overlap in domain\n", i, j);
        }
        if (!overlapsInRange) {
          printf("Quadruples %d and %d overlap in range\n", i, j);
        }
        print4ple(tm.quadruples[i]);
        print4ple(tm.quadruples[j]);
        return false;
      }
    }
  }
  return true;
}

void
tapeWrite(Tape* tape, char c) {
  if (tape->head >= 0) {
    tape->tape[tape->head] = c;
  } else {
    fatalError("Trying to write out of bounds", 2);
  }
}

boolean
tapeRead(Tape* tape, char expected) {
  if (expected == NO_READ) return true;
  if (tape->head < 0) {
    fatalError("Trying to read out of bounds", 1);
  }
  return tape->tape[tape->head] == expected;
}

void
apply4ple(TuringMachine* tm, Quadruple* quad) {
  int i;
  for (i = 0; i < NUM_TAPES; i++) {
    switch (quad->output[i]) {
    case SHIFT_LEFT:
      tm->tapes[i].head--;
      break;
    case SHIFT_STAY:
      break;
    case SHIFT_RIGHT:
      tm->tapes[i].head++;
      break;
    default:
      tapeWrite(tm->tapes+i, quad->output[i]);
      break;
    }
  }
  tm->state = quad->outState;
}

boolean
shouldRun4ple(TuringMachine* tm, Quadruple quad) {
  int i;
  if (!statesEqual(tm->state, quad.entryState)) return false;
  for (i = 0; i < NUM_TAPES; i++) {
    if (!tapeRead(tm->tapes+i, quad.entry[i])) return false;
  }
  return true;
}


void
convert5pleToReversible4ple(Quintuple quint, Quadruple quads[2]) {
  quads[0].entryState = quint.entryState;
  quads[0].entry[INPUT] = quint.entry;
  quads[0].entry[HISTORY] = NO_READ;
  quads[0].entry[OUTPUT] = BLANK;
  quads[0].output[INPUT] = quint.output;
  quads[0].output[HISTORY] = SHIFT_RIGHT;
  quads[0].output[OUTPUT] = BLANK;
  quads[0].outState.stage = Ap;
  quads[0].outState.num = quint.m;

  quads[1].entryState = quads[0].outState;
  quads[1].entry[INPUT] = NO_READ;
  quads[1].entry[HISTORY] = BLANK;
  quads[1].entry[OUTPUT] = NO_READ;
  if (quint.shift < 0)      quads[1].output[INPUT] = SHIFT_LEFT;
  else if (quint.shift > 0) quads[1].output[INPUT] = SHIFT_RIGHT;
  else                      quads[1].output[INPUT] = SHIFT_STAY;
  quads[1].output[HISTORY] = quint.m;
  quads[1].output[OUTPUT] = SHIFT_STAY;
  quads[1].outState = quint.outState;
}

void
convert5pleToInverse4ple(Quintuple quint, Quadruple quads[2]) {
  quads[0].entry[INPUT] = NO_READ;
  quads[0].entry[HISTORY] = quint.m;
  quads[0].entry[OUTPUT] = NO_READ;
  if (quint.shift > 0)      quads[0].output[INPUT] = SHIFT_LEFT;
  else if (quint.shift < 0) quads[0].output[INPUT] = SHIFT_RIGHT;
  else                      quads[0].output[INPUT] = SHIFT_STAY;
  quads[0].output[HISTORY] = BLANK;
  quads[0].output[OUTPUT] = SHIFT_STAY;
  quads[0].entryState.stage = C;
  quads[0].entryState.num = quint.outState.num;
  quads[0].outState.stage = Cp;
  quads[0].outState.num = quint.m;

  quads[1].entryState = quads[0].outState;
  quads[1].entry[INPUT] = quint.output;
  quads[1].entry[HISTORY] = NO_READ;
  quads[1].entry[OUTPUT] = BLANK;
  quads[1].output[INPUT] = quint.entry;
  quads[1].output[HISTORY] = SHIFT_LEFT;
  quads[1].output[OUTPUT] = BLANK;
  quads[1].outState.stage = C;
  quads[1].outState.num = quint.entryState.num;
}


boolean
initTuringMachine(TuringMachine* tm, int nRules, Quadruple* quadruples) {
  int i;
  for (i = 0; i < NUM_TAPES; i++) {
    tm->tapes[i].tape = calloc(sizeof(*tm->tapes[i].tape), 32 * 1024);
  }
  tm->tapeSize = 32 * 1024;
  tm->quadruplesSize = nRules;
  tm->quadruples = malloc(sizeof(*tm->quadruples) * nRules);
  for (i = 0; i < nRules; i++)
    tm->quadruples[i] = quadruples[i];
  return validateTuringMachine(*tm);
}


void
turingMachinePrint(TuringMachine* tm) {
  const char* tapeNames[NUM_TAPES] = { "INPUT", "HISTORY", "OUTPUT" };
  int i, j;
  char c;
  for (i = 0; i < NUM_TAPES; i++) {
    printf("%s: [", tapeNames[i]);
    j = 0;
    while (j < 32) {
      c = tm->tapes[i].tape[j];
      if (j == tm->tapes[i].head) {
        printf("(");
        printState(tm->state);
        printf(")");
      }
      if (c) {
        if (i == HISTORY)
          printf("%d ", c);
        else
          printf("%c", c);
      } else {
        printf(" ");
      }
      j++;
    }
    printf("]\n");
  }
}

int
turingMachineGo(TuringMachine* tm) {
  boolean reject = false;
  int i, j;
  if (runtimeFlags & PRINT) turingMachinePrint(tm);
  while (true) {
    reject = true;
    for (i = 0; i < tm->quadruplesSize; i++) {
      if (shouldRun4ple(tm, tm->quadruples[i])) {
        apply4ple(tm, tm->quadruples+i);
        reject = false;
        break;
      }
    }
    if (runtimeFlags & PRINT) turingMachinePrint(tm);
    if (statesEqual(tm->state, tm->acceptState)) {
      return 1;
    }
    if (reject) return 0;
    /* Grows tapes if necessary */
    for (i = 0; i < NUM_TAPES; i++) {
      if (tm->tapes[i].head >= tm->tapeSize) {
        tm->tapeSize += tm->tapeSize / 2;
        for (j = 0; j < NUM_TAPES; j++) {
          tm->tapes[j].tape = realloc(tm->tapes[j].tape, tm->tapeSize);
          if (!tm->tapes[j].tape) fatalError("Out of Memory", 3);
        }
        break;
      }
    }
  }
  return -1; 
}

void
drawChar(unsigned char c, int x, int y) {
  char buffer[2] = {0, 0};
  if (c >= 0x20 && c <= 0x7e) {
    buffer[0] = c;
    tigrPrint(gfx.screen, tfont, x, y, tigrRGB(0xbe, 0xbe, 0xbe), buffer);
    return;
  } 
  if (c == 0xff) {
    buffer[0] = '$';
    tigrPrint(gfx.screen, tfont, x, y, tigrRGB(0x7e, 0x00, 0x00), buffer);
    return;
  }
}

void
drawNum(int n, int x, int y) {
  char buffer[32] = {0};
  snprintf(buffer, 31, "%d", n);
  tigrPrint(gfx.screen, tfont, x, y, tigrRGB(0xbe, 0xbe, 0xbe), buffer);
}

void
turingMachineDraw(TuringMachine* tm) {
  const char* tapeNames[NUM_TAPES] = { "INPUT", "HISTORY", "OUTPUT" };
  int i, j;   
  const int TAPE_HEIGHT = 240 / 8;
  const int TAPE_OFFSET = 240 / 6;
  int y = TAPE_OFFSET / 2;
  int x;
  unsigned char c;
  int n;
  int ind;
  const int HEADX = 8 + 2 * (TAPE_HEIGHT + 3);
  for (i = 0; i < NUM_TAPES; i++) {
    x = 8;
    tigrFillRect(gfx.screen, x-4, y-12, (9 * (TAPE_HEIGHT + 3)) + 8, 12, tigrRGB(0x00, 0x00, 0x7e));
    tigrFillRect(gfx.screen, x-4, y-2, (9 * (TAPE_HEIGHT + 3)) + 8, TAPE_HEIGHT+8, tigrRGB(0xbe, 0xbe, 0xbe));
    tigrRect(gfx.screen, x-4, y-12, (9 * (TAPE_HEIGHT + 3)) + 8, 12, tigrRGB(0x7e, 0x7e, 0x7e));
    tigrRect(gfx.screen, x-4, y-1, (9 * (TAPE_HEIGHT + 3)) + 8, TAPE_HEIGHT+6, tigrRGB(0x7e, 0x7e, 0x7e));

    tigrPrint(gfx.screen, tfont, x, y - 10, tigrRGB(0xbe, 0xbe, 0xbe), tapeNames[i]);
    for (j = 0; j < 9; j++) {
      tigrFillRect(gfx.screen, x, y+1, TAPE_HEIGHT, TAPE_HEIGHT, tigrRGB(0xff,0xff,0xff));
      tigrRect(gfx.screen, x, y+1, TAPE_HEIGHT, TAPE_HEIGHT, tigrRGB(0,0,0));
      ind = tm->tapes[i].head + j - 2;
      if (ind >= 0) {
        n = tm->tapes[i].tape[ind];
        c = n;
        if (i == HISTORY && n)
          drawNum(c, x+5, y+5);
        else
          drawChar(c, x+5, y+5);
      }
      tigrFillRect(gfx.screen, HEADX + TAPE_HEIGHT / 4, y + 3 * TAPE_HEIGHT / 4, TAPE_HEIGHT / 2, TAPE_HEIGHT / 3, tigrRGB(0xbe, 0xbe, 0xbe));
      tigrRect(gfx.screen, HEADX + TAPE_HEIGHT / 4, y + 3 * TAPE_HEIGHT / 4, TAPE_HEIGHT / 2, TAPE_HEIGHT / 3, tigrRGB(0x00, 0x00, 0x00));

      x += TAPE_HEIGHT + 3;
    }
    y += TAPE_HEIGHT + TAPE_OFFSET;
  }

}

int
turingMachineGoGFX(TuringMachine* tm) {
  const int TICK_RATE = 10;
  boolean reject = false;
  int i, j;
  boolean done = false;
  boolean accepted = false;
  gfx.screen = tigrWindow(320, 240, "Reversible Turing Machine", 0);
  int tick = 0;
  const int RESULT_X = 8;
  const int RESULT_Y = 240 - 32;
  const int RESULT_W = 96;
  const int RESULT_H = 24;
  if (runtimeFlags & PRINT) turingMachinePrint(tm);
  while (!tigrClosed(gfx.screen)) {
    tigrClear(gfx.screen, tigrRGB(0x04, 0x7e, 0x7e));
    turingMachineDraw(tm);
    if (tigrKeyDown(gfx.screen, TK_SPACE)) tm->started = true;
    reject = true;
    if (!done && tm->started) {
      tick++;
      if (tick >= TICK_RATE) {
        tick = 0;
        for (i = 0; i < tm->quadruplesSize; i++) {
          if (shouldRun4ple(tm, tm->quadruples[i])) {
            apply4ple(tm, tm->quadruples+i);
            reject = false;
            break;
          }
        }
        if (runtimeFlags & PRINT) turingMachinePrint(tm);
        if (statesEqual(tm->state, tm->acceptState)) {
          accepted = true;
          done = true;
        }
        else if (reject) {
          accepted = false;
          done = true;
        }
        /* Grows tapes if necessary */
        for (i = 0; i < NUM_TAPES; i++) {
          if (tm->tapes[i].head >= tm->tapeSize) {
            tm->tapeSize += tm->tapeSize / 2;
            for (j = 0; j < NUM_TAPES; j++) {
              tm->tapes[j].tape = realloc(tm->tapes[j].tape, tm->tapeSize);
              if (!tm->tapes[j].tape) fatalError("Out of Memory", 3);
            }
            break;
          }
        }
      }
    }
    if (done) {
      if (accepted) {
        tigrFillRect(gfx.screen, RESULT_X, RESULT_Y, RESULT_W, RESULT_H, tigrRGB(0x06, 0xff, 0x04));
        tigrRect(gfx.screen, RESULT_X, RESULT_Y, RESULT_W, RESULT_H, tigrRGB(0x04, 0x7e, 0x00));
        tigrPrint(gfx.screen, tfont, RESULT_X+4, RESULT_Y+4, tigrRGB(0xbe, 0xbe, 0xbe), "Accepted!");

      } else {
        tigrFillRect(gfx.screen, RESULT_X, RESULT_Y, RESULT_W, RESULT_H, tigrRGB(0xfe, 0x00, 0x00));
        tigrRect(gfx.screen, RESULT_X, RESULT_Y, RESULT_W, RESULT_H, tigrRGB(0x7e, 0x00, 0x00));
        tigrPrint(gfx.screen, tfont, RESULT_X+4, RESULT_Y+4, tigrRGB(0xbe, 0xbe, 0xbe), "Rejected!");
      }
    } else {
      tigrFillRect(gfx.screen, RESULT_X, RESULT_Y, RESULT_W, RESULT_H, tigrRGB(0x7e, 0x7e, 0x7e));
      tigrRect(gfx.screen, RESULT_X, RESULT_Y, RESULT_W, RESULT_H, tigrRGB(0x00, 0x00, 0x00));
      tigrPrint(gfx.screen, tfont, RESULT_X+4, RESULT_Y+4, tigrRGB(0xbe, 0xbe, 0xbe), "Computing...");
    }
    tigrUpdate(gfx.screen);
  }
  return -1; 
}


struct Input {
  Quintuple* transitions;
  State* states;
  char* alphabet;
  char* tapeSymbols;
  int nStates;
  int nTransitions;
  int nSymbols;
  int nTapeSymbols;
  char entry[1024];
  boolean valid;
};

int
parse5ple(char* buffer, Quintuple* out) {
  char delta;
  if (sscanf(buffer, "(%d,%c)=(%d,%c,%c)\n", &out->entryState.num, &out->entry,  &out->outState.num, &out->output, &delta) != 5) {
    return 1;
  }

  switch (delta) {
  case 'L': out->shift = -1; break;
  case 'R': out->shift = +1; break;
  default:  out->shift = 0; break;
  }
  if (out->entry == 'B') out->entry = BLANK;
  if (out->output == 'B') out->output = BLANK;
  out->entryState.stage = out->outState.stage = A;
  return 0;
}

struct Input
readInput(FILE* in) {
  #define BUFFER_SIZE 1023
  struct Input input;
  int i, j;
  char buffer[BUFFER_SIZE+1];
  char* tok;
  State Sf;
  boolean validEntry = false, validOutput = false;
  Quintuple quint;
  input.valid = true;
  /* Reads in the first line containing the counts */
  fgets(buffer, BUFFER_SIZE, in);
  if (sscanf(buffer, "%d %d %d %d\n", &input.nStates, &input.nSymbols, &input.nTapeSymbols, &input.nTransitions) != 4) {
    fatalError("Malformed first line", 4);
  }

  input.nTapeSymbols += 1;
  input.nStates += 4;
  input.states = malloc(sizeof(*input.states) * input.nStates);

  /* Reads in the second line, containing the states */
  fgets(buffer, BUFFER_SIZE, in);
  tok = strtok(buffer, " ");
  for (i = 0; i < input.nStates-4; i++) {
    input.states[i+1].stage = A;
    input.states[i+1].num = atoi(tok);
    tok = strtok(NULL, " ");
  }


  /* Necessary extra states */
  input.states[0].stage = A;
  input.states[0].num = 0;

  Sf = input.states[input.nStates-4];

  input.states[input.nStates-3].stage = A;
  input.states[input.nStates-3].num = Sf.num+1;

  input.states[input.nStates-2].stage = A;
  input.states[input.nStates-2].num = Sf.num+2;

  input.states[input.nStates-1].stage = A;
  input.states[input.nStates-1].num = Sf.num+3;


  /* Reads in the third line, containing the input alphabet */
  fgets(buffer, BUFFER_SIZE, in);
  input.alphabet = malloc(input.nSymbols+1);
  tok = strtok(buffer, " ");
  for (i = 0; i < input.nSymbols; i++) {
    input.alphabet[i] = tok[0];
    tok = strtok(NULL, " ");
  }
  input.alphabet[input.nSymbols] = 0;


  /* Reads in the third line, containing the tape alphabet */
  fgets(buffer, BUFFER_SIZE, in);
  input.tapeSymbols = malloc(input.nTapeSymbols+1);
  tok = strtok(buffer, " ");
  for (i = 0; i < input.nTapeSymbols-1; i++) {
    input.tapeSymbols[i] = tok[0];
    tok = strtok(NULL, " ");
  }
  input.tapeSymbols[input.nTapeSymbols-1] = IO_START ; /* Special "Start of Input/Output" symbol */
  input.tapeSymbols[input.nTapeSymbols] = 0;
  /* 4 + 2z extra quintuples. 3 + 2z from the output standardizing, 1 extra required as Af from the Bennett paper */
  input.transitions = calloc(sizeof(*input.transitions), input.nTransitions + 4 + (2 * input.nTapeSymbols));
  j = 1;
  /* Parse each quintuple */
  for (i = 0; i < input.nTransitions; i++) {
    fgets(buffer, BUFFER_SIZE, in);
    if (parse5ple(buffer, &quint)) {
      fprintf(stderr, "%s\n", buffer);
      fatalError("Malformed Quintuple", 5);
    }
    validEntry  = (strchr(input.tapeSymbols, quint.entry) != NULL);  /* Checks the that read symbol is in the tape alphabet  */
    validOutput = (strchr(input.tapeSymbols, quint.output) != NULL); /* Checks the that write symbol is in the tape alphabet */ 
    if (!validEntry || !validOutput) {
      fprintf(stderr, "%s\n", buffer);
      if (!validEntry) 
        fprintf(stderr, "%c character not in tape symbol list. ", quint.entry);
      if (!validOutput) 
        fprintf(stderr, "%c character not in tape symbol list. ", quint.output);
      fatalError("Malformed Quintuple", 5);
    }
    quint.m = j + 1;
    input.transitions[j++] = quint;
  }
  /* Reads the final input line */
  fgets(input.entry, BUFFER_SIZE, in);
  input.entry[strlen(input.entry)-1] = 0; /* Trim newline from fgets - breaks if line is longer than 1023 characters though */
  for (i = 0; i < strlen(input.entry); i++) {
    if (!strchr(input.alphabet, input.entry[i])) {
      fatalError("Invalid Input String", 6);
    }
  }

  /* Writes the Start character to the start of the tape. Note that this is being combined with the 1st special quintuple from the paper
   * (A1b -> b + A2).  */
  quint.entryState = input.states[0];
  quint.outState = input.states[1];
  quint.entry = BLANK;
  quint.output = IO_START;
  quint.shift = +1;
  quint.m = 1;
  input.transitions[0] = quint;

  /* From the diagram in the repo: Sf * -> * - A1 */
  quint.entryState = Sf;
  for (i = 0; i < input.nTapeSymbols - 1; i++) { 
    if (input.tapeSymbols[i] != 'B') {
      quint.outState = input.states[input.nStates-3];
      quint.shift = -1;
      quint.entry = input.tapeSymbols[i];
      quint.output = input.tapeSymbols[i];
    } else {
      quint.outState = input.states[input.nStates-3];
      quint.shift = -1;
      quint.entry = BLANK;
      quint.output = BLANK;
    }
    quint.m = j + 1;
    input.transitions[j++] = quint;
  }

  /* From the diagram in the repo: A1 * -> * - A1 */
  quint.entryState = input.states[input.nStates-3];
  for (i = 0; i < input.nTapeSymbols - 1; i++) { 
    if (input.tapeSymbols[i] != 'B') {
      quint.outState = input.states[input.nStates-3];
      quint.shift = -1;
      quint.entry = input.tapeSymbols[i];
      quint.output = input.tapeSymbols[i];
    } else {
      quint.outState = input.states[input.nStates-3];
      quint.shift = -1;
      quint.entry = BLANK;
      quint.output = BLANK;
    }
    quint.m = j + 1;
    input.transitions[j++] = quint;
  }

  /* A1 $ -> B S A2 */
  quint.entryState = input.states[input.nStates-3];
  quint.outState = input.states[input.nStates-2];
  quint.shift = 0;
  quint.entry = IO_START;
  quint.output = BLANK;
  quint.m = j + 1;
  input.transitions[j++] = quint;

  /* S1 $ -> B S A2 */
  quint.entryState = Sf;
  quint.outState = input.states[input.nStates-2];
  quint.shift = 0;
  quint.entry = IO_START;
  quint.output = BLANK;
  quint.m = j + 1;
  input.transitions[j++] = quint;

  /* A2 B -> B 0 Af */
  quint.entryState = input.states[input.nStates-2];
  quint.outState = input.states[input.nStates-1];
  quint.entry = BLANK;
  quint.output = BLANK;
  quint.shift = 0;
  quint.m = j + 1;
  input.transitions[j++] = quint;

  input.nTransitions = j;
  return input;
}

TuringMachine
makeReversible(struct Input input) {
  #define PUSH_4PLE(e0, e1, e2, o0, o1, o2, es, os) { \
  quadruples[nextQuadruple].entry[0] = e0; \
  quadruples[nextQuadruple].entry[1] = e1; \
  quadruples[nextQuadruple].entry[2] = e2; \
  quadruples[nextQuadruple].output[0] = o0; \
  quadruples[nextQuadruple].output[1] = o1; \
  quadruples[nextQuadruple].output[2] = o2; \
  quadruples[nextQuadruple].entryState = es; \
  quadruples[nextQuadruple].outState= os; \
  nextQuadruple++; \
  }


  const int N = input.nTransitions;
  const int Z = input.nTapeSymbols;
  const State Af = input.states[input.nStates-1];
  const State b1p = { Bp, 1 };
  const State b1 = { B, 1 };
  const State b2p = { Bp, 2 };
  const State b2 = { B, 2 };
  State cf = { C };
  Quadruple* quadruples;
  TuringMachine tm = {0};
  int nextQuadruple = 0;
  int i;
  char x;
  cf.num = Af.num;
  quadruples = malloc(sizeof(*quadruples) * (4 * N + 2 * Z + 3));
  /* Create Stage 1 (Compute) quadruples */
  for (i = 0; i < N; i++) {
    convert5pleToReversible4ple(input.transitions[i], quadruples + (i * 2));
  }
  nextQuadruple = N * 2;

  /* Create Stage 2 (Copy output) quadruples */
  PUSH_4PLE(BLANK, N, BLANK, BLANK, N, BLANK, Af, b1p); /* Af[b N b] -> [b N b]B1' */

  PUSH_4PLE(NO_READ, NO_READ, NO_READ, SHIFT_RIGHT, SHIFT_STAY, SHIFT_RIGHT, b1p,  b1);  /* B1'[/ / /] -> [+ 0 +]B1 */
  for (i = 0; i < Z; i++) {
    x = input.tapeSymbols[i];
    /* x != b : { B1[x N b] -> [x N x]B1' } */
    if (x != 'B') {
      PUSH_4PLE(x, N, BLANK, x, N, x, b1, b1p);
    }
  }
  PUSH_4PLE(BLANK, N, BLANK, BLANK, N, BLANK, b1, b2p); /* B1[ b N b] -> [b N b]B2' */
  PUSH_4PLE(NO_READ, NO_READ, NO_READ, SHIFT_LEFT, SHIFT_STAY, SHIFT_LEFT, b2p, b2); /* B2'[/ / /] -> [- 0 -]B2 */
  for (i = 0; i < Z; i++) {
    x = input.tapeSymbols[i];
    /* x != b : { B2[x N x] -> [x N x]B2' } */
    if (x != 'B') {
      PUSH_4PLE(x, N, x, x, N, x, b2, b2p);
    }
  }
  PUSH_4PLE(BLANK, N, BLANK, BLANK, N, BLANK, b2, cf); /* B2[b N b] -> [b N b]Cf */

  /* Create Stage 3 (Retrace) quadruples */
  for (i = N; i > 0; i--) {
    convert5pleToInverse4ple(input.transitions[i-1], quadruples + nextQuadruple);
    nextQuadruple += 2;
  }
  tm.state.stage = A; tm.state.num = 0;
  tm.acceptState.stage = C; tm.acceptState.num = 0;
  if (!initTuringMachine(&tm, (4 * N + 2 * Z + 3), quadruples)) {
    fatalError("Invalid Turing Machine", 7);
  }
  return tm;
}

void
checkForFlags(int argn, const char* argv[]) {
  int i;
  for (i = 0; i < argn; i++) {
    if (argv[i][0] == 'p') runtimeFlags |= PRINT;
    if (argv[i][0] == 'g') runtimeFlags |= GRAPHICS;
  }
}

int
main(int argn, const char *argv[]) {
  struct Input input = {0};
  TuringMachine tm =  {0};
  checkForFlags(argn, argv);
  input = readInput(stdin);
  if (!input.valid) {
    printf("Invalid input\n");
    return 4;
  }
  tm = makeReversible(input);
  if (tm.quadruplesSize == 0) {
    return 5;
  }
  memcpy(tm.tapes[0].tape+1, input.entry, strlen(input.entry));
  if (runtimeFlags & GRAPHICS) {
    if (turingMachineGoGFX(&tm)) return 0;
    return 9;
  }
  if (turingMachineGo(&tm)) return 0;
  return 9;
}

