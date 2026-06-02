#include "matcher.hpp"
#include "parallel_matcher.hpp"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string pattern;
    std::string text;
    std::string mode = "sequential";
    std::size_t threads = 4;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--regex" && i + 1 < argc) {
            pattern = argv[++i];
        } else if (arg == "--text" && i + 1 < argc) {
            text = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--help") {
            std::cout << "Usage: regex_matcher --regex PATTERN --text TEXT "
                         "[--mode sequential|parallel|pruned|precomputed] [--threads N]\n";
            return 0;
        }
    }

    if (pattern.empty()) {
        std::cerr << "Missing --regex\n";
        return 1;
    }

    try {
        const Dfa dfa = build_dfa_from_regex(pattern);
        bool ok = false;
        if (mode == "sequential") {
            ok = dfa.accepts(text);
        } else if (mode == "parallel") {
            ok = parallel_accepts_threads(dfa, text, threads);
        } else if (mode == "pruned") {
            ok = parallel_accepts_pruned_threads(dfa, text, threads);
        } else if (mode == "precomputed") {
            ok = parallel_accepts_precomputed_threads(dfa, text, threads);
        } else {
            std::cerr << "Unknown --mode (use sequential, parallel, pruned, or precomputed)\n";
            return 1;
        }
        std::cout << (ok ? "ACCEPT\n" : "REJECT\n");
        return ok ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
