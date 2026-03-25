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

TEST_CASE("Parse assistant message without top-level type falls back to message.role") {
    std::string line = R"({"parentUuid":"abc","message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "assistant");
}
