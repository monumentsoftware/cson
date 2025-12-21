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
The following code writes the given json file:

```cpp
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
```
