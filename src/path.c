#include <assert.h>
#include <ctype.h>
#include <falcon/path.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  const char *content;
  size_t content_len;
  size_t cursor;
} parser_t;

parser_t *parser_new(const char *content)
{
  parser_t *parser = malloc(sizeof(parser_t));
  parser->content = content;
  parser->content_len = strnlen(content, PATH_MAX_LEN);
  parser->cursor = 0;
  return parser;
}

int parser_is_eof(parser_t *parser)
{
  return parser->cursor >= parser->content_len;
}

#define PARSER_EOF '\0'

char parser_peek(parser_t *parser)
{
  return parser_is_eof(parser) ? PARSER_EOF : parser->content[parser->cursor];
}

void parser_advance(parser_t *parser)
{
  parser->cursor++;
}

char parser_advance_if(parser_t *parser, char c)
{
  if (parser_peek(parser) == c)
  {
    parser_advance(parser);
    return 1;
  }
  return 0;
}

void parser_skip_whitespace(parser_t *parser)
{
  while (!parser_is_eof(parser) && isspace(parser_peek(parser)))
  {
    parser_advance(parser);
  }
}

typedef int (*parser_predicate)(int);

void parser_advance_while(parser_t *parser, parser_predicate p)
{
  while (!parser_is_eof(parser) && p(parser_peek(parser)))
  {
    parser_advance(parser);
  }
}

int parse_parameter_frag(parser_t *parser, frag_t *frag)
{
  return 1;
}

frag_t fragment_new(int is_parameter)
{
  frag_t frag;
  frag.is_parameter = is_parameter;
  memset(frag.label, 0, sizeof(frag.label));
  return frag;
}

fpath_t *parse_path(const char *content)
{
  fpath_t *path = malloc(sizeof(fpath_t));
  path->nfrags = 0;

  parser_t *parser = parser_new(content);

  while (1)
  {
    parser_skip_whitespace(parser);

    if (PARSER_EOF == parser_peek(parser))
      break;

    if ('/' == parser_peek(parser))
    {
      parser_advance(parser);
    }

    if (':' == parser_peek(parser))
    {
      parser_advance(parser);
      frag_t frag = fragment_new(1);
      size_t begin = parser->cursor;
      parser_advance_while(parser, isalnum);
      size_t ident_len = parser->cursor - begin;
      assert(ident_len < FRAG_LABEL_LEN);
      strncpy(frag.label, parser->content + begin, ident_len);
      path->frags[path->nfrags++] = frag;
    }

    if (isalnum(parser_peek(parser)))
    {
      frag_t frag = fragment_new(0);
      size_t begin = parser->cursor;
      parser_advance_while(parser, isalnum);
      size_t len = parser->cursor - begin;
      assert(len < FRAG_LABEL_LEN);
      strncpy(frag.label, parser->content + begin, len);
      path->frags[path->nfrags++] = frag;
    }
  }

  return path;
}
