# CSON

A simple to use C++ JSON parser/writer library.

## Features

* Modern fast and simple to use library for C++11 and newer
* Mutli platform support (Linux, MaxOS, Windows, iOS, Android)


### Example

This example shows how to parse and evaluate the following simple json file:

```json
{
    "name": "John",
    "lastname": "Masters",
    "age": 48
}
```

```cpp
#include <cson.h>
#include <stdio.h>

void parseFile(const std::string& filename) {
    using namespace cson;

    try {
        auto context = Parser::parseFile(filename);
        auto& rootObject = context.object();

        const auto& name = rootObject["name"].stringValue();
        const auto& lastName = rootObject["lastname"].stringValue();
        const auto age = rootObject["age"].intValue();

        printf("%s %s age %d\n", name.c_str(), lastName.c_str(), age);

    } catch (const Exception& ex) {
        // handle error
        printf("Error: %s\n", ex.message().c_str());
    }
}

```
