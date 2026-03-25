#pragma once
#include <string>
#include "../third_party/json.hpp"

using json = nlohmann::json;

struct SessionEntry {
    std::string type;
};

inline std::string extract_entry_type(const json& entry) {
    if (entry.contains("type")) {
        return entry["type"].get<std::string>();
    }
    return entry["message"]["role"].get<std::string>();
}

inline SessionEntry parse_jsonl_line(const std::string& line) {
    auto parsed = json::parse(line);
    return SessionEntry{extract_entry_type(parsed)};
}
