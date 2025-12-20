#include <cson.h>
#include <stdio.h>

using namespace cson;


void parseFile() {
    auto context = Parser::parseFromFile("example1.json");
    
    auto& obj = context.object();
    for (auto& t : obj) {
        printf("%s: ", t.key().c_str());
        if (t->isString()) {
            printf("[string] %s\n", t->stringValue().c_str());
        } else if (t->isNumber()) {
            printf("[number] %f\n", t->floatValue());
        } else if (t->isBoolean()) {
            printf("[bool] %s\n", t->boolValue() ? "true":"false");
        } else if (t->isNull()) {
            printf("[null]\n");
        } else if (t->isArray()) {
            printf("[arr]");
            for (auto a : t->array()) {
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
