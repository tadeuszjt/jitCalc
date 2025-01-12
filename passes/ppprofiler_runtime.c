#include <stdio.h>

void __ppp_enter(const char *ptr) {
    printf("ppp_enter: %s\n", ptr);
}
void __ppp_exit(const char *ptr) {
    printf("ppp_exit: %s\n", ptr);
}

