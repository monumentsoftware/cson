#include <cson.h>
#include <stdio.h>

using namespace cson;


void parseFile(const char* path) {
    const auto json = JSON::load(path);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("missing arguments\n");
        return 1;
    }

    const char* path = argv[1];

    try {
        parseFile(path);
    }
    catch (Exception ex) {
        printf("Failed: %s\n", ex.message().c_str());
        return 1;
    }

    return 0;
}
