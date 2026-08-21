#include <cson.h>
#include <stdio.h>
#include <gtest/gtest.h>

class DepthTests : public testing::TestWithParam<std::tuple<std::string, size_t>>
{
};

const char* JSON_TYPES = R"JSON(
{
    "string1": "Hello",
    "string2": "",
    "string3": "\"Hello\"",
    "string4": "\\\/\b\f\n\r\t",
    "string5": "\u0048ello",
    "string6": "\u0394",
    "string7": "\u263A",
    "string8": "a\u0001b",

    "num1": 1,
    "num2": 1.5,
    "num3": 0.5,
    "num4": -1,
    "num5": 1.1e2,
    "num6": 1.1e+2,
    "num7": 5e-1,

    "bool1": true,
    "bool2": false,

    "null": null,

    "array": [
        "a",
        "b",
        "c"
    ]
}
)JSON";

const char* JSON_ARRAY_DEPTH = R"JSON(
    [[[[[[[[[[]]]]]]]]]]
)JSON";

const char* JSON_OBJECT_DEPTH = R"JSON(
    {"1":{"2":{"3":{"4":{"5":{"6":{"7":{"8":{"9":{}}}}}}}}}}
)JSON";

const char* JSON_MIXED_DEPTH = R"JSON(
    [{"1":[{"2":[{"3":[{"4":[{}]}]}]}]}]
)JSON";


using namespace cson;


TEST(CsonTests, testTypes) {
    const auto json = JSON::fromString(JSON_TYPES);

    const auto& obj = json.object();
    EXPECT_EQ(obj["string1"].stringValue(), "Hello");
    EXPECT_TRUE(obj["string2"].stringValue().empty());
    EXPECT_EQ(obj["string3"].stringValue(), "\"Hello\"");
    EXPECT_EQ(obj["string4"].stringValue(), "\\/\b\f\n\r\t");
    EXPECT_EQ(obj["string5"].stringValue(), "Hello");
    const auto str6 = obj["string6"].stringValue();
    const auto* data = reinterpret_cast<const uint8_t*>(str6.c_str());
    EXPECT_EQ(data[0], 0xce);
    EXPECT_EQ(data[1], 0x94);

    const auto str7 = obj["string7"].stringValue();
    const auto* data2 = reinterpret_cast<const uint8_t*>(str7.c_str());
    EXPECT_EQ(data2[0], 0xe2);
    EXPECT_EQ(data2[1], 0x98);
    EXPECT_EQ(data2[2], 0xba);

    EXPECT_EQ(obj["string8"].stringValue(), std::string("a") + static_cast<char>(0x01) + "b");

    EXPECT_EQ(obj["num1"].intValue(), 1);
    EXPECT_EQ(obj["num1"].floatValue(), 1);
    EXPECT_EQ(obj["num1"].doubleValue(), 1);
    EXPECT_EQ(obj["num2"].floatValue(), 1.5);
    EXPECT_EQ(obj["num2"].doubleValue(), 1.5);
    EXPECT_EQ(obj["num3"].floatValue(), 0.5);
    EXPECT_EQ(obj["num3"].doubleValue(), 0.5);
    EXPECT_EQ(obj["num4"].intValue(), -1);
    EXPECT_EQ(obj["num4"].floatValue(), -1);
    EXPECT_EQ(obj["num4"].doubleValue(), -1);
    EXPECT_EQ(obj["num5"].floatValue(), 110);
    EXPECT_EQ(obj["num5"].doubleValue(), 110);
    EXPECT_EQ(obj["num6"].floatValue(), 110);
    EXPECT_EQ(obj["num6"].doubleValue(), 110);
    EXPECT_EQ(obj["num7"].floatValue(), 0.5);
    EXPECT_EQ(obj["num7"].doubleValue(), 0.5);

    EXPECT_EQ(obj["bool1"].boolValue(), true);
    EXPECT_EQ(obj["bool2"].boolValue(), false);

    EXPECT_TRUE(obj["null"].isNull());

    EXPECT_EQ(obj["array"].array().count(), 3);
    EXPECT_EQ(obj["array"].array()[0].stringValue(), "a");
    EXPECT_EQ(obj["array"].array()[1].stringValue(), "b");
    EXPECT_EQ(obj["array"].array()[2].stringValue(), "c");
}

// Control characters are not allowed to appear raw in a json string literal. The ones
// without a dedicated short escape sequence have to be written in the \u00xx form.
TEST(CsonTests, testEscapeControlCharacters) {
    struct EscapeCase {
        unsigned char input;
        const char* expected;
    };
    const EscapeCase cases[] = {
        // Control characters with a dedicated short escape sequence.
        { 0x08, "\"\\b\"" },
        { 0x09, "\"\\t\"" },
        { 0x0a, "\"\\n\"" },
        { 0x0c, "\"\\f\"" },
        { 0x0d, "\"\\r\"" },
        // Everything else below 0x20 uses \u00xx.
        { 0x00, "\"\\u0000\"" },
        { 0x01, "\"\\u0001\"" },
        { 0x0b, "\"\\u000b\"" },
        { 0x0e, "\"\\u000e\"" },
        { 0x1f, "\"\\u001f\"" },
    };

    for (const auto& escapeCase : cases) {
        String str;
        str.setString(std::string(1, static_cast<char>(escapeCase.input)));
        EXPECT_EQ(str.toString(false), escapeCase.expected)
            << "input: " << static_cast<int>(escapeCase.input);
    }

    // 0x7f is a control character in unicode, but json does not require escaping it.
    String del;
    del.setString(std::string(1, static_cast<char>(0x7f)));
    EXPECT_EQ(del.toString(false), std::string("\"") + static_cast<char>(0x7f) + "\"");
}

// Writing control characters raw used to produce json that this very parser rejects,
// so serializing and re-parsing has to survive every single one of them.
TEST(CsonTests, testControlCharacterRoundTrip) {
    const std::string controlKey = std::string("key") + static_cast<char>(0x01) + static_cast<char>(0x1f);

    Object obj;
    for (int i = 0; i < 0x20; i++) {
        obj.addString("value" + std::to_string(i)).setString(std::string(1, static_cast<char>(i)));
    }
    // Keys are escaped by the same code path as values.
    obj.addString(controlKey, "plain");

    const std::string serialized = obj.toString(false);
    for (const char c : serialized) {
        EXPECT_GE(static_cast<unsigned int>(static_cast<unsigned char>(c)), 0x20u)
            << "raw control character in serialized json";
    }

    JSON json;
    ASSERT_NO_THROW({ json = Parser::parseString(serialized); });

    const auto& parsed = json.object();
    ASSERT_EQ(parsed.count(), static_cast<size_t>(0x21));
    for (int i = 0; i < 0x20; i++) {
        const std::string expected(1, static_cast<char>(i));
        EXPECT_EQ(parsed.stringValueForKey("value" + std::to_string(i)), expected) << "value" << i;
    }
    EXPECT_EQ(parsed.stringValueForKey(controlKey), "plain");
}

// The counterpart of the escaping above: raw control characters are rejected on read.
TEST(CsonTests, testRawControlCharacterInStringIsRejected) {
    const std::string json = std::string("{\"key\":\"a") + static_cast<char>(0x01) + "b\"}";
    EXPECT_THROW(Parser::parseString(json), ParseError);
}

TEST(CsonTests, testEmpty) {
    const JSON json;
    EXPECT_FALSE(json.root().isObject());
    EXPECT_FALSE(json.root().isArray());
    EXPECT_FALSE(json.root().isNumber());
    EXPECT_FALSE(json.root().isString());
    EXPECT_FALSE(json.root().isBoolean());
    EXPECT_TRUE(json.root().isNull());
}

TEST(CsonTests, testIterators) {
    const auto json = JSON::fromString(JSON_TYPES);

    const auto& obj = json.object();
    std::string testString;
    for (const auto& it : obj) {
        if (it == "string1") {
            testString += "ok1";
        }
        if (it == "notthere") {
            testString += "NOTTHERE";
        }
        if (it == "string5") {
            testString += it->stringValue();
        }
    }
    EXPECT_EQ(testString, "ok1Hello");

    testString.clear();
    const auto& arr = json.object()["array"].array();
    for (const auto& entity : arr) {
        testString += entity.stringValue();
    }

    EXPECT_EQ(testString, "abc");
}

TEST_P(DepthTests, testDepth) {
    const auto& jsonString = std::get<0>(GetParam());
    const auto& maxValidDepth = std::get<1>(GetParam());
    Parser parser;
    parser.setMaxDepth(maxValidDepth);

    EXPECT_NO_THROW({
        parser.parse(jsonString);
    });
}

TEST_P(DepthTests, testDepthException) {
    const auto& jsonString = std::get<0>(GetParam());
    const auto& maxValidDepth = std::get<1>(GetParam());
    Parser parser;
    parser.setMaxDepth(maxValidDepth - 1);

    EXPECT_THROW({
        parser.parse(jsonString);
    }, TooManyNestings);
}

INSTANTIATE_TEST_SUITE_P(CsonTests, // suite name
                         DepthTests, // fixture class
                         testing::Values(
                             std::make_tuple(JSON_ARRAY_DEPTH, 10),
                             std::make_tuple(JSON_OBJECT_DEPTH, 10),
                             std::make_tuple(JSON_MIXED_DEPTH, 10)
                         )
);
