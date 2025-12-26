#include <cson.h>
#include <stdio.h>

void writeFile(const std::string& filename) {
    using namespace cson;

    try {
        Object object;
        object.addString("name", "John");
        object.addString("lastname", "Masters");
        object.addInt("age", 48);
        

        Writer::writeToFile(filename, object);
        
    } catch (const Exception& ex) {
        // handle error
        printf("Ex: %s\n", ex.message().c_str());
    }
}

int main() {
    writeFile("out.json");
    return 0;
}
