#include "../third_party/catch_amalgamated.hpp"
#include "../src/parser.hpp"

TEST_CASE("Parse single JSONL line extracts type") {
    std::string line = R"({"type":"user","message":{"role":"user","content":[{"type":"text","text":"hello"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "user");
}

TEST_CASE("Parse assistant message without top-level type falls back to message.role") {
    std::string line = R"({"parentUuid":"abc","message":{"role":"assistant","content":[{"type":"text","text":"hi"}]}})";

    auto entry = parse_jsonl_line(line);

    REQUIRE(entry.type == "assistant");
}
