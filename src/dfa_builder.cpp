#include "dfa_builder.hpp"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>
static std::vector<std::size_t> sorted_unique(std::vector<std::size_t> v) {
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}
static std::vector<std::size_t> epsilon_closure_set(const Nfa& nfa, const std::vector<std::size_t>& start) {
    std::unordered_set<std::size_t> seen(start.begin(), start.end());
    std::queue<std::size_t> q;
    for (auto s : start) q.push(s);
    while (!q.empty()) {
        auto s = q.front();
        q.pop();
        for (auto nxt : nfa.epsilon_from(s)) {
            if (seen.insert(nxt).second) {
                q.push(nxt);
            }
        }
    }
    std::vector<std::size_t> out(seen.begin(), seen.end());
    return sorted_unique(out);
}
static std::vector<std::size_t> move_set(const Nfa& nfa, const std::vector<std::size_t>& states, char c) {
    std::vector<std::size_t> out;
    for (auto s : states) {
        const auto& tr = nfa.transitions_from(s);
        auto it = tr.find(c);
        if (it != tr.end()) out.push_back(it->second);
    }
    return sorted_unique(out);
}
static bool subset_has_final(const Nfa& nfa, const std::vector<std::size_t>& subset) {
    for (auto s : subset) {
        if (nfa.is_final(s)) return true;
    }
    return false;
}
static std::vector<char> collect_alphabet(const Nfa& nfa) {
    std::unordered_set<char> chars;
    for (std::size_t s = 0; s < nfa.state_count(); ++s) {
        for (const auto& kv : nfa.transitions_from(s)) {
            chars.insert(kv.first);
        }
    }
    std::vector<char> alphabet(chars.begin(), chars.end());
    std::sort(alphabet.begin(), alphabet.end());
    return alphabet;
}
static std::string key_of(const std::vector<std::size_t>& subset) {
    std::string k;
    k.reserve(subset.size() * 4);
    for (auto x : subset) {
        k.append(std::to_string(x));
        k.push_back(',');
    }
    return k;
}
Dfa build_dfa_from_nfa(const Nfa& nfa) {
    const std::vector<char> alphabet = collect_alphabet(nfa);
    std::vector<std::vector<std::size_t>> subsets;
    std::unordered_map<std::string, std::size_t> id_of;
    const std::vector<std::size_t> start =
        epsilon_closure_set(nfa, std::vector<std::size_t>{nfa.initial_state()});
    subsets.push_back(start);
    id_of.emplace(key_of(start), 0);
    struct Edge { std::size_t from; char c; std::size_t to; };
    std::vector<Edge> edges;
    std::queue<std::size_t> q;
    q.push(0);
    while (!q.empty()) {
        const std::size_t from_id = q.front();
        q.pop();
        const auto& from_subset = subsets[from_id];
        for (char c : alphabet) {
            const std::vector<std::size_t> moved = move_set(nfa, from_subset, c);
            if (moved.empty()) continue;
            const std::vector<std::size_t> target = epsilon_closure_set(nfa, moved);
            const std::string k = key_of(target);
            auto it = id_of.find(k);
            std::size_t to_id;
            if (it == id_of.end()) {
                to_id = subsets.size();
                subsets.push_back(target);
                id_of.emplace(k, to_id);
                q.push(to_id);
            } else {
                to_id = it->second;
            }
            edges.push_back({from_id, c, to_id});
        }
    }
    Dfa dfa(subsets.size(), 0);
    for (std::size_t i = 0; i < subsets.size(); ++i) {
        if (subset_has_final(nfa, subsets[i])) {
            dfa.set_final(i, true);
        }
    }
    for (const auto& e : edges) {
        dfa.add_transition(e.from, e.c, e.to);
    }
    return dfa;
}
