#pragma once
#include <string>
#include <sstream>
#include <vector>
#include "../third_party/json.hpp"

using json = nlohmann::json;

struct ContentBlock {
    std::string type;
    std::string text;
    std::string tool_name;
    std::string tool_input;
    std::string tool_use_id;
    bool is_error = false;
};

struct SessionEntry {
    std::string type;
    std::string timestamp;
    std::vector<ContentBlock> content;
};

struct ParseError {
    int line_number;
    std::string raw_line;
};

struct Session {
    std::string title;
    std::vector<SessionEntry> entries;
    std::vector<ParseError> errors;
    SessionEntry& operator[](size_t i) { return entries[i]; }
    const SessionEntry& operator[](size_t i) const { return entries[i]; }
    size_t size() const { return entries.size(); }
};

inline std::string extract_entry_type(const json& entry) {
    if (entry.contains("type")) {
        return entry["type"].get<std::string>();
    }
    return entry["message"]["role"].get<std::string>();
}

inline std::string extract_block_text(const json& block) {
    std::string block_type = block.value("type", "");
    if (block_type == "thinking") {
        return block.value("thinking", "");
    }
    if (block_type == "tool_result" && block.contains("content") && block["content"].is_string()) {
        return block["content"].get<std::string>();
    }
    return block.value("text", "");
}

inline ContentBlock parse_content_block(const json& block) {
    ContentBlock result;
    result.type = block.value("type", "");
    result.text = extract_block_text(block);
    result.tool_name = block.value("name", "");
    result.tool_input = block.contains("input") ? block["input"].dump() : "";
    result.tool_use_id = block.value("tool_use_id", block.value("id", ""));
    result.is_error = block.value("is_error", false);
    return result;
}

inline std::vector<ContentBlock> extract_content(const json& entry) {
    std::vector<ContentBlock> blocks;
    if (entry.contains("message") && entry["message"].contains("content")) {
        for (const auto& block : entry["message"]["content"]) {
            blocks.push_back(parse_content_block(block));
        }
    }
    return blocks;
}

inline SessionEntry parse_entry(const json& parsed) {
    SessionEntry entry;
    entry.type = extract_entry_type(parsed);
    entry.timestamp = parsed.value("timestamp", "");
    entry.content = extract_content(parsed);
    return entry;
}

inline SessionEntry parse_jsonl_line(const std::string& line) {
    return parse_entry(json::parse(line));
}

inline Session parse_session(const std::string& jsonl) {
    Session session;
    std::istringstream stream(jsonl);
    std::string line;
    int line_number = 0;
    while (std::getline(stream, line)) {
        ++line_number;
        if (line.empty()) continue;
        auto parsed = json::parse(line, nullptr, false);
        if (parsed.is_discarded()) {
            session.errors.push_back({line_number, line});
            continue;
        }
        std::string type = parsed.value("type", "");
        if (type == "ai-title") {
            session.title = parsed.value("aiTitle", "");
        }
        session.entries.push_back(parse_entry(parsed));
    }
    return session;
}
