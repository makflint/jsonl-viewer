#pragma once
#include <string>
#include "../third_party/json.hpp"

using json = nlohmann::json;

struct SessionEntry {
    std::string type;
};

inline SessionEntry parse_jsonl_line(const std::string& line) {
    auto j = json::parse(line);
    if (j.contains("type")) {
        return SessionEntry{j["type"].get<std::string>()};
    }
    return SessionEntry{j["message"]["role"].get<std::string>()};
}
