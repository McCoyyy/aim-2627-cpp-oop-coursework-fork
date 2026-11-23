#include "warehouse/domain.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace warehouse {
namespace {

bool has_edge_whitespace(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    const auto first = static_cast<unsigned char>(value.front());
    const auto last = static_cast<unsigned char>(value.back());
    return std::isspace(first) != 0 || std::isspace(last) != 0;
}

}  // namespace

void Inventory::add_station(const std::string& station_id, int stock) {
    if (has_edge_whitespace(station_id)) {
        throw std::invalid_argument(
            "station id must be non-empty without surrounding whitespace");
    }
    if (stock < 0) {
        throw std::invalid_argument("station stock must not be negative");
    }
    const auto inserted = stations_.emplace(
        station_id,
        PickStation{station_id, stock, RobotState::idle, std::string{}});
    if (!inserted.second) {
        throw std::invalid_argument("station already registered: " + station_id);
    }
}

std::optional<PickStation> Inventory::station(
    const std::string& station_id) const {
    const auto it = stations_.find(station_id);
    if (it == stations_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<PickStation> Inventory::stations() const {
    std::vector<PickStation> result;
    result.reserve(stations_.size());
    for (const auto& entry : stations_) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<PickStation> Inventory::available_for(const Order& order) const {
    std::vector<PickStation> candidates;
    for (const auto& entry : stations_) {
        const PickStation& candidate = entry.second;
        if (candidate.robot_state == RobotState::idle &&
            candidate.stock >= order.quantity) {
            candidates.push_back(candidate);
        }
    }
    return candidates;
}

bool Inventory::reserve(const Order& order, const std::string& station_id) {
    if (order.quantity <= 0) {
        return false;
    }
    const auto it = stations_.find(station_id);
    if (it == stations_.end()) {
        return false;
    }
    PickStation& station = it->second;
    if (station.robot_state != RobotState::idle) {
        return false;
    }
    if (station.stock < order.quantity) {
        return false;
    }
    station.stock -= order.quantity;
    station.robot_state = RobotState::busy;
    station.current_order_id = order.id;
    return true;
}

bool Inventory::finish(const std::string& station_id,
                       const std::string& order_id) {
    const auto it = stations_.find(station_id);
    if (it == stations_.end()) {
        return false;
    }
    PickStation& station = it->second;
    if (station.robot_state != RobotState::busy) {
        return false;
    }
    if (station.current_order_id != order_id) {
        return false;
    }
    station.robot_state = RobotState::idle;
    station.current_order_id.clear();
    return true;
}

}  // namespace warehouse
