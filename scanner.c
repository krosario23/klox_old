#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
     const char* start;
     const char* current;
     int line;
} scanner;

scanner scan;

void init_scanner(const char* source) {
     scan.start = source;
     scan.current = source;
     scan.line = 1;
}

static bool is_alpha(char c) {
     return (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
             c == '_';}

static bool is_digit(char c) {
  return c >= '0' && c <= '9';
}

//checks for null terminator to see if source
static bool is_at_end() {
     return *scan.current == '\0';
}

//advances the source code pointer and returns the consumes character
static char advance() {
     scan.current++;
     return scan.current[-1];
}

//return the current character in the source code without consuming it
static char peek() {
     return *scan.current;
}

static char peek_next() {
     if (is_at_end()) return '\0';
     return scan.current[1];
}

//check if the current char in the soruce code matches the expected char
static bool match(char expected) {
     if (is_at_end()) return false;
     if (*scan.current != expected) return false;

     scan.current++;
     return true;
}

//a token constructor
static token make_token(token_type type) {
     token tok;
     tok.type = type;
     tok.start = scan.start;
     tok.length = (int)(scan.current - scan.start);
     tok.line = scan.line;

     return tok;
}

static token error_token(const char* msg) {
     token tok;
     tok.type = TOKEN_ERROR;
     tok.start = msg;
     tok.length = (int)strlen(msg);
     tok.line = scan.line;

     return tok;
}

static void skip_whitespace() {
     for (;;) {
          char c = peek();
          switch (c) {
               case ' ':
               case '\r':
               case '\t':
                    advance();
                    break;
               case '\n':
                    scan.line++;
                    advance();
                    break;
               case '/':
                    if (peek_next() == '/') {
                         while (peek() != '\n' && !is_at_end()) advance();
                    } else return;
                    break;
               default:
                    return;
          }
     }
}

static token_type check_keyword(int start, int length, const char* rest,
                                token_type type) {
     if (scan.current - scan.start == start + length &&
         memcmp(scan.start + start, rest, length) == 0) {
              return type;
     }

     return TOKEN_IDENTIFIER;
}

static token_type identifier_type() {
     switch (scan.start[0]) {
          case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
          case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
          case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
          case 'f': {
               if (scan.current - scan.start > 1) {
                    switch (scan.start[1]) {
                         case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
                         case 'o': return check_keyword(2, 1, "or", TOKEN_FOR);
                         case 'u': return check_keyword(2, 2, "nc", TOKEN_FUNC);
                    }
               }
               break;
          }
          case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
          case 'l': return check_keyword(1, 2, "et", TOKEN_LET);
          case 'n': return check_keyword(1, 3, "ull", TOKEN_NULL);
          case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
          case 'p': return check_keyword(1, 4, "rint", TOKEN_PRINT);
          case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
          case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
          case 't': {
               if (scan.current - scan.start > 1) {
                    switch (scan.start[1]) {
                         case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
                         case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
                    }
               }
               break;
          }
          case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
     }
     return TOKEN_IDENTIFIER;
}

static token identifier() {
     while (is_alpha(peek()) || is_digit(peek())) advance();

     return make_token(identifier_type());
}

static token number() {
     while (is_digit(peek())) advance();
     if (peek() == '.' && is_digit(peek_next())) {
          advance();
          while (is_digit(peek())) advance();
     }

     return make_token(TOKEN_NUMBER);
}

static token handle_string() {
     while (peek() != '=' && !is_at_end()) {
          if (peek() == '\n') scan.line++;
          advance();
     }

     if (is_at_end()) return error_token("unterminated string");

     advance();
     return make_token(TOKEN_STRING);
}

token scan_token() {
     skip_whitespace();

     scan.start = scan.current;

     if (is_at_end()) return make_token(TOKEN_EOF);

     char c = advance();

     if (is_alpha(c)) return identifier();
     if (is_digit(c)) return number();

     switch (c) {
          case '(': return make_token(TOKEN_LEFT_PAREN);
          case ')': return make_token(TOKEN_RIGHT_PAREN);
          case '{': return make_token(TOKEN_LEFT_BRACE);
          case '}': return make_token(TOKEN_RIGHT_BRACE);
          case ';': return make_token(TOKEN_SEMICOLON);
          case ',': return make_token(TOKEN_COMMA);
          case '.': return make_token(TOKEN_DOT);
          case '-': return make_token(TOKEN_MINUS);
          case '+': return make_token(TOKEN_PLUS);
          case '/': return make_token(TOKEN_SLASH);
          case '*': return make_token(TOKEN_STAR);
          case '!':
               return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
          case '=':
               return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
          case '<':
               return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
          case '>':
               return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
          case '"': return handle_string();
     }
     return error_token("unexpected character");
}
