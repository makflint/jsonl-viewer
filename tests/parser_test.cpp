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

TEST_CASE("Parse assistant message without top-level type falls back to message.role") {
    std::string line = R"({"parentUuid":"abc","message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "assistant");
}
