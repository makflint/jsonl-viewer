#pragma once
#include <string>
#include <vector>
#include "../third_party/json.hpp"

using json = nlohmann::json;

struct ContentBlock {
    std::string type;
    std::string text;
};

struct SessionEntry {
    std::string type;
    std::vector<ContentBlock> content;
};

inline std::string extract_entry_type(const json& entry) {
    if (entry.contains("type")) {
        return entry["type"].get<std::string>();
    }
    return entry["message"]["role"].get<std::string>();
}

inline std::vector<ContentBlock> extract_content(const json& entry) {
    std::vector<ContentBlock> blocks;
    if (entry.contains("message") && entry["message"].contains("content")) {
        for (const auto& block : entry["message"]["content"]) {
            blocks.push_back({block.value("type", ""), block.value("text", "")});
        }
    }
    return blocks;
}

inline SessionEntry parse_jsonl_line(const std::string& line) {
    auto parsed = json::parse(line);
    return SessionEntry{extract_entry_type(parsed), extract_content(parsed)};
}
