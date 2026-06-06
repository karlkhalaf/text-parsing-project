#include "dense_dfa.hpp"
#include "matcher.hpp"
#include "parallel_matcher.hpp"
#include "search_matcher.hpp"
#include "sfa.hpp"

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    std::string pattern;
    std::string text;
    std::string input_path;
    std::string mode = "sequential";
    std::string task = "search";
    std::size_t threads = 4;
    bool text_argument_seen = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--regex" && i + 1 < argc) {
            pattern = argv[++i];
        } else if (arg == "--text" && i + 1 < argc) {
            text = argv[++i];
            text_argument_seen = true;
        } else if (arg == "--input" && i + 1 < argc) {
            input_path = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--task" && i + 1 < argc) {
            task = argv[++i];
        } else if (arg == "--threads" && i + 1 < argc) {
            threads = static_cast<std::size_t>(std::stoul(argv[++i]));
        } else if (arg == "--help") {
            std::cout << "Usage: regex_matcher --regex PATTERN (--text TEXT | --input FILE) "
                         "[--mode sequential|parallel|pruned|sfa|naive] "
                         "[--task search|full] [--threads N]\n";
            return 0;
        }
    }

    if (pattern.empty()) {
        std::cerr << "Missing --regex\n";
        return 1;
    }

    if (!text_argument_seen && !input_path.empty()) {
        std::ifstream input_file(input_path);
        if (!input_file) {
            std::cerr << "Could not open --input file\n";
            return 1;
        }
        std::ostringstream buffer;
        buffer << input_file.rdbuf();
        text = buffer.str();
    }

    if (!text_argument_seen && input_path.empty()) {
        std::cerr << "Missing --text or --input\n";
        return 1;
    }

    if (task != "full" && task != "search") {
        std::cerr << "Unknown --task (use full or search)\n";
        return 1;
    }

    try {
        const Dfa dfa =
            (task == "search") ? build_search_dfa_from_regex(pattern, text)
                               : build_dfa_from_regex(pattern);
        bool ok = false;
        if (mode == "sequential") {
            const DenseDfa dense(dfa);
            ok = dense.accepts(text);
        } else if (mode == "naive") {
            ok = dfa.accepts(text);
        } else if (mode == "parallel") {
            ok = parallel_accepts_threads(dfa, text, threads);
        } else if (mode == "pruned") {
            ok = parallel_accepts_pruned_threads(dfa, text, threads);
        } else if (mode == "sfa") {
            const Sfa sfa = Sfa::build_from_dfa(dfa);
            ok = sfa.accepts_parallel(text, threads);
        } else {
            std::cerr << "Unknown --mode (use sequential, parallel, pruned, sfa, or naive)\n";
            return 1;
        }
        std::cout << (ok ? "ACCEPT\n" : "REJECT\n");
        return ok ? 0 : 2;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
