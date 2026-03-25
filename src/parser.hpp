#pragma once
#include <string>
#include <vector>
#include "../third_party/json.hpp"

using json = nlohmann::json;

struct ContentBlock {
    std::string type;
    std::string text;
    std::string tool_name;
    std::string tool_input;
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

inline std::string extract_block_text(const json& block) {
    std::string text = block.value("text", "");
    if (text.empty()) {
        text = block.value("thinking", "");
    }
    return text;
}

inline std::vector<ContentBlock> extract_content(const json& entry) {
    std::vector<ContentBlock> blocks;
    if (entry.contains("message") && entry["message"].contains("content")) {
        for (const auto& block : entry["message"]["content"]) {
            std::string tool_name = block.value("name", "");
            std::string tool_input = block.contains("input") ? block["input"].dump() : "";
            blocks.push_back({block.value("type", ""), extract_block_text(block), tool_name, tool_input});
        }
    }
    return blocks;
}

inline SessionEntry parse_jsonl_line(const std::string& line) {
    auto parsed = json::parse(line);
    return SessionEntry{extract_entry_type(parsed), extract_content(parsed)};
}
