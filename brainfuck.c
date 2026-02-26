#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// dont change cuz we rely on wrap around of tape_idx(uint16_t)
#define TAPE_SIZE 65536

char *get_ops(FILE *f, size_t *ops_len) {
  if (!f || !ops_len) {
    return NULL;
  }

  size_t capacity = 64;
  size_t idx = 0;

  char *ops = malloc(capacity);
  if (!ops) {
    perror("malloc");
    return NULL;
  }

  for (int ch = fgetc(f); ch != EOF; ch = fgetc(f)) {
    switch (ch) {
    case '>':
    case '<':
    case '+':
    case '-':
    case '.':
    case ',':
    case '[':
    case ']': {
      if (idx + 1 >= capacity) {
        capacity *= 2;
        char *tmp = realloc(ops, capacity);
        if (!tmp) {
          free(ops);
          perror("realloc");
          return NULL;
        }
        ops = tmp;
      }
      ops[idx++] = (char)ch;
      break;
    }
    default:
      break;
    }
  }

  ops[idx] = '\0';
  *ops_len = idx;
  return ops;
}

int64_t *calculate_offsets(const char *code, size_t len) {

  int64_t *offsets = calloc(len, sizeof(int64_t));
  if (!offsets) {
    perror("calloc");
    return NULL;
  }

  size_t stack[len];
  size_t stack_top = 0;

  for (size_t i = 0; i < len; i++) {

    if (code[i] == '[') {
      stack[stack_top++] = i;
    } else if (code[i] == ']') {
      if (stack_top == 0) {
        fprintf(stderr, "Error: unmatched ']'\n");
        free(offsets);
        return NULL;
      }

      size_t open = stack[--stack_top];
      offsets[open] = i;
      offsets[i] = open;

    } else if (code[i] == '>' || code[i] == '<' || code[i] == '+' ||
               code[i] == '-') {

      char op = code[i];
      int64_t count = 0;
      size_t j;

      for (j = i; j < len && code[j] == op; j++) {
        count++;
      }

      offsets[i] = count;
      i = j - 1;
    }
  }

  if (stack_top != 0) {
    fprintf(stderr, "Error: unmatched '['\n");
    free(offsets);
    return NULL;
  }

  return offsets;
}

int execute_code(char *code, size_t len) {
  uint8_t *tape = calloc(TAPE_SIZE, sizeof(uint8_t));
  if (!tape) {
    perror("calloc");
    return 1;
  }

  int64_t *offsets = calculate_offsets(code, len);
  if (!offsets) {
    free(tape);
    return 1;
  }

  uint16_t tape_idx = 0;

  for (size_t code_idx = 0; code_idx < len;) {

    switch (code[code_idx]) {

    case '>': {
      int64_t count = offsets[code_idx];
      tape_idx += (uint16_t)count;
      code_idx += count;
      break;
    }

    case '<': {
      int64_t count = offsets[code_idx];
      tape_idx -= (uint16_t)count;
      code_idx += count;
      break;
    }

    case '+': {
      int64_t count = offsets[code_idx];
      tape[tape_idx] += (uint8_t)count;
      code_idx += count;
      break;
    }

    case '-': {
      int64_t count = offsets[code_idx];
      tape[tape_idx] -= (uint8_t)count;
      code_idx += count;
      break;
    }

    case '.': {
      putchar(tape[tape_idx]);
      code_idx++;
      break;
    }

    case ',': {
      int input = getchar();
      tape[tape_idx] = (input == EOF) ? 0 : (uint8_t)input;
      code_idx++;
      break;
    }

    case '[': {
      if (tape[tape_idx] == 0) {
        code_idx = offsets[code_idx] + 1;
      } else {
        code_idx++;
      }
      break;
    }

    case ']': {
      if (tape[tape_idx] != 0) {
        code_idx = offsets[code_idx] + 1;
      } else {
        code_idx++;
      }
      break;
    }

    default:
      code_idx++;
      break;
    }
  }

  free(offsets);
  free(tape);
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s /path/to/file\n", argv[0]);
    return EXIT_FAILURE;
  }

  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror("fopen");
    return EXIT_FAILURE;
  }

  size_t code_len = 0;
  char *code = get_ops(f, &code_len);
  fclose(f);

  if (!code) {
    return EXIT_FAILURE;
  }

  int res = execute_code(code, code_len);

  free(code);
  return res;
}
