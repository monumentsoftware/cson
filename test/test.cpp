#include <cson.h>
#include <stdio.h>


const char* JSON_TYPES = R"JSON(
{
    "string1": "Hello",
    "string2": "",
    "string3": "\"\Hello\"",
    "string4": "\\\/\b\f\n\r\t",
    "string5": "\u0048ello",

    "num1": 1,
    "num2": 1.5,
    "num3": 0.5,
    "num4": -1,
    "num5": 1.1e+2,
    "num6": 5e-1,

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

using namespace cson;


#define FAIL(test, expression) fail(test, #expression)

#define SUCCESS(test, expression) success(test, #expression)

#define TEST_TRUE(expression) (expression) ? SUCCESS("TEST_TRUE", expression) : FAIL("TEST_TRUE", expression)

inline void fail(const char* test, const char* expression) {
    printf("[fail]: %s(%s)\n", test, expression);
    exit(1);
}

inline void success(const char* test, const char* expression) {
    printf("[success]: %s(%s)\n", test, expression);
}

void testTypes() {
    auto context = Parser::parseString(JSON_TYPES);

    const auto& obj = context.object();
    TEST_TRUE(obj["string1"].stringValue() == "Hello");
    TEST_TRUE(obj["string2"].stringValue().empty());
    TEST_TRUE(obj["string3"].stringValue() == "\"Hello\"");
    TEST_TRUE(obj["string4"].stringValue() == "\\/\b\f\n\r\t");
    TEST_TRUE(obj["string5"].stringValue() == "Hello");
    
    TEST_TRUE(obj["num1"].intValue() == 1);
    TEST_TRUE(obj["num1"].floatValue() == 1);
    TEST_TRUE(obj["num1"].doubleValue() == 1);
    TEST_TRUE(obj["num2"].floatValue() == 1.5);
    TEST_TRUE(obj["num2"].doubleValue() == 1.5);
    TEST_TRUE(obj["num3"].floatValue() == 0.5);
    TEST_TRUE(obj["num3"].doubleValue() == 0.5);
    TEST_TRUE(obj["num4"].intValue() == -1);
    TEST_TRUE(obj["num4"].floatValue() == -1);
    TEST_TRUE(obj["num4"].doubleValue() == -1);
    TEST_TRUE(obj["num5"].floatValue() == 110);
    TEST_TRUE(obj["num5"].doubleValue() == 110);
    TEST_TRUE(obj["num6"].floatValue() == 0.5);
    TEST_TRUE(obj["num6"].doubleValue() == 0.5);

    TEST_TRUE(obj["bool1"].boolValue() == true);
    TEST_TRUE(obj["bool2"].boolValue() == false);

    TEST_TRUE(obj["null"].isNull());

    TEST_TRUE(obj["array"].array().count() == 3);
    TEST_TRUE(obj["array"].array()[0].stringValue() == "a");
    TEST_TRUE(obj["array"].array()[1].stringValue() == "b");
    TEST_TRUE(obj["array"].array()[2].stringValue() == "c");
}

void testIterators() {
    auto context = Parser::parseString(JSON_TYPES);

    const auto& obj = context.object();
    std::string testString;
    for (auto it : obj) {
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
    TEST_TRUE(testString == "ok1Hello");

    testString.clear();
    const auto& arr = context.object()["array"].array();
    for (auto entity : arr) {
        testString += entity->stringValue();
    }

    TEST_TRUE(testString == "abc");
}


int main() {
    try {    
        testTypes();
        testIterators();
    } catch (const cson::Exception& e) {
        printf("Error: %s\n", e.message().c_str());
        exit(1);
    }
    return 0;
}
