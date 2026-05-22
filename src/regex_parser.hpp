#ifndef REGEX_PARSER_HPP
#define REGEX_PARSER_HPP

#include <string>
#include <vector>

enum class RegexTokenType {
    Literal,
    Union,
    Star,
    LeftParen,
    RightParen
};

struct RegexToken {
    RegexTokenType type;
    char value;
};

std::vector<RegexToken> tokenize_regex(const std::string& pattern);

#endif
