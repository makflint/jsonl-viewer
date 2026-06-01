#pragma once
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include "../third_party/json.hpp"
#include "schema.hpp"

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
    int line_number = 0;
};

struct ParseError {
    int line_number;
    std::string raw_line;
};

struct Session {
    std::string title;
    std::vector<SessionEntry> entries;
    std::vector<ParseError> errors;
    std::optional<RawSchema> raw_schema;
    SessionEntry& operator[](size_t i) { return entries[i]; }
    const SessionEntry& operator[](size_t i) const { return entries[i]; }
    size_t size() const { return entries.size(); }
};

[[nodiscard]] inline std::expected<json, ParseError> line_to_json(std::string_view line, int line_number) {
    auto parsed = json::parse(line, nullptr, false);
    if (parsed.is_discarded()) {
        return std::unexpected(ParseError{.line_number = line_number, .raw_line = std::string{line}});
    }
    return parsed;
}

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

[[nodiscard]] inline bool is_unmatched_record(const std::string& type, const std::vector<ContentBlock>& content) {
    return type == "unknown" && content.empty();
}

[[nodiscard]] inline ContentBlock make_raw_block(const json& entry) {
    return ContentBlock{.type = "raw", .text = entry.dump(2)};
}

[[nodiscard]] inline SessionEntry parse_entry(const json& parsed) {
    auto type = extract_entry_type(parsed);
    auto timestamp = extract_timestamp(parsed).value_or("");
    auto content = extract_content(parsed);
    if (is_unmatched_record(type, content)) {
        type = "raw";
        content.push_back(make_raw_block(parsed));
    }
    return SessionEntry{.type = type, .timestamp = timestamp, .content = content};
}

[[nodiscard]] inline SessionEntry parse_jsonl_line(std::string_view line) {
    return parse_entry(json::parse(line));
}

[[nodiscard]] inline std::optional<Session> try_parse_json(const std::string& input) {
    auto parsed = json::parse(input, nullptr, false);
    if (parsed.is_discarded()) return std::nullopt;

    Session session;
    std::vector<json> raw_entries_json;
    auto process = [&](const json& item, int index) {
        auto type = item.value("type", std::string{});
        if (type == "ai-title") {
            session.title = item.value("aiTitle", "");
            return;
        }
        auto entry = parse_entry(item);
        entry.line_number = index;
        if (entry.type == "raw") raw_entries_json.push_back(item);
        session.entries.push_back(std::move(entry));
    };

    if (parsed.is_array()) {
        int index = 0;
        for (const auto& item : parsed) process(item, ++index);
    } else {
        process(parsed, 1);
    }

    if (!raw_entries_json.empty())
        session.raw_schema = analyze_raw_schema(raw_entries_json);
    return session;
}

[[nodiscard]] inline Session parse_session(const std::string& jsonl) {
    if (auto json_session = try_parse_json(jsonl))
        return *json_session;

    Session session;
    std::vector<json> raw_entries_json;
    std::istringstream stream(jsonl);
    std::string line;
    int line_number = 0;
    while (std::getline(stream, line)) {
        ++line_number;
        if (line.empty()) continue;
        auto parsed = line_to_json(line, line_number);
        if (!parsed) {
            session.errors.push_back(parsed.error());
            continue;
        }
        auto type = parsed->value("type", std::string{});
        if (type == "ai-title") {
            session.title = parsed->value("aiTitle", "");
            continue;
        }
        auto entry = parse_entry(*parsed);
        entry.line_number = line_number;
        if (entry.type == "raw") {
            raw_entries_json.push_back(*parsed);
        }
        session.entries.push_back(std::move(entry));
    }
    if (!raw_entries_json.empty()) {
        session.raw_schema = analyze_raw_schema(raw_entries_json);
    }
    return session;
}

// WASM binding helpers: std::optional<RawSchema> on Session cannot be bound
// directly via value_object .field(); expose as free functions instead.
inline bool      has_raw_schema(const Session& s) { return s.raw_schema.has_value(); }
inline RawSchema get_raw_schema(const Session& s) { return s.raw_schema.value_or(RawSchema{}); }
