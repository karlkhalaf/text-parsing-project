#include "parem_route.hpp"

#include <thread>

RouteVector::RouteVector(std::size_t state_count)
    : state_count_(state_count) {}

std::size_t RouteVector::state_count() const {
    return state_count_;
}

std::size_t RouteVector::active_route_count() const {
    return routes_.size();
}

const std::vector<RouteEntry>& RouteVector::entries() const {
    return routes_;
}

void RouteVector::set_route(std::size_t from, std::size_t to) {
    if (from >= state_count_) {
        return;
    }
    routes_.push_back(RouteEntry{from, to});
}

std::size_t RouteVector::apply(std::size_t from) const {
    for (const RouteEntry& route : routes_) {
        if (route.from == from) {
            return route.to;
        }
    }
    return ROUTE_INVALID_STATE;
}

RouteVector RouteVector::compose_with(const RouteVector& earlier) const {
    RouteVector composed(earlier.state_count());

    for (const RouteEntry& entry : earlier.routes_) {
        const std::size_t middle = entry.to;
        if (middle == ROUTE_INVALID_STATE) {
            continue;
        }
        const std::size_t end = apply(middle);
        if (end == ROUTE_INVALID_STATE) {
            continue;
        }
        composed.set_route(entry.from, end);
    }

    return composed;
}

RouteVector RouteVector::from_chunk_mapping(
    const std::vector<std::size_t>& mapping,
    const std::vector<std::size_t>& active_starts
) {
    RouteVector route(mapping.size());
    for (std::size_t start : active_starts) {
        if (start >= mapping.size()) {
            continue;
        }
        const std::size_t end = mapping[start];
        if (end == ROUTE_INVALID_STATE) {
            continue;
        }
        route.set_route(start, end);
    }
    return route;
}

RouteVector reduce_routes_sequential(const std::vector<RouteVector>& routes) {
    if (routes.empty()) {
        return RouteVector(0);
    }

    RouteVector accumulated = routes.front();
    for (std::size_t i = 1; i < routes.size(); ++i) {
        accumulated = routes[i].compose_with(accumulated);
    }
    return accumulated;
}

RouteVector reduce_routes_parallel(const std::vector<RouteVector>& routes) {
    if (routes.empty()) {
        return RouteVector(0);
    }

    std::vector<RouteVector> level = routes;
    while (level.size() > 1) {
        const std::size_t pair_count = level.size() / 2;
        const bool has_tail = (level.size() % 2 == 1);
        const std::size_t next_size = pair_count + (has_tail ? 1 : 0);

        std::vector<RouteVector> next(next_size, RouteVector(level.front().state_count()));
        std::vector<std::thread> workers;
        workers.reserve(pair_count);

        for (std::size_t pair = 0; pair < pair_count; ++pair) {
            workers.emplace_back([&level, &next, pair]() {
                const std::size_t left = 2 * pair;
                next[pair] = level[left + 1].compose_with(level[left]);
            });
        }

        for (std::thread& worker : workers) {
            worker.join();
        }

        if (has_tail) {
            next[next_size - 1] = level.back();
        }

        level = std::move(next);
    }

    return level.front();
}
