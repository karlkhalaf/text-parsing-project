#include "matcher.hpp"
#include "parallel_matcher.hpp"

#include <cassert>
#include <vector>

static void check_same(const char* pattern, const char* text, std::size_t chunk_count) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == parallel_accepts(dfa, text, chunk_count));
}

static void check_same_threads(const char* pattern, const char* text, std::size_t threads) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == parallel_accepts_threads(dfa, text, threads));
}

static void check_same_pruned(const char* pattern, const char* text, std::size_t chunks) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == parallel_accepts_pruned(dfa, text, chunks));
}

static void check_same_pruned_threads(const char* pattern, const char* text, std::size_t threads) {
    const Dfa dfa = build_dfa_from_regex(pattern);
    assert(dfa.accepts(text) == parallel_accepts_pruned_threads(dfa, text, threads));
}

static void check_parem_candidates_are_reduced() {
    const Dfa dfa = build_dfa_from_regex("ab");
    const std::vector<std::size_t> candidates =
        candidate_states_for_chunk(dfa, "a", "b", false);

    assert(!candidates.empty());
    assert(candidates.size() < dfa.state_count());
}

static void check_parem_boundary_depth_reduces_candidates() {
    const Dfa dfa = build_dfa_from_regex("abc");
    const std::vector<std::size_t> one_char =
        candidate_states_for_chunk(dfa, "a", "bc", false);
    const std::vector<std::size_t> two_char =
        candidate_states_for_chunk(dfa, "ab", "c", false);

    assert(!one_char.empty());
    assert(!two_char.empty());
    assert(two_char.size() <= one_char.size());
}

int main() {
    check_same("a", "", 1);
    check_same("a", "", 4);
    check_same("a", "a", 1);
    check_same("a", "a", 3);

    check_same("a|b", "b", 1);
    check_same("a|b", "b", 2);
    check_same("a|b", "ab", 4);

    check_same("(a|b)*", "abba", 1);
    check_same("(a|b)*", "abba", 5);
    check_same("(a|b)*", "c", 3);

    check_same_threads("(a|b)*", "abba", 4);
    check_same_threads("(a|b)*", "c", 4);
    check_same_threads("a|b", "b", 2);

    check_same_pruned("a*", "aaaa", 3);
    check_same_pruned("a|b", "b", 2);
    check_same_pruned("ab", "ab", 2);
    check_same_pruned("(a|b)*", "abba", 4);
    check_same_pruned("(a|b)*", "c", 4);

    check_same_pruned_threads("a*", "aaaa", 4);
    check_same_pruned_threads("a|b", "b", 2);
    check_same_pruned_threads("ab", "ab", 4);
    check_same_pruned_threads("(a|b)*", "abba", 4);
    check_same_pruned_threads("(a|b)*", "c", 4);

    check_parem_candidates_are_reduced();
    check_parem_boundary_depth_reduces_candidates();

    return 0;
}
