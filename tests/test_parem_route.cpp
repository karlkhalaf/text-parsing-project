#include "matcher.hpp"
#include "parem_route.hpp"
#include "parallel_matcher.hpp"

#include <cassert>

static void test_route_compose_matches_mapping_compose() {
    Dfa dfa(3, 0);
    dfa.add_transition(0, 'a', 1);
    dfa.add_transition(1, 'b', 2);
    dfa.set_final(2);

    const ChunkMapping left = simulate_chunk(dfa, "a");
    const ChunkMapping right = simulate_chunk(dfa, "b");
    const ChunkMapping composed = compose_mappings(left, right);

    RouteVector route_left(dfa.state_count());
    route_left.set_route(0, left[0]);

    RouteVector route_right(dfa.state_count());
    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        if (right[state] != INVALID_STATE) {
            route_right.set_route(state, right[state]);
        }
    }

    const RouteVector route_composed = route_right.compose_with(route_left);
    assert(route_composed.apply(0) == composed[0]);
}

int main() {
    test_route_compose_matches_mapping_compose();
    return 0;
}
