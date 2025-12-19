#include <cson.h>
#include <stdio.h>

using namespace cson;


void parseFile() {
    auto context = Parser::parseFromFile("example1.json");
    
    auto& obj = context.object();
    for (auto t : obj) {
        printf("%s: ", t.key().c_str());
        if (t.entity().isString()) {
            printf("[string] %s\n", t.entity().stringValue().c_str());
        } else if (t.entity().isNumber()) {
            printf("[number] %f\n", t.entity().floatValue());
        } else if (t.entity().isBoolean()) {
            printf("[bool] %s\n", t.entity().boolValue() ? "true":"false");
        } else if (t.entity().isNull()) {
            printf("[null]\n");
        } else if (t.entity().isArray()) {
            printf("[arr]");
            for (auto a : t.entity().array()) {
                printf(" %s;", a->stringValue().c_str());
            }
            printf("\n");
        } else {
            printf("unkown type\n");
        }
    }
}

int main() {
    
    try {
        parseFile();
    }
    catch (Exception ex) {
        printf("Caught: %s\n", ex.message().c_str());        
    }
    
    return 0;
}
