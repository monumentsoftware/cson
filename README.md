# CSON

A simple to use C++ JSON parser/writer library.

## Features

* Modern fast and simple to use library for C++11 and newer
* Multi platform support (Linux, MaxOS, Windows, iOS, Android)
* Optional C style comment support in JSON files
* Permissive MIT license

## Examples

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
        const auto json = JSON::load(filename);
        const auto& rootObject = json.object();

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

        JSON::save(object, filename);

    } catch (const Exception& ex) {
        // handle error
        printf("Ex: %s\n", ex.message().c_str());
    }
}
```

This snippet shows how to iterate arrays:

```cpp
    auto& arr = someObject["list"].array();
    for (auto& entity : arr) {
        printf("Value: %s\n", entity.stringValue());
    }
```

Objects can be iterated like this:

```cpp
    auto& obj = someObject["dict"].array();
    for (auto it : obj) {
        if (it == "name") {
            printf("Name: %s\n", it->stringValue().c_str());
        }
    }
```

## Comments

CSON supports C-style comments in objects and arrays:

```
{
  // This is a first comment
  [
    // This is a second comment
    "Example": "value",
    // Another comment
    "Example2": "value2",
    // Last comment in array
  ]
  // Last comment in object
}
```

Comments are disabled by default and must be enabled when loading a json file that contains any.

```c++
const auto json = JSON::load(filename, { JSON::Option::enableComments });
```

Internally, comments are represented as JSON entities. They can be accessed like other entities. When writing JSON files, comments are only written if pretty printing is active.



