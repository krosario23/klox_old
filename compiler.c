#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
     init_scanner(source);
     int line = -1;

     for (;;) {
          token tok = scan_token();
          if (tok.line != line) {
               printf("%4d ", tok.line);
               line = tok.line;
          } else {
               printf("       | ");
          }
          printf("%2d '%.*s'\n", tok.type, tok.length, tok.start);

          if (tok.type == TOKEN_EOF) break;
     }
}
