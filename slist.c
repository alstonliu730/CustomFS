/**
 * @file slist.c
 * @author CS3650 staff
 *
 * A simple linked list of strings.
 *
 * This might be useful for directory listings and for manipulating paths.
 */

#include <alloca.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "slist.h"

slist_t *slist_cons(const char *text, slist_t *rest) {
  slist_t *xs = malloc(sizeof(slist_t));
  xs->data = strdup(text);
  xs->refs = 1;
  xs->next = rest;
  return xs;
}

void slist_free(slist_t *xs) {
  if (xs == 0) {
    return;
  }

  xs->refs -= 1;

  if (xs->refs == 0) {
    slist_free(xs->next);
    free(xs->data);
    free(xs);
  }
}

slist_t *slist_explode(const char *text, char delim) {
  // printf("DEBUG: slist_explode(%s, %c) -> Called function.\n", text, delim);
  if (*text == 0) {
    return 0;
  }

  int plen = 0;
  while (text[plen] != 0 && text[plen] != delim) {
    plen += 1;
  }

  int skip = 0;
  if (text[plen] == delim) {
    skip = 1;
  }

  slist_t *rest = slist_explode(text + plen + skip, delim);
  char *part = alloca(plen + 2);
  memcpy(part, text, plen);
  part[plen] = 0;

  // printf("DEBUG: slist_explode(%s, %c) -> (%p)\n", text, delim, slist_cons(part, rest));
  return slist_cons(part, rest);
}

void print_list(slist_t *list) {
  while (list != NULL) {
    printf("%s\n", list->data);
    list = list->next;
  }
}