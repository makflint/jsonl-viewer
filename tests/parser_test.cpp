#include "../third_party/catch_amalgamated.hpp"
#include "../src/parser.hpp"

TEST_CASE("Parse single JSONL line extracts type") {
    std::string line = R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "user");
}

TEST_CASE("Parse user message extracts text content") {
    std::string line = R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello world"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.content.size() == 1);
    REQUIRE(entry.content[0].type == "text");
    REQUIRE(entry.content[0].text == "hello world");
}

TEST_CASE("Parse assistant message with thinking block") {
    std::string line = R"({"message":{"role":"assistant","content":[{"type":"thinking","thinking":"let me think..."},{"type":"text","text":"here is my answer"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.content.size() == 2);
    REQUIRE(entry.content[0].type == "thinking");
    REQUIRE(entry.content[0].text == "let me think...");
    REQUIRE(entry.content[1].type == "text");
    REQUIRE(entry.content[1].text == "here is my answer");
}

TEST_CASE("Parse tool_use block extracts name and input") {
    std::string line = R"({"message":{"role":"assistant","content":[{"type":"tool_use","id":"toolu_123","name":"Bash","input":{"command":"ls","description":"list files"}}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.content.size() == 1);
    REQUIRE(entry.content[0].type == "tool_use");
    REQUIRE(entry.content[0].tool_name == "Bash");
    REQUIRE(entry.content[0].tool_input == "{\"command\":\"ls\",\"description\":\"list files\"}");
    REQUIRE(entry.content[0].tool_use_id == "toolu_123");
}

TEST_CASE("Parse tool_result block extracts content and tool_use_id") {
    std::string line = R"({"type":"user","message":{"role":"user","content":[{"type":"tool_result","tool_use_id":"toolu_123","content":"file1.txt\nfile2.txt","is_error":false}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.content.size() == 1);
    REQUIRE(entry.content[0].type == "tool_result");
    REQUIRE(entry.content[0].text == "file1.txt\nfile2.txt");
    REQUIRE(entry.content[0].tool_use_id == "toolu_123");
    REQUIRE(entry.content[0].is_error == false);
}

TEST_CASE("Parse multiple JSONL lines into session entries") {
    std::string jsonl =
        R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})" "\n"
        R"({"message":{"role":"assistant","content":[{"type":"text","text":"hi there"}]}})";

    auto entries = parse_session(jsonl);

    REQUIRE(entries.size() == 2);
    REQUIRE(entries[0].type == "user");
    REQUIRE(entries[0].content[0].text == "hello");
    REQUIRE(entries[1].type == "assistant");
    REQUIRE(entries[1].content[0].text == "hi there");
}

TEST_CASE("Parse error tool_result has is_error true") {
    std::string line = R"({"type":"user","message":{"role":"user","content":[{"type":"tool_result","tool_use_id":"toolu_456","content":"command not found","is_error":true}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.content[0].is_error == true);
    REQUIRE(entry.content[0].text == "command not found");
}

TEST_CASE("Parse entry extracts timestamp") {
    std::string line = R"({"type":"user","timestamp":"2026-03-25T06:20:14.840Z","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.timestamp == "2026-03-25T06:20:14.840Z");
}

TEST_CASE("Parse session title from ai-title entry") {
    std::string jsonl =
        R"({"type":"user","timestamp":"2026-03-25T06:20:15.000Z","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})" "\n"
        R"({"type":"ai-title","sessionId":"abc-123","aiTitle":"Create subdirectories"})";

    auto session = parse_session(jsonl);

    REQUIRE(session.title == "Create subdirectories");
}

TEST_CASE("Parse session keeps all entries including metadata") {
    std::string jsonl =
        R"({"type":"queue-operation","operation":"enqueue","timestamp":"2026-03-25T06:20:14.840Z"})" "\n"
        R"({"type":"user","timestamp":"2026-03-25T06:20:15.000Z","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})" "\n"
        R"({"type":"file-history-snapshot","messageId":"abc","snapshot":{}})" "\n"
        R"({"message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto session = parse_session(jsonl);

    REQUIRE(session.size() == 4);
    REQUIRE(session[0].type == "queue-operation");
    REQUIRE(session[0].content.empty());
    REQUIRE(session[1].type == "user");
    REQUIRE(session[2].type == "file-history-snapshot");
    REQUIRE(session[3].type == "assistant");
}

TEST_CASE("Parse session skips empty lines") {
    std::string jsonl =
        R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})" "\n"
        "\n"
        R"({"message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto session = parse_session(jsonl);

    REQUIRE(session.size() == 2);
}

TEST_CASE("Parse session handles malformed line gracefully") {
    std::string jsonl =
        R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})" "\n"
        "not valid json" "\n"
        R"({"message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto session = parse_session(jsonl);

    REQUIRE(session.size() == 2);
    REQUIRE(session.errors.size() == 1);
    REQUIRE(session.errors[0].line_number == 2);
    REQUIRE(session.errors[0].raw_line == "not valid json");
}

TEST_CASE("Parse assistant message without top-level type falls back to message.role") {
    std::string line = R"({"parentUuid":"abc","message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "assistant");
}

TEST_CASE("Parse entry with string content does not throw") {
    std::string line = R"({"type":"user","message":{"role":"user","content":"some plain text"}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.content.empty());
}
