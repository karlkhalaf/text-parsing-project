#include "regex_parser.hpp"
#include <cassert>
void test_tokenize_simple_pattern() {
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
}
void test_postfix_for_union_pattern() {
    const std::vector<RegexToken> postfix = parse_to_postfix("a|b");
    assert(postfix.size() == 3);
    assert(postfix[0].type == RegexTokenType::Literal);
    assert(postfix[0].value == 'a');
    assert(postfix[1].type == RegexTokenType::Literal);
    assert(postfix[1].value == 'b');
    assert(postfix[2].type == RegexTokenType::Union);
}
void test_postfix_for_starred_group() {
    const std::vector<RegexToken> postfix = parse_to_postfix("(a|b)*");
    assert(postfix.size() == 4);
    assert(postfix[0].type == RegexTokenType::Literal);
    assert(postfix[0].value == 'a');
    assert(postfix[1].type == RegexTokenType::Literal);
    assert(postfix[1].value == 'b');
    assert(postfix[2].type == RegexTokenType::Union);
    assert(postfix[3].type == RegexTokenType::Star);
}
int main() {
    test_tokenize_simple_pattern();
    test_postfix_for_union_pattern();
    test_postfix_for_starred_group();
    return 0;
}
