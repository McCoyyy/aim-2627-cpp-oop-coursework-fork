#include "dispatch/scheduler.hpp"

#include <cctype>
#include <stdexcept>
#include <utility>

namespace dispatch {
namespace {

bool has_edge_whitespace(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    const auto first = static_cast<unsigned char>(value.front());
    const auto last = static_cast<unsigned char>(value.back());
    return std::isspace(first) != 0 || std::isspace(last) != 0;
}

DispatchDecision reject(const std::string& order_id, const std::string& reason) {
    return DispatchDecision{false, order_id, std::string{}, reason};
}

}  // namespace

Scheduler::Scheduler(warehouse::Inventory& inventory,
                     std::unique_ptr<DispatchPolicy> policy,
                     audit::Trace& trace)
    : inventory_(inventory), policy_(std::move(policy)), trace_(trace) {
    if (!policy_) {
        throw std::invalid_argument("dispatch policy cannot be null");
    }
}

DispatchDecision Scheduler::submit(const warehouse::Order& order) {
    if (has_edge_whitespace(order.id)) {
        return reject(order.id, "invalid order id");
    }
    if (has_edge_whitespace(order.sku)) {
        return reject(order.id, "invalid sku");
    }
    if (order.quantity <= 0) {
        return reject(order.id, "invalid quantity");
    }

    const auto candidates = inventory_.available_for(order);
    if (candidates.empty()) {
        return reject(order.id, "no available station");
    }

    const auto selected = policy_->select(order, candidates);
    if (!selected) {
        return reject(order.id, "no available station");
    }

    if (!inventory_.reserve(order, *selected)) {
        return reject(order.id, "reservation rejected");
    }

    trace_.append("DISPATCH " + order.id + " " + *selected);
    return DispatchDecision{true, order.id, *selected, "accepted"};
}

std::vector<DispatchDecision> Scheduler::submit_all(
    const std::vector<warehouse::Order>& orders) {
    std::vector<DispatchDecision> decisions;
    decisions.reserve(orders.size());
    for (const auto& order : orders) {
        decisions.push_back(submit(order));
    }
    return decisions;
}

bool Scheduler::complete(const std::string& station_id,
                         const std::string& order_id,
                         std::string& failure_reason) {
    const auto station = inventory_.station(station_id);
    if (!station) {
        failure_reason = "unknown station: " + station_id;
        return false;
    }
    if (station->robot_state != warehouse::RobotState::busy) {
        failure_reason = "station is idle: " + station_id;
        return false;
    }
    if (station->current_order_id != order_id) {
        failure_reason = "station " + station_id + " holds order " +
                         station->current_order_id + ", not " + order_id;
        return false;
    }
    if (!inventory_.finish(station_id, order_id)) {
        failure_reason = "completion rejected: " + station_id;
        return false;
    }
    trace_.append("COMPLETE " + order_id + " " + station_id);
    failure_reason.clear();
    return true;
}

}  // namespace dispatch
