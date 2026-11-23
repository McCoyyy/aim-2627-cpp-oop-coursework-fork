#include "app/cli.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <string>

namespace app {
namespace {

bool has_edge_whitespace(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    const auto first = static_cast<unsigned char>(value.front());
    const auto last = static_cast<unsigned char>(value.back());
    return std::isspace(first) != 0 || std::isspace(last) != 0;
}

std::string trim(const std::string& value) {
    std::size_t begin = 0;
    std::size_t end = value.size();
    while (begin < end &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(begin, end - begin);
}

}  // namespace

std::string normalize_sku(const std::string& sku) {
    std::string normalized = trim(sku);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::toupper(character));
                   });
    return normalized;
}

bool valid_station_id(const std::string& station_id) {
    return !has_edge_whitespace(station_id);
}

bool valid_order_id(const std::string& order_id) {
    return !has_edge_whitespace(order_id);
}

int run() {
    std::cout << "Use the ROS 2 demo package for the end-to-end runtime.\n";
    return 0;
}

int print_trace(int argc, char** argv) {
    if (argc > 1 && std::string(argv[1]) == "--trace") {
        std::cout << "TRACE command is reserved for grading\n";
        return 0;
    }
    return run();
}

}  // namespace app
