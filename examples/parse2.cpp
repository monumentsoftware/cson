#include <cson.h>
#include <stdio.h>

void parseFile(const std::string& filename) {
    using namespace cson;

    try {
        const auto json = JSON::load(filename);
        const auto& rootObject = json.object();

        const auto& name = rootObject["name"].stringValue();
        const auto& lastName = rootObject["lastname"].stringValue();
        const auto age = rootObject["age"].intValue();

        printf("%s %s age %d\n", name.c_str(), lastName.c_str(), age);

    } catch (const Exception& ex) {
        // handle error
        printf("Ex: %s\n", ex.message().c_str());
    }
}

int main() {
    parseFile("example2.json");
    return 0;
}
