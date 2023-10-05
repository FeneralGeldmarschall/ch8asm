#include "common.h"

bool isHex(char c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f') ||
           ('A' <= c && c <= 'F');
}

bool isBinary(char c) { return ('0' == c || '1' == c); }

bool isDigit(char c) { return ('0' <= c && c <= '9'); }

bool isUpper(char c) { return ('A' <= c && c <= 'Z'); }

bool isLower(char c) { return ('a' <= c && c <= 'z'); }

bool isAlpha(char c) { return (isLower(c) || isUpper(c)); }

bool isAlphaNumeric(char c) { return (isAlpha(c) || isDigit(c)); }