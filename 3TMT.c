#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#define PRINT_TAPES*/
#define REVERSIBLE

typedef char boolean;
#define true (1 == 1)
#define false (!true)

#ifdef REVERSIBLE
#undef NUM_TAPES
#define NUM_TAPES 3
#else
#ifndef NUM_TAPES
#define NUM_TAPES 1
#endif /* NUM_TAPES */
#endif /* REVERSIBLE */

#define NO_READ '/'
#define SHIFT_LEFT '-'
#define SHIFT_STAY '0'
#define SHIFT_RIGHT '+'
#define BLANK '\0'

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

typedef struct Quintuple {
  char entry[NUM_TAPES];
  char output[NUM_TAPES];
  int shift[NUM_TAPES];
  int entryState;
  int outState;
} Quintuple;


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
    printf("%c ", quad.entry[i]);
  }
  printf("]) -> (%d [", quad.outState);
  for (i = 0; i < NUM_TAPES; i++) {
    printf("%c ", quad.output[i]);
  }
  printf("])\n");
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
          print4ple(tm.quadruples[i]);
          print4ple(tm.quadruples[j]);
        }
        if (!overlapsInRange) {
          printf("Quadruples %d and %d overlap in range\n", i, j);
          print4ple(tm.quadruples[i]);
          print4ple(tm.quadruples[j]);
        }
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

#ifdef REVERSIBLE
int 
convert5pleToReversible4ple(Quintuple quint, Quadruple quads[2], int quintIndex, int nextState) {
  ++nextState;
  quads[0].entryState = quint.entryState;
  quads[0].entry[0] = quint.entry[0];
  quads[0].entry[1] = NO_READ;
  quads[0].entry[2] = BLANK;
  quads[0].output[0] = quint.output[0];
  quads[0].output[1] = SHIFT_RIGHT;
  quads[0].output[2] = BLANK;
  quads[0].outState = nextState;

  quads[1].entryState = nextState;
  quads[1].entry[0] = NO_READ;
  quads[1].entry[1] = BLANK;
  quads[1].entry[2] = NO_READ;
  if (quint.shift[0] < 0) quads[1].output[0] = SHIFT_LEFT;
  else if (quint.shift[0] > 0) quads[1].output[0] = SHIFT_RIGHT;
  else quads[1].output[0] = SHIFT_STAY;
  quads[1].output[1] = quintIndex;
  quads[1].output[2] = SHIFT_STAY;
  quads[1].outState = nextState;
  return nextState;
}
#endif


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
      printf("Tape %2d: [", i);
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

void
readInput(FILE* in) {

}

int
main(int argn, char *argv[]) {
  int i;
  Quintuple quintuples[13] = {
    {"a", "a", {+1}, 1, 1},
    {"b", "b", {+1}, 1, 1},
    {"#", "#", {+1}, 1, 1},
    {"\0", "\0", {-1}, 1, 2},
    {"a", "\0", {-1}, 2, 3},
    {"b", "\0", {-1}, 2, 4},
    {"#", "\0", {-1}, 2, 100},
    {"a", "a", {-1}, 3, 3},
    {"#", "a", {-1}, 3, 100},
    {"b", "a", {-1}, 3, 4},
    {"a", "b", {-1}, 4, 3},
    {"#", "b", {-1}, 4, 100},
    {"b", "b", {-1}, 4, 4},
  };
  Quadruple quadruples[26];
  TuringMachine tm = {0};
  int maxState = 0;
  char entry[] = "abba#baab";
  for (i = 0; i < 13; i++) {
    maxState = quintuples[i].outState > maxState ? quintuples[i].outState : maxState;
  }
  for (i = 0; i < 13; i++) {
    maxState = convert5pleTo4ple(quintuples[i], quadruples + (i * 2), maxState);
  }
  if (!initTuringMachine(&tm, sizeof(quadruples)/sizeof(*quadruples), quadruples)) {
    printf("Invalid turing machine\n");
    /*return 1;*/
  }
  memcpy(tm.tapes[0].tape, entry, sizeof(entry));
  tm.state = 1;
  tm.acceptState = 100;
  if (turingMachineGo(&tm)) {
    printf("Accept\n");
  } else {
    printf("Reject\n");
  }
  return 0;
}
