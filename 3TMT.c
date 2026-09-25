#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_TAPES
#define NO_READ '/'
#define SHIFT_LEFT '-'
#define SHIFT_STAY '.'
#define SHIFT_RIGHT '+'
#define BLANK '\0'

typedef char boolean;
#define true (1 == 1)
#define false (!true)

#define TAPES \
  X(INPUT) \
  X(HISTORY) \
  X(OUTPUT) 

enum TapeType {
#define X(name) name,
  TAPES
#undef X
  NUM_TAPES
};

const char* tapeNames[NUM_TAPES] = {
#define X(name) #name,
  TAPES
#undef X
};

typedef struct State {
  enum {
    A,
    Ap,
    B,
    Bp,
    C,
    Cp,
    F,
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

typedef struct Symbol {
  enum {
    SHIFT,
    NUMBER,
    SYMBOL
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
} Symbol;

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
} TuringMachine;

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
      if (quad.entry[i] == SHIFT_LEFT) return false;
      if (quad.entry[i] == SHIFT_STAY) return false;
      if (quad.entry[i] == SHIFT_RIGHT) return false;
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
  if (tape->head >= 0)
    tape->tape[tape->head] = c;
}

boolean
tapeRead(Tape* tape, char expected) {
  if (expected == NO_READ) return true;
  if (tape->head < 0) return BLANK == expected;
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

/*
int
convert5pleTo4ple(Quintuple quint, Quadruple quads[2], int nextState) {
  int i;
  ++nextState;
  quads[0].entryState = quint.entryState;
  memcpy(quads[0].entry, quint.entry, sizeof(*quint.entry) * NUM_TAPES);
  memcpy(quads[0].output, quint.output, sizeof(*quint.output) * NUM_TAPES);
  quads[0].outState = nextState;

  quads[1].entryState = nextState;
  memset(quads[1].entry, NO_READ, sizeof(*quads[1].entry) * NUM_TAPES);
  for (i = 0; i < NUM_TAPES; i++) {
    if (quint.shift[i] < 0) quads[1].output[i] = SHIFT_LEFT;
    else if (quint.shift[i] > 0) quads[1].output[i] = SHIFT_RIGHT;
    else quads[1].output[i] = SHIFT_STAY;
  }
  quads[1].outState = quint.outState;
  return nextState;
}
*/

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

int
turingMachineGo(TuringMachine* tm) {
  boolean reject = false;
  int i;
#ifdef PRINT_TAPES
  int j;
  char c;
#endif
  while (true) {
#ifdef PRINT_TAPES
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
#endif
    reject = true;
    for (i = 0; i < tm->quadruplesSize; i++) {
      if (shouldRun4ple(tm, tm->quadruples[i])) {
        apply4ple(tm, tm->quadruples+i);
        reject = false;
        break;
      }
    }
    if (statesEqual(tm->state, tm->acceptState)) {
#ifdef PRINT_TAPES
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
#endif
      return 1;
    }
    if (reject) return 0;
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

struct Input
readInput(FILE* in) {
  #define BUFFER_SIZE 1023
  struct Input input;
  int i, j;
  char throwAway;
  char buffer[BUFFER_SIZE+1];
  char* tok;
  State Sf;
  boolean validEntry = false, validOutput = false;
  Quintuple quint;
  input.valid = true;
  fgets(buffer, BUFFER_SIZE, in);
  sscanf(buffer, "%d %d %d %d\n", &input.nStates, &input.nSymbols, &input.nTapeSymbols, &input.nTransitions);
  input.nTapeSymbols += 1;
  input.nStates += 4;
  input.states = malloc(sizeof(*input.states) * input.nStates);
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


  fgets(buffer, BUFFER_SIZE, in);
  input.alphabet = malloc(input.nSymbols+1);
  tok = strtok(buffer, " ");
  for (i = 0; i < input.nSymbols; i++) {
    input.alphabet[i] = tok[0];
    tok = strtok(NULL, " ");
  }
  input.alphabet[input.nSymbols] = 0;

  fgets(buffer, BUFFER_SIZE, in);
  input.tapeSymbols = malloc(input.nTapeSymbols+1);
  tok = strtok(buffer, " ");
  for (i = 0; i < input.nTapeSymbols-1; i++) {
    input.tapeSymbols[i] = tok[0];
    tok = strtok(NULL, " ");
  }
  input.tapeSymbols[input.nTapeSymbols-1] = 0xff; /* Special "Start of Input/Output" symbol */
  input.tapeSymbols[input.nTapeSymbols] = 0;
  /* 4 + 2z extra quintuples. 3 + 2z from the output standardizing, 1 extra required as Af from the Bennett paper */
  input.transitions = calloc(sizeof(*input.transitions), input.nTransitions + 4 + (2 * input.nTapeSymbols));
  j = 1;
  for (i = 0; i < input.nTransitions; i++) {
    fscanf(in, "(%d,%c)=(%d,%c,%c)\n", &quint.entryState.num, &quint.entry,  &quint.outState.num, &quint.output, &throwAway);
    switch (throwAway) {
    case 'L': quint.shift = -1; break;
    case 'R': quint.shift = +1; break;
    default:  quint.shift = 0; break;
    }
    if (quint.entry == 'B') quint.entry = BLANK;
    if (quint.output == 'B') quint.output = BLANK;
    quint.entryState.stage = quint.outState.stage = A;
    validEntry             = (strchr(input.tapeSymbols, quint.entry) != NULL);  /* Checks the that read symbol is in the tape alphabet  */
    validOutput            = (strchr(input.tapeSymbols, quint.output) != NULL); /* Checks the that write symbol is in the tape alphabet */ 
    if (!validEntry || !validOutput) {
      print5ple(quint);
      printf("5ple transition invalid: ");
      if (!validEntry) 
        printf("%c character not in tape symbol list. ", quint.entry);
      if (!validOutput) 
        printf("%c character not in tape symbol list. ", quint.output);
      printf("\n");
      input.valid = false;
    }
    quint.m = j + 1;
    input.transitions[j++] = quint;
  }
  fgets(input.entry, BUFFER_SIZE, in);
  input.entry[strlen(input.entry)-1] = 0; /* Trim newline from fgets - breaks if line is longer than 1023 characters though */
  for (i = 0; i < strlen(input.entry); i++) {
    if (!strchr(input.alphabet, input.entry[i])) {
      printf("Character %c of entry does not belong to the tape alphabet.\n", input.entry[i]);
      input.valid = false;
    }
  }

  /* Writes the Start character to the start of the tape. Note that this is being combined with the 1st special quintuple from the paper
   * (A1b -> b + A2).  */
  quint.entryState = input.states[0];
  quint.outState = input.states[1];
  quint.entry = BLANK;
  quint.output = 0xff;
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
  quint.entry = 0xff;
  quint.output = BLANK;
  quint.m = j + 1;
  input.transitions[j++] = quint;

  /* S1 $ -> B S A2 */
  quint.entryState = Sf;
  quint.outState = input.states[input.nStates-2];
  quint.shift = 0;
  quint.entry = 0xff;
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

  printf("Total Transitions: %d. Expected Transitions: %d\n", j, input.nTransitions + (input.nTapeSymbols - 1) * 2 + 4);

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
  for (i = 0; i < (4 * N + 2 * Z + 3); i++) {
    print4ple(quadruples[i]);
  }
  if (!initTuringMachine(&tm, (4 * N + 2 * Z + 3), quadruples)) {
    printf("Invalid turing machine\n");
    free(quadruples);
    tm.quadruplesSize = 0;
  }
  return tm;
}

int
main(int argn, char *argv[]) {
  struct Input input = {0};
  TuringMachine tm =  {0};
  input = readInput(stdin);
  if (!input.valid) {
    printf("Invalid input\n");
    return 1;
  }
  tm = makeReversible(input);
  if (tm.quadruplesSize == 0) {
    return 1;
  }
  memcpy(tm.tapes[0].tape+1, input.entry, strlen(input.entry));
  if (turingMachineGo(&tm)) {
    printf("Accept\n");
  } else {
    printf("Reject\n");
  }
  return 0;
}
