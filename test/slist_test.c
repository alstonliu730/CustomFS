
#include <stdio.h>

#include "../slist.h"

int main(int argc, char **argv) {
  slist_t *list1 =
      slist_cons("This", slist_cons("is", slist_cons("a", slist_cons("list", NULL))));

  printf("List 1:\n");
  print_list(list1);

  // Try splitting
  const char *str = "/hello/dir1/dir2/test1/test2/whole lotta gang shit/hello.txt";

  printf("\nExploding \"%s\":\n", str);
  slist_t *list2 = slist_explode(str, '/');

  print_list(list2);

  slist_free(list1);
  slist_free(list2);
  return 0;
}
