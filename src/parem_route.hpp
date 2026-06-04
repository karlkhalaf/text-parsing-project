#ifndef PAREM_ROUTE_HPP
#define PAREM_ROUTE_HPP

#include <cstddef>
#include <vector>

constexpr std::size_t ROUTE_INVALID_STATE = static_cast<std::size_t>(-1);

struct RouteEntry {
    std::size_t from;
    std::size_t to;
};

class RouteVector {
public:
    explicit RouteVector(std::size_t state_count);

    std::size_t state_count() const;
    std::size_t active_route_count() const;
    const std::vector<RouteEntry>& entries() const;

    void set_route(std::size_t from, std::size_t to);
    std::size_t apply(std::size_t from) const;

    RouteVector compose_with(const RouteVector& earlier) const;

    static RouteVector from_chunk_mapping(
        const std::vector<std::size_t>& mapping,
        const std::vector<std::size_t>& active_starts
    );

private:
    std::size_t state_count_;
    std::vector<RouteEntry> routes_;
};

#endif
