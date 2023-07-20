#include <stdlib.h>
#include <stdio.h>

#include "common.c"
#include "tokenizer.c"

typedef struct {
  const char *string;
  size_t len;
} Span;

static Span inputFromFile(int argc, char **argv) {
  StringView result = {0};
  if (argc < 2) {
    printf("Error: no input file.\n");
    exit(EXIT_FAILURE);
  }
  FILE *input_file = NULL;
  errno_t error = fopen_s(&input_file, argv[1], "rb");
  if (error) {
    perror("input file cannot be opened.");
    exit(EXIT_FAILURE);
  }

  if (fseek(input_file, 0, SEEK_END)) {
    printf("Error: cannot determine input file size\n");
    exit(EXIT_FAILURE);
  }
  const size_t n_bytes = ftell(input_file);

  char *input_buffer = (char *)Calloc(1, n_bytes);

  if (!input_buffer) {
    printf("Error: no enough memory to read an input file\n");
    exit(EXIT_FAILURE);
  }

  size_t n_read = fread(input_buffer, n_bytes, n_bytes, input_file);
  if (n_read != n_bytes) {
    perror("cannot read an input file");
    exit(EXIT_FAILURE);
  }
  fclose(input_file);
  return (Span){.string = input_buffer, .len = n_read};
}

#define STRING_VIEW(literal) \
  { .string = literal, .len = sizeof(literal) - 1 }

int main(int argc, char **argv) {
  // StringView input = inputFromFile(argc, argv);
  Span input = STRING_VIEW("asd = 123 + 0xfep12");
  InitTokenizer(input.string, input.len);

  const Token *token = NextToken();
  while (token->type != TOKEN_END_OF_STREAM) {
    if (token->type == TOKEN_INVALID) {
      break;
    }
    token = NextToken();
  }

  return 0;
}
