#include "matcher.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string pattern;
    std::string text;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--regex" && i + 1 < argc) {
            pattern = argv[++i];
        } else if (arg == "--text" && i + 1 < argc) {
            text = argv[++i];
        } else if (arg == "--help") {
            std::cout << "Usage: regex_matcher --regex PATTERN --text TEXT\n";
            return 0;
        }
    }

    if (pattern.empty()) {
        std::cerr << "Missing --regex\n";
        return 1;
    }

    try {
        const Dfa dfa = build_dfa_from_regex(pattern);
        const bool ok = dfa.accepts(text);
        std::cout << (ok ? "ACCEPT\n" : "REJECT\n");
        return ok ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
