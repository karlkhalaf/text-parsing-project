#include "matcher.hpp"
#include "parem_route.hpp"
#include "parallel_matcher.hpp"

#include <cassert>
#include <vector>

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

static void test_parallel_reduce_matches_sequential_reduce() {
    const Dfa dfa = build_dfa_from_regex("ab");
    const ChunkMapping left = simulate_chunk(dfa, "a");
    const ChunkMapping right = simulate_chunk(dfa, "b");

    RouteVector route_left(dfa.state_count());
    route_left.set_route(0, left[0]);

    RouteVector route_right(dfa.state_count());
    for (std::size_t state = 0; state < dfa.state_count(); ++state) {
        if (right[state] != INVALID_STATE) {
            route_right.set_route(state, right[state]);
        }
    }

    const std::vector<RouteVector> routes = {route_left, route_right, route_left};
    assert(reduce_routes_sequential(routes).apply(0) ==
           reduce_routes_parallel(routes).apply(0));
}

static void test_empty_route_list_keeps_initial_state() {
    const std::vector<RouteVector> routes;
    assert(apply_routes_to_state(routes, 2) == 2);
    assert(apply_routes_parallel(routes, 2) == 2);
}

int main() {
    test_route_compose_matches_mapping_compose();
    test_parallel_reduce_matches_sequential_reduce();
    test_empty_route_list_keeps_initial_state();
    return 0;
}
