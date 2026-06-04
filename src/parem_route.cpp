#include "parem_route.hpp"

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
