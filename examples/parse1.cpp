#include <cson.h>
#include <stdio.h>

using namespace cson;


void parseFile() {
    auto json = Parser::parseFile("example1.json");
    
    auto& obj = json.object();
    for (auto& t : obj) {
        printf("%s: ", t.key().c_str());

        switch (t->type()) {
        case Entity::Type::string:
            printf("[string] %s\n", t->stringValue().c_str());
            break;
        case Entity::Type::number:
            printf("[number] %f\n", t->floatValue());
            break;
        case Entity::Type::boolean:
            printf("[bool] %s\n", t->boolValue() ? "true":"false");
            break;
        case Entity::Type::null:
            printf("[null]\n");
            break;
        case Entity::Type::array:
            printf("[arr]");
            for (auto a : t->array()) {
                printf(" %s;", a->stringValue().c_str());
            }
            printf("\n");
            break;
        default:
            printf("unkown type\n");
            break;
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
