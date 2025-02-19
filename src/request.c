#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <falcon/errn.h>
#include <falcon/request.h>
#include <falcon/stringview.h>

static void fc__trim_left(const char *content, const size_t content_len, size_t *cursor);
static int fc__is_eof(const size_t content_len, const size_t cursor);
static size_t fc__advance_upto(const char *content, const size_t content_len, size_t *cursor, char limit);

/*
  NOTE: This procedure does not cover edge cases.
  TODO: Handle edge cases
*/
fc_errno fc__req_parse_cookies(fc_request_t *req)
{
  req->cookies.parsed = true;

  char *content = NULL;
  size_t content_len = 0;
  fc_errno err = fc__sv_kv_array_get(&req->headers, "Cookie", &content, &content_len);
  if (FC_ERR_OK != err)
  {
    return err;
  }

  size_t cursor = 0;
  while (cursor < content_len)
  {
    fc__trim_left(content, content_len, &cursor);
    if (fc__is_eof(content_len, cursor))
    {
      break;
    }

    fc_stringview_t key = {.ptr = &content[cursor], .len = fc__advance_upto(content, content_len, &cursor, '=')};
    fc_stringview_t val = {.ptr = &content[cursor], .len = fc__advance_upto(content, content_len, &cursor, ';')};
    req->cookies.cookies.items[req->cookies.cookies.count++] = (fc__sv_kv){.key = key, .value = val};
  }

  return FC_ERR_OK;
}

static int fc__is_eof(const size_t content_len, const size_t cursor)
{
  return cursor >= content_len;
}

static void fc__trim_left(const char *content, const size_t content_len, size_t *cursor)
{
  while (!fc__is_eof(content_len, (*cursor)) && isspace(content[(*cursor)]))
  {
    (*cursor)++;
  }
}

static size_t fc__advance_upto(const char *content, const size_t content_len, size_t *cursor, char limit)
{
  size_t begin = *cursor;
  while (!fc__is_eof(content_len, (*cursor)) && limit != content[(*cursor)])
  {
    (*cursor)++;
  }
  size_t advanced_length = (*cursor) - begin;
  (*cursor)++; // skip the 'limit' char
  return advanced_length;
}
