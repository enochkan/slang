#include "utils.h"

bool isWhitespace(char ch) {
    return ch == ' ' || ch == '\n' || ch == '\t';
}

bool isDigit(char ch) {
    return ch >= '0' && ch <= '9';
}

bool isAlpha(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}