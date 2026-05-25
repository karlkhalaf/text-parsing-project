#include "nfa_builder.hpp"

#include <stack>
#include <stdexcept>

static NfaFragment make_literal(Nfa& nfa, char symbol) {
    const std::size_t start = nfa.add_state();
    const std::size_t end = nfa.add_state();
    nfa.add_transition(start, symbol, end);
    return {start, end};
}

static NfaFragment concatenate(Nfa& nfa, const NfaFragment& left, const NfaFragment& right) {
    nfa.add_epsilon(left.end, right.start);
    return {left.start, right.end};
}

static NfaFragment unite(Nfa& nfa, const NfaFragment& left, const NfaFragment& right) {
    const std::size_t start = nfa.add_state();
    const std::size_t end = nfa.add_state();

    nfa.add_epsilon(start, left.start);
    nfa.add_epsilon(start, right.start);
    nfa.add_epsilon(left.end, end);
    nfa.add_epsilon(right.end, end);

    return {start, end};
}

static NfaFragment star(Nfa& nfa, const NfaFragment& inner) {
    const std::size_t start = nfa.add_state();
    const std::size_t end = nfa.add_state();

    nfa.add_epsilon(start, inner.start);
    nfa.add_epsilon(start, end);
    nfa.add_epsilon(inner.end, inner.start);
    nfa.add_epsilon(inner.end, end);

    return {start, end};
}

Nfa build_nfa_from_postfix(const std::vector<RegexToken>& postfix) {
    Nfa nfa;
    std::stack<NfaFragment> stack;

    for (const RegexToken& token : postfix) {
        switch (token.type) {
        case RegexTokenType::Literal:
            stack.push(make_literal(nfa, token.value));
            break;
        case RegexTokenType::Concat: {
            if (stack.size() < 2) {
                throw std::invalid_argument("invalid postfix expression");
            }
            const NfaFragment right = stack.top();
            stack.pop();
            const NfaFragment left = stack.top();
            stack.pop();
            stack.push(concatenate(nfa, left, right));
            break;
        }
        case RegexTokenType::Union: {
            if (stack.size() < 2) {
                throw std::invalid_argument("invalid postfix expression");
            }
            const NfaFragment right = stack.top();
            stack.pop();
            const NfaFragment left = stack.top();
            stack.pop();
            stack.push(unite(nfa, left, right));
            break;
        }
        case RegexTokenType::Star: {
            if (stack.empty()) {
                throw std::invalid_argument("invalid postfix expression");
            }
            const NfaFragment inner = stack.top();
            stack.pop();
            stack.push(star(nfa, inner));
            break;
        }
        default:
            throw std::invalid_argument("unsupported token in postfix NFA construction");
        }
    }

    if (stack.size() != 1) {
        throw std::invalid_argument("invalid postfix expression");
    }

    const NfaFragment result = stack.top();
    nfa.set_initial(result.start);
    nfa.add_final(result.end);
    return nfa;
}

Nfa build_nfa_from_pattern(const std::string& pattern) {
    return build_nfa_from_postfix(parse_to_postfix(pattern));
}
