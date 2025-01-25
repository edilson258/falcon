#include <assert.h>
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "falcon/path.h"

#define TODO(message)                                              \
  fprintf(stderr, "TODO %s:%d %s\n", __FILE__, __LINE__, message); \
  abort();

typedef struct
{
  size_t cursor;
  char *content;
  size_t content_len;
} lexer;

void lexer_init(lexer *l, char *content)
{
  l->cursor = 0;
  l->content = content;
  l->content_len = strnlen(content, PATH_MAX_LEN);
}

typedef enum
{
  TOKEN_EOF = 0,
  TOKEN_SLASH = 1,
  TOKEN_COLON = 2,
  TOKEN_IDENT = 3,
} token_t;

typedef struct
{
  token_t typ;
  char *lexeme;
} token;

int lexer_is_eof(lexer *l)
{
  return l->cursor >= l->content_len;
}

char lexer_peek_one(lexer *l)
{
  return lexer_is_eof(l) ? 0 : l->content[l->cursor];
}

void lexer_advance_one(lexer *l)
{
  if (lexer_is_eof(l))
  {
    return;
  }
  l->cursor++;
}

void lexer_skip_whitespace(lexer *l)
{
  while (!lexer_is_eof(l) && isspace(lexer_peek_one(l)))
  {
    lexer_advance_one(l);
  }
}

typedef int (*lexer_read_predicate)(int);

int read_ident_predicate(int c)
{
  return isalnum(c);
}

void lexer_read_while(lexer *l, lexer_read_predicate pred, char **out)
{
  size_t start = l->cursor;
  while (!lexer_is_eof(l) && pred(lexer_peek_one(l)))
  {
    lexer_advance_one(l);
  }
  size_t buf_len = l->cursor - start;
  *out = (char *)malloc(sizeof(char) * (buf_len + 1));
  strncpy(*out, l->content + start, buf_len);
  (*out)[buf_len] = 0;
}

token lexer_next_token(lexer *l)
{
  lexer_skip_whitespace(l);

  token token;
  if (lexer_is_eof(l))
  {
    token.typ = TOKEN_EOF;
  }
  else if (lexer_peek_one(l) == '/')
  {
    token.typ = TOKEN_SLASH;
    lexer_advance_one(l);
  }
  else if (lexer_peek_one(l) == ':')
  {
    token.typ = TOKEN_COLON;
    lexer_advance_one(l);
  }
  else if (isalpha(lexer_peek_one(l)))
  {
    char *out;
    lexer_read_while(l, read_ident_predicate, &out);
    token.typ = TOKEN_IDENT;
    token.lexeme = out;
  }
  else
  {
    assert(!"Invalid token");
  }

  return token;
}

int parse_path(char *raw, fpath_t *path)
{
  lexer l;
  lexer_init(&l, raw);

  token t = lexer_next_token(&l);
  while (t.typ)
  {
    switch (t.typ)
    {
    case TOKEN_EOF:
      printf("EOF\n");
      break;
    case TOKEN_COLON:
      printf(":\n");
      break;
    case TOKEN_SLASH:
      printf("/\n");
      break;
    case TOKEN_IDENT:
      printf("%s\n", t.lexeme);
      break;
    }
    t = lexer_next_token(&l);
  }

  return 1;
}
