#pragma once
#include <string>

struct SessionEntry {
    std::string type;
};

inline SessionEntry parse_jsonl_line(const std::string& line) {
    SessionEntry entry;
    auto pos = line.find("\"type\":\"");
    if (pos != std::string::npos) {
        auto start = pos + 8;
        auto end = line.find('"', start);
        entry.type = line.substr(start, end - start);
    }
    return entry;
}
