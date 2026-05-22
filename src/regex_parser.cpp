#include "regex_parser.hpp"

std::vector<RegexToken> tokenize_regex(const std::string& pattern) {
    std::vector<RegexToken> tokens;

    for (char c : pattern) {
        switch (c) {
        case '|':
            tokens.push_back({RegexTokenType::Union, c});
            break;
        case '*':
            tokens.push_back({RegexTokenType::Star, c});
            break;
        case '(':
            tokens.push_back({RegexTokenType::LeftParen, c});
            break;
        case ')':
            tokens.push_back({RegexTokenType::RightParen, c});
            break;
        default:
            tokens.push_back({RegexTokenType::Literal, c});
            break;
        }
    }

    return tokens;
}
