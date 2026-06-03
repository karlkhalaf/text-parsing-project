#ifndef SFA_HPP
#define SFA_HPP

#include "dense_dfa.hpp"
#include "dfa.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

// One SFA state = a function f : Q -> Q from the underlying DFA (invalid = no transition).
using SfaMapping = std::vector<std::size_t>;

// Helpers used by build_sfa_from_dfa (Algorithm 4, reference [6]).
SfaMapping make_identity_mapping(std::size_t dfa_state_count);
SfaMapping apply_symbol_to_mapping(const DenseDfa& dfa, const SfaMapping& mapping, char symbol);
bool same_mapping(const SfaMapping& left, const SfaMapping& right);

class Sfa {
public:
    static constexpr std::size_t invalid_sfa_state = static_cast<std::size_t>(-1);

    // Phase 3 (Chkeibs): subset construction from DFA using the helpers above.
    static Sfa build_from_dfa(const Dfa& dfa);

    // Phase 4: single-thread walk on the SFA transition table.
    bool accepts(std::string_view text) const;

    // Phase 5: chunk the text, walk each chunk on the SFA, then merge SFA states.
    bool accepts_parallel(std::string_view text, std::size_t thread_count) const;

    std::size_t sfa_state_count() const;
    std::size_t initial_sfa_state() const;
    bool is_final(std::size_t sfa_state) const;
    std::size_t next_sfa_state(std::size_t from, char symbol) const;

    std::size_t underlying_dfa_state_count() const;
    const SfaMapping& mapping_for_state(std::size_t sfa_state) const;
    const std::vector<char>& alphabet() const;

private:
    Sfa(
        std::size_t dfa_state_count,
        std::size_t dfa_initial_state,
        std::vector<SfaMapping> state_mappings,
        std::size_t initial_sfa_state,
        std::vector<bool> final_sfa_states,
        std::vector<char> alphabet,
        std::vector<std::size_t> symbol_to_index,
        std::vector<std::vector<std::size_t>> transitions_by_symbol
    );

    std::size_t symbol_index(char symbol) const;
    void check_sfa_state(std::size_t sfa_state) const;

    std::size_t dfa_state_count_;
    std::size_t dfa_initial_state_;
    std::vector<SfaMapping> state_mappings_;
    std::size_t initial_sfa_state_;
    std::vector<bool> final_sfa_states_;
    std::vector<char> alphabet_;
    std::vector<std::size_t> symbol_to_index_;
    std::vector<std::vector<std::size_t>> transitions_by_symbol_;
};

#endif
