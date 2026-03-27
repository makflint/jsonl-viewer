#pragma once
#include <optional>
#include <string>
#include <string_view>
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

[[nodiscard]] inline std::optional<std::string> extract_timestamp(const json& entry) {
    if (!entry.contains("timestamp")) return std::nullopt;
    return entry["timestamp"].get<std::string>();
}

[[nodiscard]] inline std::optional<std::string> extract_tool_use_id(const json& block) {
    if (block.contains("tool_use_id")) return block["tool_use_id"].get<std::string>();
    if (block.contains("id")) return block["id"].get<std::string>();
    return std::nullopt;
}

[[nodiscard]] inline std::string extract_entry_type(const json& entry) {
    if (entry.contains("type")) {
        return entry["type"].get<std::string>();
    }
    if (entry.contains("message") && entry["message"].contains("role")) {
        return entry["message"]["role"].get<std::string>();
    }
    return "unknown";
}

[[nodiscard]] inline std::string extract_block_text(const json& block) {
    auto block_type = block.value("type", std::string{});
    if (block_type == "thinking") {
        return block.value("thinking", "");
    }
    if (block_type == "tool_result" && block.contains("content") && block["content"].is_string()) {
        return block["content"].get<std::string>();
    }
    return block.value("text", "");
}

[[nodiscard]] inline ContentBlock parse_content_block(const json& block) {
    return ContentBlock{
        .type      = block.value("type", ""),
        .text      = extract_block_text(block),
        .tool_name = block.value("name", ""),
        .tool_input   = block.contains("input") ? block["input"].dump() : "",
        .tool_use_id  = extract_tool_use_id(block).value_or(""),
        .is_error     = block.value("is_error", false),
    };
}

[[nodiscard]] inline std::vector<ContentBlock> extract_content(const json& entry) {
    std::vector<ContentBlock> blocks;
    if (!entry.contains("message")) return blocks;
    const auto& content = entry["message"]["content"];
    if (!content.is_array()) return blocks;
    for (const auto& block : content) {
        blocks.push_back(parse_content_block(block));
    }
    return blocks;
}

[[nodiscard]] inline SessionEntry parse_entry(const json& parsed) {
    return SessionEntry{
        .type      = extract_entry_type(parsed),
        .timestamp = extract_timestamp(parsed).value_or(""),
        .content   = extract_content(parsed),
    };
}

[[nodiscard]] inline SessionEntry parse_jsonl_line(std::string_view line) {
    return parse_entry(json::parse(line));
}

[[nodiscard]] inline Session parse_session(const std::string& jsonl) {
    Session session;
    std::istringstream stream(jsonl);
    std::string line;
    int line_number = 0;
    while (std::getline(stream, line)) {
        ++line_number;
        if (line.empty()) continue;
        auto parsed = json::parse(line, nullptr, false);
        if (parsed.is_discarded()) {
            session.errors.push_back({.line_number = line_number, .raw_line = line});
            continue;
        }
        auto type = parsed.value("type", std::string{});
        if (type == "ai-title") {
            session.title = parsed.value("aiTitle", "");
            continue;
        }
        session.entries.push_back(parse_entry(parsed));
    }
    return session;
}
