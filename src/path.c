#include <falcon.h>
#include <falcon/path.h>

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#define PARSER_EOF '\0'

typedef struct
{
  const char *content;
  size_t content_len;
  size_t cursor;
} parser_t;

int parser_is_eof(parser_t *parser)
{
  return parser->cursor >= parser->content_len;
}

char parser_peek(parser_t *parser)
{
  return parser_is_eof(parser) ? PARSER_EOF : parser->content[parser->cursor];
}

void parser_advance(parser_t *parser)
{
  parser->cursor++;
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

frag_t fragment_new(int is_parameter)
{
  frag_t frag;
  frag.is_parameter = is_parameter;
  memset(frag.label, 0, sizeof(frag.label));
  return frag;
}

ferrno parse_fragment(parser_t *parser, frag_t *frag)
{
  ferrno err = FERR_OK;

  size_t begin = parser->cursor;
  parser_advance_while(parser, isalnum);
  size_t ident_len = parser->cursor - begin;

  if (ident_len >= FRAG_LABEL_LEN)
    err = FERR_STRING_TOO_LONG;
  else
    strncpy(frag->label, parser->content + begin, ident_len);

  return err;
}

ferrno push_fragment(fpath_t *path, frag_t frag)
{
  ferrno err = FERR_OK;
  if (path->nfrags >= PATH_MAX_FRAGS)
    err = FERR_TOO_MANY_FRAGS;
  else
    path->frags[path->nfrags++] = frag;
  return err;
}

ferrno parse_path(const char *content, fpath_t *path)
{
  ferrno err = FERR_OK;
  parser_t parser = {.content = content, .content_len = PATH_MAX_LEN, .cursor = 0};

  while (1)
  {
    parser_skip_whitespace(&parser);

    if (PARSER_EOF == parser_peek(&parser))
      break;

    if ('/' == parser_peek(&parser))
    {
      parser_advance(&parser); // eat '/'
    }
    else if (isalnum(parser_peek(&parser)))
    {
      frag_t frag = fragment_new(0);
      err = parse_fragment(&parser, &frag);
      if (FERR_OK != err)
        break;
      err = push_fragment(path, frag);
      if (FERR_OK != err)
        break;
    }
    else if (':' == parser_peek(&parser))
    {
      parser_advance(&parser); // eat ':'
      frag_t frag = fragment_new(1);
      err = parse_fragment(&parser, &frag);
      if (FERR_OK != err)
        break;
      err = push_fragment(path, frag);
      if (FERR_OK != err)
        break;
    }
    else
    {
      err = FERR_INVALID_TOKEN;
      break;
    }
  }

  return FERR_OK;
}
