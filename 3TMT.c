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

#define TAPE_NAMES \
  X(INPUT) \
  X(HISTORY) \
  X(OUTPUT) 

enum TapeType {
#define X(name) name,
  TAPE_NAMES
#undef X
  NUM_TAPES
};

const char* tapeNames[NUM_TAPES] = {
#define X(name) #name,
  TAPE_NAMES
#undef X
};

typedef struct Quintuple {
  char entry;
  char output;
  int shift;
  int entryState;
  int outState;
} Quintuple;

typedef struct Tape {
  char* tape;
  int head;
} Tape;

typedef struct Quadruple {
  char entry[NUM_TAPES];
  char output[NUM_TAPES];
  int entryState;
  int outState;
} Quadruple;


typedef struct TuringMachine {
  Tape tapes[NUM_TAPES];
  Quadruple* quadruples;
  int quadruplesSize;
  int tapeSize;
  int state;
  int acceptState;
} TuringMachine;

void
print4ple(Quadruple quad) {
  int i;
  printf("(%d, [", quad.entryState);
  for (i = 0; i < NUM_TAPES; i++) {
    printf("%d ", quad.entry[i]);
  }
  printf("]) -> (%d [", quad.outState);
  for (i = 0; i < NUM_TAPES; i++) {
    printf("%d ", quad.output[i]);
  }
  printf("])\n");
}

void
print5ple(Quintuple quint) {
  printf("(%d %c) -> (%d %c %d)\n", quint.entryState, quint.entry, quint.outState, quint.output, quint.shift);
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
validate4pleDomainOverlap(Quadruple a, Quadruple b) {
  int i;
  if (a.entryState != b.entryState) return true;
  for (i = 0; i < NUM_TAPES; i++) {
    if ((a.entry[i] != b.entry[i]) && (a.entry[i] != NO_READ) && (b.entry[i] != NO_READ)) return true;
  }
  return false;
}

boolean
validate4pleRangeOverlap(Quadruple a, Quadruple b) {
  int i;
  if (a.outState != b.outState) return true;
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
  tape->tape[tape->head] = c;
}

boolean
tapeRead(Tape* tape, char expected) {
  if (expected == NO_READ) return true;
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
  if (tm->state != quad.entryState) return false;
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

int 
convert5pleToReversible4ple(Quintuple quint, Quadruple quads[2], int quintIndex, int nextState) {
  ++nextState;
  quads[0].entryState = quint.entryState;
  quads[0].entry[INPUT] = quint.entry;
  quads[0].entry[HISTORY] = NO_READ;
  quads[0].entry[OUTPUT] = BLANK;
  quads[0].output[INPUT] = quint.output;
  quads[0].output[HISTORY] = SHIFT_RIGHT;
  quads[0].output[OUTPUT] = BLANK;
  quads[0].outState = nextState;

  quads[1].entryState = nextState;
  quads[1].entry[INPUT] = NO_READ;
  quads[1].entry[HISTORY] = BLANK;
  quads[1].entry[OUTPUT] = NO_READ;
  if (quint.shift < 0)      quads[1].output[INPUT] = SHIFT_LEFT;
  else if (quint.shift > 0) quads[1].output[INPUT] = SHIFT_RIGHT;
  else                      quads[1].output[INPUT] = SHIFT_STAY;
  quads[1].output[HISTORY] = quintIndex;
  quads[1].output[OUTPUT] = SHIFT_STAY;
  quads[1].outState = quint.outState;
  return nextState;
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
    reject = true;
    for (i = 0; i < tm->quadruplesSize; i++) {
      if (shouldRun4ple(tm, tm->quadruples[i])) {
        apply4ple(tm, tm->quadruples+i);
        reject = false;
        break;
      }
    }
#ifdef PRINT_TAPES
    for (i = 0; i < NUM_TAPES; i++) {
      printf("%s: [", tapeNames[i]);
      j = 0;
      while ((c = tm->tapes[i].tape[j])) {
        if (j == tm->tapes[i].head) {
          printf("(q%d)", tm->state);
        }
        printf("%c", c);
        j++;
      }
      printf("]\n");
    }
#endif
    if (tm->state == tm->acceptState) return 1;
    if (reject) return 0;
  }
  return -1; 
}


struct Input {
  Quintuple* transitions;
  int* states;
  char* alphabet;
  char* tapeSymbols;
  int nStates;
  int nTransitions;
  int nSymbols;
  int nTapeSymbols;
  char entry[1024];
};

struct Input
readInput(FILE* in) {
  #define BUFFER_SIZE 1023
  struct Input input;
  int i;
  char throwAway;
  char buffer[BUFFER_SIZE+1];
  char* tok;
  boolean validEntry = false, validOutput = false;
  Quintuple quint;
  fgets(buffer, BUFFER_SIZE, in);
  sscanf(buffer, "%d %d %d %d\n", &input.nStates, &input.nSymbols, &input.nTapeSymbols, &input.nTransitions);
  input.states = malloc(sizeof(*input.states) * input.nStates);
  fgets(buffer, BUFFER_SIZE, in);
  tok = strtok(buffer, " ");
  for (i = 0; i < input.nStates; i++) {
    input.states[i] = atoi(tok);
    tok = strtok(NULL, " ");
  }

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
  for (i = 0; i < input.nTapeSymbols; i++) {
    input.tapeSymbols[i] = tok[0];
    tok = strtok(NULL, " ");
  }
  input.tapeSymbols[input.nTapeSymbols] = 0;
  input.transitions = calloc(sizeof(*input.transitions), input.nTransitions);
  for (i = 0; i < input.nTransitions; i++) {
    fscanf(in, "(%d,%c)=(%d,%c,%c)\n", &quint.entryState, &quint.entry,  &quint.outState, &quint.output, &throwAway);
    switch (throwAway) {
    case 'L': quint.shift = -1; break;
    case 'R': quint.shift = +1; break;
    default:  break;
    }
    validEntry      = (strchr(input.tapeSymbols, quint.entry) != NULL);
    validOutput     = (strchr(input.tapeSymbols, quint.output) != NULL);
    if (!validEntry || !validOutput) {
      print5ple(quint);
      printf("5ple transition invalid: ");
      if (!validEntry) 
        printf("%c character not in transition alphabet. ", quint.entry);
      if (!validOutput) 
        printf("%c character not in transition alphabet. ", quint.output);
      printf("\n");
    }
    input.transitions[i] = quint;
  }
  fgets(input.entry, BUFFER_SIZE, in);
  input.entry[strlen(input.entry)-1] = 0;
  for (i = 0; i < strlen(input.entry); i++) {
    if (!strchr(input.alphabet, input.entry[i])) {
      printf("Character %c of entry does not belong to the tape alphabet.\n", input.entry[i]);
    }
  }
  return input;
}

int
main(int argn, char *argv[]) {
  int i;
  struct Input input;
  Quadruple* quadruples;
  int nextState;
  TuringMachine tm = {0};
  input = readInput(stdin);
  quadruples = malloc(sizeof(*quadruples) * input.nTransitions * 2);
  nextState = input.states[input.nStates-1];
  for (i = 0; i < input.nTransitions; i++) {
    nextState = convert5pleToReversible4ple(input.transitions[i], quadruples + (i * 2), i, nextState);
  }
  if (!initTuringMachine(&tm, input.nTransitions * 2, quadruples)) {
    printf("Invalid turing machine\n");
    return 1;
  }
  memcpy(tm.tapes[0].tape, input.entry, strlen(input.entry));
  tm.state = input.states[0];
  tm.acceptState = input.states[input.nStates-1];
  if (turingMachineGo(&tm)) {
    printf("Accept\n");
  } else {
    printf("Reject\n");
  }
  return 0;
}
