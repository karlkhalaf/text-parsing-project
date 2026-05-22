#include "regex_parser.hpp"

#include <cassert>

int main() {
    const std::vector<RegexToken> tokens = tokenize_regex("(a|b)*");

    assert(tokens.size() == 6);
    assert(tokens[0].type == RegexTokenType::LeftParen);
    assert(tokens[1].type == RegexTokenType::Literal);
    assert(tokens[1].value == 'a');
    assert(tokens[2].type == RegexTokenType::Union);
    assert(tokens[3].type == RegexTokenType::Literal);
    assert(tokens[3].value == 'b');
    assert(tokens[4].type == RegexTokenType::RightParen);
    assert(tokens[5].type == RegexTokenType::Star);

    return 0;
}