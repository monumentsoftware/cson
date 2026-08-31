#include <cson.h>
#include <stdio.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

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

// Looking up a key that does not exist used to dereference a null pointer.
TEST(CsonTests, testMissingKeyThrows) {
    auto json = JSON::fromString(JSON_TYPES);

    const auto& constObj = json.object();
    EXPECT_THROW(constObj["notthere"], NoSuchKey);
    EXPECT_EQ(constObj.entityForKey("notthere"), nullptr);

    auto& obj = json.object();
    EXPECT_THROW(obj["notthere"], NoSuchKey);

    // The key that was missing is part of the message.
    try {
        constObj["notthere"];
        FAIL() << "no exception thrown";
    } catch (const NoSuchKey& e) {
        EXPECT_NE(e.message().find("notthere"), std::string::npos) << e.message();
    }

    // Existing keys are unaffected, including ones holding null.
    EXPECT_EQ(constObj["string1"].stringValue(), "Hello");
    EXPECT_EQ(obj["string1"].stringValue(), "Hello");
    EXPECT_TRUE(constObj["null"].isNull());
    EXPECT_TRUE(obj["null"].isNull());

    // An empty object has no keys at all.
    Object empty;
    EXPECT_THROW(empty["anything"], NoSuchKey);
    const Object& constEmpty = empty;
    EXPECT_THROW(constEmpty["anything"], NoSuchKey);

    // Key lookup on a non-object is a type error, not a missing key.
    EXPECT_THROW(constObj["array"]["notthere"], InvalidType);
    EXPECT_THROW(obj["array"]["notthere"], InvalidType);
    EXPECT_THROW(constObj["string1"]["notthere"], InvalidType);
}

// Index based access used to read past the end of the underlying vector.
TEST(CsonTests, testArrayIndexOutOfBoundsThrows) {
    Array arr;
    arr.addString("a");
    arr.addString("b");

    const Array& constArr = arr;

    EXPECT_EQ(arr.entityAtIndex(1).stringValue(), "b");
    EXPECT_EQ(constArr.entityAtIndex(1).stringValue(), "b");

    EXPECT_THROW(arr.entityAtIndex(2), OutOfBounds);
    EXPECT_THROW(constArr.entityAtIndex(2), OutOfBounds);
    EXPECT_THROW(arr.entityAtIndex(static_cast<size_t>(-1)), OutOfBounds);
    EXPECT_THROW(constArr.entityAtIndex(static_cast<size_t>(-1)), OutOfBounds);

    // ... including through operator[].
    EXPECT_THROW(arr[2], OutOfBounds);
    EXPECT_THROW(constArr[2], OutOfBounds);

    Array empty;
    const Array& constEmpty = empty;
    EXPECT_THROW(empty.entityAtIndex(0), OutOfBounds);
    EXPECT_THROW(constEmpty.entityAtIndex(0), OutOfBounds);
    EXPECT_THROW(empty[0], OutOfBounds);
    EXPECT_THROW(constEmpty[0], OutOfBounds);
}

TEST(CsonTests, testObjectIndexOutOfBoundsThrows) {
    Object obj;
    obj.addString("first", "a");
    obj.addString("second", "b");

    const Object& constObj = obj;

    EXPECT_EQ(obj.entityAtIndex(1).stringValue(), "b");
    EXPECT_EQ(constObj.entityAtIndex(1).stringValue(), "b");
    EXPECT_EQ(constObj.keyByIndex(1), "second");

    EXPECT_THROW(obj.entityAtIndex(2), OutOfBounds);
    EXPECT_THROW(constObj.entityAtIndex(2), OutOfBounds);
    EXPECT_THROW(obj.entityAtIndex(static_cast<size_t>(-1)), OutOfBounds);
    EXPECT_THROW(constObj.keyByIndex(2), OutOfBounds);
    EXPECT_THROW(constObj.keyByIndex(static_cast<size_t>(-1)), OutOfBounds);

    // ... including through operator[] and the Entity level accessors.
    EXPECT_THROW(obj[2], OutOfBounds);
    EXPECT_THROW(constObj[2], OutOfBounds);
    const Entity& entity = constObj;
    EXPECT_THROW(entity.keyByIndex(2), OutOfBounds);

    // remove() shrinks the object, so what was a valid index before is not anymore.
    ASSERT_TRUE(obj.remove("second"));
    ASSERT_EQ(obj.count(), static_cast<size_t>(1));
    EXPECT_THROW(obj.entityAtIndex(1), OutOfBounds);
    EXPECT_THROW(constObj.keyByIndex(1), OutOfBounds);

    Object empty;
    const Object& constEmpty = empty;
    EXPECT_THROW(empty.entityAtIndex(0), OutOfBounds);
    EXPECT_THROW(constEmpty.entityAtIndex(0), OutOfBounds);
    EXPECT_THROW(constEmpty.keyByIndex(0), OutOfBounds);
    EXPECT_THROW(empty[0], OutOfBounds);
    EXPECT_THROW(constEmpty[0], OutOfBounds);
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

namespace {

struct IntRangeCase {
    const char* name;
    int64_t value;
};

// The boundaries of one integer type, plus the values right next to them.
#define CSON_INT_RANGE_CASES(type)                                                    \
    { #type " min",     static_cast<int64_t>(std::numeric_limits<type>::min()) },     \
    { #type " min + 1", static_cast<int64_t>(std::numeric_limits<type>::min()) + 1 }, \
    { #type " max - 1", static_cast<int64_t>(std::numeric_limits<type>::max()) - 1 }, \
    { #type " max",     static_cast<int64_t>(std::numeric_limits<type>::max()) }

// Every integer type that fits into the int64_t based setInt()/addInt() api. uint64_t is
// not among them, its upper half does not fit into an int64_t.
const IntRangeCase INT_RANGE_CASES[] = {
    { "zero", 0 },
    { "one", 1 },
    { "minus one", -1 },
    CSON_INT_RANGE_CASES(uint8_t),
    CSON_INT_RANGE_CASES(int8_t),
    CSON_INT_RANGE_CASES(uint16_t),
    CSON_INT_RANGE_CASES(int16_t),
    CSON_INT_RANGE_CASES(uint32_t),
    CSON_INT_RANGE_CASES(int32_t),
    CSON_INT_RANGE_CASES(int64_t),
};

#undef CSON_INT_RANGE_CASES

// Serializing and re-parsing makes both the writer and the parser see every value.
JSON roundTrip(const Entity& entity, bool prettyPrint) {
    return Parser::parseString(entity.toString(prettyPrint));
}

} // namespace

// Number::setInt() is what Object::addInt(), Object::setInt() and Array::addInt() all
// write through.
TEST(CsonTests, testNumberSetIntRanges) {
    for (const auto& testCase : INT_RANGE_CASES) {
        const std::string expected = std::to_string(testCase.value);

        Number number;
        number.setInt(testCase.value);
        EXPECT_EQ(number.value(), expected) << testCase.name;
        EXPECT_EQ(number.valueInt(), testCase.value) << testCase.name;
        EXPECT_EQ(number.toString(false), expected) << testCase.name;
        EXPECT_EQ(number.toString(true), expected) << testCase.name;

        // A bare number is a valid json document on its own.
        JSON json;
        ASSERT_NO_THROW({ json = roundTrip(number, false); }) << testCase.name;
        EXPECT_TRUE(json.root().isNumber()) << testCase.name;
        EXPECT_EQ(json.root().number().value(), expected) << testCase.name;
        EXPECT_EQ(json.root().number().valueInt(), testCase.value) << testCase.name;
        EXPECT_EQ(json.root().intValue(), testCase.value) << testCase.name;

        // Overwriting a number keeps no trace of the previous value.
        Number overwritten;
        overwritten.setInt(std::numeric_limits<int64_t>::min());
        overwritten.setInt(testCase.value);
        EXPECT_EQ(overwritten.value(), expected) << testCase.name;
        EXPECT_EQ(overwritten.valueInt(), testCase.value) << testCase.name;
    }
}

TEST(CsonTests, testObjectAddIntRanges) {
    Object obj;
    for (const auto& testCase : INT_RANGE_CASES) {
        Number& number = obj.addInt(testCase.name, testCase.value);
        EXPECT_EQ(number.value(), std::to_string(testCase.value)) << testCase.name;
        EXPECT_EQ(number.valueInt(), testCase.value) << testCase.name;

        EXPECT_EQ(obj.intValueForKey(testCase.name), testCase.value) << testCase.name;
        EXPECT_EQ(obj[testCase.name].intValue(), testCase.value) << testCase.name;
    }

    for (const bool prettyPrint : { false, true }) {
        JSON json;
        ASSERT_NO_THROW({ json = roundTrip(obj, prettyPrint); }) << "prettyPrint: " << prettyPrint;

        const auto& parsed = json.object();
        ASSERT_EQ(parsed.count(), sizeof(INT_RANGE_CASES) / sizeof(INT_RANGE_CASES[0]));
        for (const auto& testCase : INT_RANGE_CASES) {
            ASSERT_TRUE(parsed.contains(testCase.name)) << testCase.name;
            EXPECT_EQ(parsed.intValueForKey(testCase.name), testCase.value) << testCase.name;
            EXPECT_EQ(parsed[testCase.name].number().value(), std::to_string(testCase.value)) << testCase.name;
        }
    }
}

TEST(CsonTests, testObjectSetIntRanges) {
    // setInt() has three paths: adding a new key, overwriting an existing number and
    // replacing an entity of a different type. All of them have to cover the full range.
    Object added;
    Object overwritten;
    Object replaced;

    for (const auto& testCase : INT_RANGE_CASES) {
        overwritten.addInt(testCase.name, 0);
        replaced.addString(testCase.name, "not a number");

        Number& addedNumber = added.setInt(testCase.name, testCase.value);
        Number& overwrittenNumber = overwritten.setInt(testCase.name, testCase.value);
        Number& replacedNumber = replaced.setInt(testCase.name, testCase.value);

        const std::string expected = std::to_string(testCase.value);
        EXPECT_EQ(addedNumber.value(), expected) << testCase.name;
        EXPECT_EQ(overwrittenNumber.value(), expected) << testCase.name;
        EXPECT_EQ(replacedNumber.value(), expected) << testCase.name;

        EXPECT_EQ(added.intValueForKey(testCase.name), testCase.value) << testCase.name;
        EXPECT_EQ(overwritten.intValueForKey(testCase.name), testCase.value) << testCase.name;
        EXPECT_EQ(replaced.intValueForKey(testCase.name), testCase.value) << testCase.name;
        EXPECT_TRUE(replaced[testCase.name].isNumber()) << testCase.name;
    }

    const Object* objects[] = { &added, &overwritten, &replaced };
    for (const auto* obj : objects) {
        JSON json;
        ASSERT_NO_THROW({ json = roundTrip(*obj, false); });

        const auto& parsed = json.object();
        ASSERT_EQ(parsed.count(), sizeof(INT_RANGE_CASES) / sizeof(INT_RANGE_CASES[0]));
        for (const auto& testCase : INT_RANGE_CASES) {
            EXPECT_EQ(parsed.intValueForKey(testCase.name), testCase.value) << testCase.name;
        }
    }
}

TEST(CsonTests, testArrayAddIntRanges) {
    Array arr;
    size_t index = 0;
    for (const auto& testCase : INT_RANGE_CASES) {
        Number& number = arr.addInt(testCase.value);
        EXPECT_EQ(number.value(), std::to_string(testCase.value)) << testCase.name;
        EXPECT_EQ(number.valueInt(), testCase.value) << testCase.name;

        EXPECT_EQ(arr.intValueAtIndex(index), testCase.value) << testCase.name;
        EXPECT_EQ(arr.numberAtIndex(index).valueInt(), testCase.value) << testCase.name;
        EXPECT_EQ(arr[index].intValue(), testCase.value) << testCase.name;
        index++;
    }

    for (const bool prettyPrint : { false, true }) {
        JSON json;
        ASSERT_NO_THROW({ json = roundTrip(arr, prettyPrint); }) << "prettyPrint: " << prettyPrint;

        const auto& parsed = json.array();
        ASSERT_EQ(parsed.count(), sizeof(INT_RANGE_CASES) / sizeof(INT_RANGE_CASES[0]));
        index = 0;
        for (const auto& testCase : INT_RANGE_CASES) {
            EXPECT_EQ(parsed.intValueAtIndex(index), testCase.value) << testCase.name;
            EXPECT_EQ(parsed.numberAtIndex(index).value(), std::to_string(testCase.value)) << testCase.name;
            index++;
        }
    }
}

namespace {

// A document using every container nesting that the pretty printer has to indent:
// objects and arrays at the top level, inside an array and inside an object.
void buildPrettyPrintDocument(Object& root) {
    root.addString("name", "cson");
    root.addInt("version", 1);

    Array& items = root.addArray("items");
    Object& first = items.addObject();
    first.addInt("id", 1);
    Array& tags = first.addArray("tags");
    tags.addString("a");
    tags.addString("b");
    items.addInt(2);
    Array& innerArray = items.addArray();
    innerArray.addObject().addBoolean("nested", true);

    Object& nested = root.addObject("nested");
    nested.addBoolean("flag", false);
    nested.addNull("nothing");
}

// Every line of the expected output, so a failure points at the line that differs.
void expectLines(const std::string& actual, const std::vector<std::string>& expectedLines) {
    std::vector<std::string> actualLines;
    std::string line;
    for (const char c : actual) {
        if (c == '\n') {
            actualLines.push_back(line);
            line.clear();
        } else {
            line += c;
        }
    }
    actualLines.push_back(line);

    EXPECT_EQ(actualLines.size(), expectedLines.size()) << "full output:\n" << actual;
    for (size_t i = 0; i < std::min(actualLines.size(), expectedLines.size()); i++) {
        EXPECT_EQ(actualLines[i], expectedLines[i]) << "line " << i << "\nfull output:\n" << actual;
    }
}

} // namespace

// Both containers open on the line they start on. An object used to emit a newline and its
// own indentation before the opening brace, which left an empty line behind whenever the
// caller had already written indentation for it - an object inside an array for example.
TEST(CsonTests, testPrettyPrintNestedContainers) {
    Object root;
    buildPrettyPrintDocument(root);

    expectLines(root.toString(true), {
        "{",
        "  \"name\":\"cson\",",
        "  \"version\":1,",
        "  \"items\":[",
        "    {",
        "      \"id\":1,",
        "      \"tags\":[",
        "        \"a\",",
        "        \"b\"",
        "      ]",
        "    },",
        "    2,",
        "    [",
        "      {",
        "        \"nested\":true",
        "      }",
        "    ]",
        "  ],",
        "  \"nested\":{",
        "    \"flag\":false,",
        "    \"nothing\":null",
        "  }",
        "}",
    });
}

// The empty lines the old object output produced are gone, and no line is whitespace only.
TEST(CsonTests, testPrettyPrintHasNoEmptyLines) {
    Object root;
    buildPrettyPrintDocument(root);

    const std::string pretty = root.toString(true);
    EXPECT_EQ(pretty.find("\n\n"), std::string::npos) << pretty;

    size_t lineStart = 0;
    while (lineStart <= pretty.size()) {
        const size_t lineEnd = std::min(pretty.find('\n', lineStart), pretty.size());
        const std::string line = pretty.substr(lineStart, lineEnd - lineStart);
        EXPECT_NE(line.find_first_not_of(" \t"), std::string::npos)
            << "whitespace only line at offset " << lineStart << "\n" << pretty;
        lineStart = lineEnd + 1;
    }
}

// An array of objects is the case the empty lines were most visible in.
TEST(CsonTests, testPrettyPrintArrayOfObjects) {
    Array arr;
    arr.addObject().addInt("a", 1);
    arr.addObject().addInt("b", 2);

    EXPECT_EQ(arr.toString(true),
              "[\n"
              "  {\n"
              "    \"a\":1\n"
              "  },\n"
              "  {\n"
              "    \"b\":2\n"
              "  }\n"
              "]");
}

// Scalars ignore prettyPrint entirely, they never span more than one line.
TEST(CsonTests, testPrettyPrintScalars) {
    Number number;
    number.setInt(42);
    EXPECT_EQ(number.toString(true), "42");

    String str;
    str.setString("a\"b\nc");
    EXPECT_EQ(str.toString(true), "\"a\\\"b\\nc\"");

    Boolean boolean;
    boolean.setBool(true);
    EXPECT_EQ(boolean.toString(true), "true");

    Null null;
    EXPECT_EQ(null.toString(true), "null");
}

// Empty containers still open and close on separate lines, and both do it the same way.
TEST(CsonTests, testPrettyPrintEmptyContainers) {
    Object emptyObject;
    EXPECT_EQ(emptyObject.toString(true), "{\n}");

    Array emptyArray;
    EXPECT_EQ(emptyArray.toString(true), "[\n]");

    Object root;
    root.addObject("object");
    root.addArray("array");
    EXPECT_EQ(root.toString(true),
              "{\n"
              "  \"object\":{\n"
              "  },\n"
              "  \"array\":[\n"
              "  ]\n"
              "}");
}

// The indentation string is used verbatim, and level shifts everything but the opening
// brace, which the caller is responsible for placing.
TEST(CsonTests, testPrettyPrintIndentation) {
    Object root;
    root.addInt("a", 1);
    root.addObject("b").addInt("c", 2);

    EXPECT_EQ(root.toString(true, "    "),
              "{\n"
              "    \"a\":1,\n"
              "    \"b\":{\n"
              "        \"c\":2\n"
              "    }\n"
              "}");

    EXPECT_EQ(root.toString(true, "\t"),
              "{\n"
              "\t\"a\":1,\n"
              "\t\"b\":{\n"
              "\t\t\"c\":2\n"
              "\t}\n"
              "}");

    EXPECT_EQ(root.toString(true, "  ", 1),
              "{\n"
              "    \"a\":1,\n"
              "    \"b\":{\n"
              "      \"c\":2\n"
              "    }\n"
              "  }");
}

// Comments are dropped when writing compact json, but kept on their own line when pretty
// printing.
TEST(CsonTests, testPrettyPrintComments) {
    Parser parser;
    parser.allowComments(true);
    const auto json = parser.parse("{\n// leading\n\"a\":1, // trailing\n\"arr\":[1, // in array\n2]\n}");

    EXPECT_EQ(json.root().toString(true),
              "{\n"
              "  // leading\n"
              "  \"a\":1,\n"
              "  // trailing\n"
              "  \"arr\":[\n"
              "    1,\n"
              "    // in array\n"
              "    2\n"
              "  ]\n"
              "}");

    EXPECT_EQ(json.root().toString(false), "{\"a\":1,\"arr\":[1,2]}");
}

// Whatever the pretty printer emits has to parse back into the same document, so the
// indentation and the newlines never end up inside a value.
TEST(CsonTests, testPrettyPrintRoundTrip) {
    Object root;
    buildPrettyPrintDocument(root);
    // Keys and values that need escaping have to survive the trip as well.
    root.addString("escaped\t key", "line\nbreak \"quoted\" \\ backslash");

    for (const std::string& indentation : { std::string("  "), std::string("    "), std::string("\t") }) {
        JSON json;
        ASSERT_NO_THROW({ json = Parser::parseString(root.toString(true, indentation)); }) << indentation;
        EXPECT_EQ(json.root().toString(false), root.toString(false)) << indentation;
    }
}

// The JSON convenience wrapper maps its options onto the same pretty printer.
TEST(CsonTests, testJsonToStringOptions) {
    auto json = JSON::fromString("{\"a\":[{\"b\":1}]}");
    const Entity& root = json.root();

    EXPECT_EQ(json.toString(root), "{\"a\":[{\"b\":1}]}");

    EXPECT_EQ(json.toString(root, { JSON::Option::prettyPrint }),
              "{\n"
              "  \"a\":[\n"
              "    {\n"
              "      \"b\":1\n"
              "    }\n"
              "  ]\n"
              "}");

    EXPECT_EQ(json.toString(root, { JSON::Option::prettyPrint, JSON::Option::indent4Spaces }),
              "{\n"
              "    \"a\":[\n"
              "        {\n"
              "            \"b\":1\n"
              "        }\n"
              "    ]\n"
              "}");

    EXPECT_EQ(json.toString(root, { JSON::Option::prettyPrint, JSON::Option::indentTab }),
              "{\n"
              "\t\"a\":[\n"
              "\t\t{\n"
              "\t\t\t\"b\":1\n"
              "\t\t}\n"
              "\t]\n"
              "}");
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
