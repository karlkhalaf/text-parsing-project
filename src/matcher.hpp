#ifndef MATCHER_HPP
#define MATCHER_HPP

#include "dfa.hpp"

#include <string>

Dfa build_dfa_from_regex(const std::string& pattern);

#endif
