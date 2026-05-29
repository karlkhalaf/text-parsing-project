#ifndef PARALLEL_MATCHER_HPP
#define PARALLEL_MATCHER_HPP

#include "dfa.hpp"

#include <cstddef>
#include <string_view>
#include <vector>

constexpr std::size_t INVALID_STATE = static_cast<std::size_t>(-1);

using ChunkMapping = std::vector<std::size_t>;

ChunkMapping simulate_chunk(const Dfa& dfa, std::string_view chunk);

ChunkMapping compose_mappings(const ChunkMapping& left, const ChunkMapping& right);

bool parallel_accepts(const Dfa& dfa, std::string_view text, std::size_t chunk_count);

// Same algorithm as parallel_accepts, but each chunk mapping runs on its own thread.
bool parallel_accepts_threads(const Dfa& dfa, std::string_view text, std::size_t thread_count);

#endif
