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


#define FAIL(test, expression) fail(test, #expression)

#define SUCCESS(test, expression) success(test, #expression)

#define TEST_TRUE(expression) (expression) ? SUCCESS("TEST_TRUE", expression) : FAIL("TEST_TRUE", expression)

#define RUN_TEST(func) \
    printf("running " #func "\n"); \
    try { \
        func; \
    } catch (const Exception& e) { \
        fail("catched unexpected exception", #func); \
    } \
    printf("finished without exception\n");


#define RUN_TEST_EXCEPT(func, exception) \
    { \
        bool ok = false; \
        try { \
            func; \
        } catch (const exception& ex) { \
            printf("finished with expected exception %s\n", #exception); \
            ok = true; \
        } catch (const Exception& ex) { \
        } \
        if (!ok) { \
            fail("exception not thrown", #func); \
        } \
    }

inline void fail(const char* test, const char* expression) {
    printf("\t[fail]: %s(%s)\n", test, expression);
    exit(1);
}

inline void success(const char* test, const char* expression) {
    printf("\t[success]: %s(%s)\n", test, expression);
}

void testTypes() {
    const auto json = JSON::fromString(JSON_TYPES);

    const auto& obj = json.object();
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
    const auto json = JSON::fromString(JSON_TYPES);

    const auto& obj = json.object();
    std::string testString;
    for (const auto it : obj) {
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
    const auto& arr = json.object()["array"].array();
    for (const auto entity : arr) {
        testString += entity->stringValue();
    }

    TEST_TRUE(testString == "abc");
}

void testDepth(const std::string& jsonString, size_t maxDepth) {
    Parser parser;
    parser.setMaxDepth(maxDepth);

    parser.parse(jsonString);
}


int main() {
    RUN_TEST(testTypes());
    RUN_TEST(testIterators());
    RUN_TEST(testDepth(JSON_ARRAY_DEPTH, 10));
    RUN_TEST(testDepth(JSON_OBJECT_DEPTH, 10));
    RUN_TEST(testDepth(JSON_MIXED_DEPTH, 10));
    RUN_TEST_EXCEPT(testDepth(JSON_ARRAY_DEPTH, 9), TooManyNestings);
    RUN_TEST_EXCEPT(testDepth(JSON_OBJECT_DEPTH, 9), TooManyNestings);
    RUN_TEST_EXCEPT(testDepth(JSON_MIXED_DEPTH, 9), TooManyNestings);
    return 0;
}
