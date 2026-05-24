#include "regex_parser.hpp"
#include <cctype>
#include <stdexcept>
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
            if (std::isprint(static_cast<unsigned char>(c)) && !std::isspace(static_cast<unsigned char>(c))) {
                tokens.push_back({RegexTokenType::Literal, c});
            }
            break;
        }
    }
    return tokens;
}
static bool is_operand_start(RegexTokenType type) {
    return type == RegexTokenType::Literal || type == RegexTokenType::LeftParen;
}
static bool is_operand_end(RegexTokenType type) {
    return type == RegexTokenType::Literal || type == RegexTokenType::RightParen ||
           type == RegexTokenType::Star;
}
static int precedence(RegexTokenType type) {
    switch (type) {
    case RegexTokenType::Union:
        return 1;
    case RegexTokenType::Concat:
        return 2;
    case RegexTokenType::Star:
        return 3;
    default:
        return 0;
    }
}
static std::vector<RegexToken> insert_implicit_concat(const std::vector<RegexToken>& tokens) {
    std::vector<RegexToken> with_concat;
    bool has_previous = false;
    RegexTokenType previous_type = RegexTokenType::Literal;
    for (const RegexToken& token : tokens) {
        if (has_previous && is_operand_end(previous_type) && is_operand_start(token.type)) {
            with_concat.push_back({RegexTokenType::Concat, '.'});
        }
        with_concat.push_back(token);
        has_previous = true;
        previous_type = token.type;
    }
    return with_concat;
}
std::vector<RegexToken> parse_to_postfix(const std::string& pattern) {
    const std::vector<RegexToken> tokens = insert_implicit_concat(tokenize_regex(pattern));
    std::vector<RegexToken> output;
    std::vector<RegexToken> operators;
    for (const RegexToken& token : tokens) {
        switch (token.type) {
        case RegexTokenType::Literal:
            output.push_back(token);
            break;
        case RegexTokenType::Concat:
        case RegexTokenType::Union:
            while (!operators.empty() && operators.back().type != RegexTokenType::LeftParen &&
                   precedence(operators.back().type) >= precedence(token.type)) {
                output.push_back(operators.back());
                operators.pop_back();
            }
            operators.push_back(token);
            break;
        case RegexTokenType::Star:
            output.push_back(token);
            break;
        case RegexTokenType::LeftParen:
            operators.push_back(token);
            break;
        case RegexTokenType::RightParen:
            while (!operators.empty() && operators.back().type != RegexTokenType::LeftParen) {
                output.push_back(operators.back());
                operators.pop_back();
            }
            if (operators.empty()) {
                throw std::invalid_argument("mismatched parentheses in regex");
            }
            operators.pop_back();
            break;
        }
    }
    while (!operators.empty()) {
        if (operators.back().type == RegexTokenType::LeftParen) {
            throw std::invalid_argument("mismatched parentheses in regex");
        }
        output.push_back(operators.back());
        operators.pop_back();
    }
    return output;
}
