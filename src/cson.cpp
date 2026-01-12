#include "cson.h"
#include <string.h>
#include <sstream>
#include <cstdarg>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <limits>

#ifndef _WIN32
#define MJSONvsprintf(str, size, format, args) vsnprintf(str, size, format, args)
#else // !_WIN32
#define MJSONvsprintf(str, size, format, args) vsprintf_s(str, size, format, args)
#endif // !_WIN32

namespace cson {

std::string Entity::s_EmptyString;

Exception::Exception(const char* txt, ...) {
    constexpr size_t bufSize = 16384;
    char buf[bufSize];
    va_list list;
    va_start(list, txt);
    MJSONvsprintf(buf, bufSize, txt, list);
    va_end(list);

    mMessage = std::string(buf);
}

Exception::Exception() {
}

InvalidType::InvalidType(const char* txt, ...) : Exception(std::string()) {
    constexpr size_t bufSize = 4096;
    char buf[bufSize];
    va_list list;
    va_start(list, txt);
    MJSONvsprintf(buf, bufSize, txt, list);
    va_end(list);

    mMessage = std::string(buf);
}


ParseError::ParseError(const char* data, size_t dataLength, size_t position, const char* txt, ...)
    : mPosition(position)
{
    char buf[16384];
    va_list list;
    va_start(list, txt);
    MJSONvsprintf(buf, 16384, txt, list);
    va_end(list);

    mMessage = std::string(buf);

    setupSurrounding(data, dataLength, position);
}

void ParseError::setupSurrounding(const char* data, size_t dataLength, size_t position) {
    if (!data) {
        return;
    }
    if (position >= dataLength) {
        return;
    }

    size_t currentStartOfLinePos = 0;
    size_t prevStartOfLinePos = std::numeric_limits<size_t>::max();
    size_t prev2StartOfLinePos = std::numeric_limits<size_t>::max();
    size_t line = 1;
    for (size_t i = 0; i < position; i++) {
        if (data[i] == '\n') {
            prev2StartOfLinePos = prevStartOfLinePos;
            prevStartOfLinePos = currentStartOfLinePos;
            currentStartOfLinePos = i + 1;
            line++;
        }
    }

    size_t endPosOfLine = position;
    for (size_t i = position; (size_t)i < dataLength; i++) {
        endPosOfLine = i;
        if (data[i] == '\n') {
            break;
        }
    }

    size_t nextLinesCount = 0;
    size_t next2LinesEndPos = endPosOfLine;
    for (size_t i = endPosOfLine + 1; i < dataLength; i++) {
        next2LinesEndPos = i;
        if (data[i] == '\n') {
            nextLinesCount++;
            if (nextLinesCount >= 2) {
                break;
            }
        }
    }

    mLine = line;
    mColumn = (mPosition - currentStartOfLinePos + 1);

    std::string markerLine;
    if (mColumn > 0) {
        markerLine = std::string(mColumn - 1, ' ');
    }
    markerLine.push_back('^');
    markerLine.push_back('\n');

    size_t surroundingStart = prev2StartOfLinePos; // attempt to include 2 previous lines
    if (surroundingStart == std::numeric_limits<size_t>::max()) {
        surroundingStart = prevStartOfLinePos;
    }

    if (surroundingStart == std::numeric_limits<size_t>::max()) {
        surroundingStart = currentStartOfLinePos;
    }
    std::string prevAndCurrentText(data + surroundingStart, endPosOfLine - surroundingStart + 1);
    std::string nextText(data + endPosOfLine + 1, next2LinesEndPos - endPosOfLine);
    mSurrounding = prevAndCurrentText + markerLine + nextText;
}

TooManyNestings::TooManyNestings(const char* data, size_t dataLength, size_t position)
: ParseError(data, dataLength, position, "Too many nestings") {
}

IOError::IOError(const char* txt, ...)
: Exception() {
    char buf[16384];
    va_list list;
    va_start(list, txt);
    MJSONvsprintf(buf, 16384, txt, list);
    va_end(list);

    mMessage = std::string(buf);
}

static std::string EscapeString(const std::string& str) {
    size_t escapeCount = 0;
    for (std::string::size_type i = 0; i < str.length(); i++) {
        char c = str[i];
        if (c == '\b' ||
            c == '\r' ||
            c == '\n' ||
            c == '\f' ||
            c == '\t' ||
            c == '\\' ||
            c == '/' ||
            c == '\"') {
            escapeCount++;
        }
    }
    std::string out;
    out.reserve(str.length() + escapeCount);
    for (std::string::size_type i = 0; i < str.length(); i++) {
        char c = str[i];
        switch (c) {
        case '\b':
            out += "\\b";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\t':
            out += "\\t";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '/':
            out += "\\/";
            break;
        case '\"':
            out += "\\\"";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

bool Entity::isObject() const {
    return dynamic_cast<const Object*>(this) != NULL;
}

bool Entity::isArray() const {
    return dynamic_cast<const Array*>(this) != NULL;
}

bool Entity::isString() const {
    return dynamic_cast<const String*>(this) != NULL;
}

bool Entity::isNumber() const {
    return dynamic_cast<const Number*>(this) != NULL;
}

bool Entity::isBoolean() const {
    return dynamic_cast<const Boolean*>(this) != NULL;
}

bool Entity::isNull() const {
    return dynamic_cast<const Null*>(this) != NULL;
}

const Object& Entity::object() const {
    const auto* obj = dynamic_cast<const Object*>(this);
    if (!obj) {
        throw InvalidType("object() called on entity that is not of type Object");
    }
    return *obj;
}

Object& Entity::object() {
    auto* obj = dynamic_cast<Object*>(this);
    if (!obj) {
        throw InvalidType("object() called on entity that is not of type Object");
    }
    return *obj;
}

const Array& Entity::array() const {
    const auto* arr = dynamic_cast<const Array*>(this);
    if (!arr) {
        throw InvalidType("array() called on entity that is not of type Array");
    }
    return *arr;
}

Array& Entity::array() {
    auto* arr = dynamic_cast<Array*>(this);
    if (!arr) {
        throw InvalidType("array() called on entity that is not of type Array");
    }
    return *arr;
}

const String& Entity::string() const {
    const auto* str = dynamic_cast<const String*>(this);
    if (!str) {
        throw InvalidType("string() called on entity that is not of type String");
    }
    return *str;
}

String& Entity::string() {
    auto* str = dynamic_cast<String*>(this);
    if (!str) {
        throw InvalidType("string() called on entity that is not of type String");
    }
    return *str;
}

const Number& Entity::number() const {
    const auto* number = dynamic_cast<const Number*>(this);
    if (!number) {
        throw InvalidType("number() called on entity that is not of type Number");
    }
    return *number;
}

Number& Entity::number() {
    auto* number = dynamic_cast<Number*>(this);
    if (!number) {
        throw InvalidType("number() called on entity that is not of type Number");
    }
    return *number;
}

const Boolean& Entity::boolean() const {
    const auto* boolean = dynamic_cast<const Boolean*>(this);
    if (!boolean) {
        throw InvalidType("boolean() called on entity that is not of type Boolean");
    }
    return *boolean;
}

Boolean& Entity::boolean() {
    auto* boolean = dynamic_cast<Boolean*>(this);
    if (!boolean) {
        throw InvalidType("boolean() called on entity that is not of type Boolean");
    }
    return *boolean;
}

const Null& Entity::null() const {
    const auto* n = dynamic_cast<const Null*>(this);
    if (!n) {
        throw InvalidType("null() called on entity that is not of type Null");
    }
    return *n;
}

Null& Entity::null() {
    auto* n = dynamic_cast<Null*>(this);
    if (!n) {
        throw InvalidType("null() called on entity that is not of type Null");
    }
    return *n;
}

const std::string& Entity::keyByIndex(size_t index) const {
    if (!isObject()) {
        throw InvalidType("keyByIndex() is only allowed for entities of type Object");
    }
    return Object().keyByIndex(index);
}

size_t Entity::count() const {
    throw InvalidType("count() is only allowed for entities of type Object or Array");
}

const std::string& Entity::stringValue() const {
    if (!isString()) {
        throw InvalidType("stringValue() is only allowed for entities of type String");
    }
    return string().value();
}

int Entity::intValue() const {
    return atoi(number().value().c_str());
}

float Entity::floatValue() const {
    return static_cast<float>(doubleValue());
}

double Entity::doubleValue() const {
    return atof(number().value().c_str());
}

bool Entity::boolValue() const {
    return boolean().value();
}

const Entity& Entity::operator[] (size_t idx) const {
    if (isArray()) {
        return array().entityAtIndex(idx);
    }
    else if (isObject()) {
        return object().entityAtIndex(idx);
    } else {
        throw InvalidType("operator[](int) is only allowed for entities of type Object or Array");
    }
}

const Entity& Entity::operator[] (const std::string& key) const {
    if (!isObject()) {
        throw InvalidType("operator[](key) is only allowed for entities of type Object");
    }
    return *object().entityForKey(key);
}

Entity& Entity::operator[] (size_t idx) {
    if (isArray()) {
        return array().entityAtIndex(idx);
    }
    else if (isObject()) {
        return object().entityAtIndex(idx);
    } else {
        throw InvalidType("operator[](int) is only allowed for entities of type Object or Array");
    }
}

Entity& Entity::operator[] (const std::string& key) {
    if (!isObject()) {
        throw InvalidType("operator[](key) is only allowed for entities of type Object");
    }
    return *object().entityForKey(key);
}

void Number::setInt(int i) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%d", i);
    mNumber = buf;
}

void Number::setFloat(float f) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%f", f);
    mNumber = buf;
}

void Number::setDouble(double d) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%f", d);
    mNumber = buf;
}

void Number::setString(const std::string& num) {
    mNumber = num;
}

int Number::valueInt() const {
    std::stringstream stream(mNumber);
    int v;
    stream >> v;
    if (stream.fail()) {
        v = 0;
    }
    return v;
}

float Number::valueFloat() const {
    std::stringstream stream(mNumber);
    float v;
    stream >> v;
    if (stream.fail()) {
        v = 0.0f;
    }
    return v;
}

double Number::valueDouble() const {
    std::stringstream stream(mNumber);
    double v;
    stream >> v;
    if (stream.fail()) {
        v = 0.0;
    }
    return v;
}

std::string Number::toString(bool prettyPrint, const std::string& indentation, size_t level) const {
    (void)prettyPrint;
    (void)indentation;
    (void)level;

    return mNumber;
}

Entity* Number::clone() const {
    auto* clone = new Number();
    clone->mNumber = mNumber;
    return clone;
}

void String::setString(const char* str) {
    mValue = std::string(str);
}

void String::setString(const std::string& str) {
    mValue = str;
}

std::string String::toString(bool prettyPrint, const std::string& indentation, size_t level) const {
    (void)prettyPrint;
    (void)indentation;
    (void)level;

    std::string s;
    s += "\"";
    s += EscapeString(mValue);
    s += "\"";
    return s;
}

Entity* String::clone() const {
    return new String(mValue);
}

Array::~Array() {
    for (size_t i = 0; i < mValues.size(); i++) {
        delete mValues[i];
    }
}

void Array::removeAtIndex(size_t index) {
    if (index >= mValues.size()) {
        throw OutOfBounds();
    }
    auto* ent = mValues[index];
    delete ent;
    mValues.erase(std::next(mValues.begin(), static_cast<std::vector<Entity*>::difference_type>(index)));
}

Array& Array::addArray() {
    auto* arr = new Array();
    mValues.push_back(arr);
    return *arr;
}

Object& Array::addObject() {
    auto* arr = new Object();
    mValues.push_back(arr);
    return *arr;
}

Number& Array::addInt(int value) {
    auto* num = new Number();
    num->setInt(value);
    mValues.push_back(num);
    return *num;
}

Number& Array::addFloat(float value) {
    auto* num = new Number();
    num->setFloat(value);
    mValues.push_back(num);
    return *num;
}

Number& Array::addDouble(double value) {
    auto* num = new Number();
    num->setDouble(value);
    mValues.push_back(num);
    return *num;
}

String& Array::addString(const char* str) {
    auto* s = new String(str);
    mValues.push_back(s);
    return *s;
}

String& Array::addString(const std::string& str) {
    auto* s = new String(str);
    mValues.push_back(s);
    return *s;
}

Boolean& Array::addBool(bool value) {
    auto* b = new Boolean();
    b->setBool(value);
    mValues.push_back(b);
    return *b;
}

Null& Array::addNull() {
    auto* n = new Null();
    mValues.push_back(n);
    return *n;
}

static void writeCommaIfNeeded(std::string& str, const std::vector<Entity*>& entities, size_t index) {
    if (entities.at(index)->type() == Entity::Type::comment) {
        return;
    }

    bool needsComma = false;
    for (size_t i = index + 1; i < entities.size(); i++) {
        if (entities.at(i)->type() != Entity::Type::comment) {
            needsComma = true;
            break;
        }
    }

    if (needsComma) {
        str += ",";
    }
}

std::string Array::toString(bool prettyPrint, const std::string& indentation, size_t level) const {

    std::string s = "[";
    if (prettyPrint) {
        std::string prefix;
        for (size_t i = 0; i < level; i++) {
            prefix += indentation;
        }

        s += "\n";
        for (size_t i = 0; i < mValues.size(); i++) {
            s += prefix;
            s += indentation;

            auto* entity = mValues.at(i);
            s += entity->toString(prettyPrint, indentation, level + 1);
            writeCommaIfNeeded(s, mValues, i);

            s += "\n";
        }
        s += prefix;
    } else {
        for (size_t i = 0; i < mValues.size(); i++) {
            auto* entity = mValues.at(i);
            if (entity->type() == Type::comment) {
                continue;
            }

            s += entity->toString(prettyPrint, indentation, level + 1);
            writeCommaIfNeeded(s, mValues, i);
        }
    }
    s += "]";
    return s;
}

const std::string& Array::stringValueAtIndex(size_t index, const std::string& defaultValue) const {
    if (index >= count() || !mValues[index] || !mValues[index]->isString()) {
        return defaultValue;
    }
    return static_cast<String*>(mValues[index])->value();
}

Number& Array::numberAtIndex(size_t index) const {
    if (index >= count() || !mValues[index] || !mValues[index]->isNumber()) {
        throw OutOfBounds();
    }
    if (!mValues[index] || !mValues[index]->isNumber()) {
        throw InvalidType("numberAtIndex() failed - type of entity at index %zu is not Number", index);
    }
    return *static_cast<Number*>(mValues[index]);
}

int Array::intValueAtIndex(size_t index, int defaultValue) const {
    if (index >= count() || !mValues[index] || !mValues[index]->isNumber()) {
        return defaultValue;
    }

    auto& number = numberAtIndex(index);
    return number.valueInt();
}

float Array::floatValueAtIndex(size_t index, float defaultValue) const {
    if (index >= count() || !mValues[index] || !mValues[index]->isNumber()) {
        return defaultValue;
    }

    auto& number = numberAtIndex(index);
    return number.valueFloat();
}

double Array::doubleValueAtIndex(size_t index, double defaultValue) const {
    if (index >= count() || !mValues[index] || !mValues[index]->isNumber()) {
        return defaultValue;
    }

    auto& number = numberAtIndex(index);
    return number.valueDouble();
}

Array& Array::arrayAtIndex(size_t index) const {
    if (index >= count()) {
        throw OutOfBounds();
    }

    auto* e = mValues[index];
    if (!e || !e->isArray()) {
        throw InvalidType();
    }
    return *static_cast<Array*>(e);
}

Object& Array::objectAtIndex(size_t index) const {
    if (index >= count()) {
        throw OutOfBounds();
    }

    auto* e = mValues[index];
    if (!e || !e->isObject()) {
        throw InvalidType();
    }
    return *static_cast<Object*>(e);
}

Boolean& Array::boolAtIndex(size_t index) const {
    if (index >= count()) {
        throw OutOfBounds();
    }

    auto* e = mValues[index];
    if (!e || !e->isBoolean()) {
        throw InvalidType();
    }

    return *static_cast<Boolean*>(e);
}

bool Array::boolValueAtIndex(size_t index, bool defaultValue) const {
    if (index >= count() || !mValues[index] || !mValues[index]->isBoolean()) {
        return defaultValue;
    }

    auto& b = boolAtIndex(index);
    return b.value();
}

Null& Array::nullAtIndex(size_t index) const {
    if (index >= count()) {
        throw OutOfBounds();
    }

    auto* e = mValues[index];
    if (!e || !e->isNull()) {
        throw InvalidType();
    }
    return *static_cast<Null*>(e);
}

Entity& Array::entityAtIndex(size_t index) {
    return *mValues[index];
}

const Entity& Array::entityAtIndex(size_t index) const {
    return *mValues[index];
}

Entity* Array::clone() const {
    auto* clone = new Array();
    clone->mValues.resize(mValues.size());
    for (std::size_t i = 0; i < mValues.size(); i++) {
        auto* e = mValues[i]->clone();
        clone->mValues[i] = e;
    }
    return clone;
}

Object::~Object() {
    for (auto& entity : mEntities) {
        delete entity.mEntity;
    }
}

bool Object::contains(const std::string& key) const {
    return mEntityByKey.find(key) != mEntityByKey.end();
}

Array& Object::addArray(const std::string& name) {
    if (contains(name)) {
        throw NoSuchKey();
    }

    auto* arr = new Array();
    mEntities.push_back(KeyAndEntity(name, arr));
    mEntityByKey[name] = arr;
    return *arr;
}

Object& Object::addObject(const std::string& name) {
    if (contains(name)) {
        throw NoSuchKey();
    }

    auto* obj = new Object();
    mEntities.push_back(KeyAndEntity(name, obj));
    mEntityByKey[name] = obj;
    return *obj;
}

Number& Object::addNumber(const std::string& name) {
    if (contains(name)) {
        throw NoSuchKey();
    }

    auto* num = new Number();
    mEntities.push_back(KeyAndEntity(name, num));
    mEntityByKey[name] = num;
    return *num;
}

Number& Object::addInt(const std::string& name, int i) {
    auto& number = addNumber(name);
    number.setInt(i);
    return number;
}

Number& Object::addFloat(const std::string& name, float f) {
    auto& number = addNumber(name);
    number.setFloat(f);
    return number;
}

Number& Object::addDouble(const std::string& name, double d) {
    auto& number = addNumber(name);
    number.setDouble(d);
    return number;
}

String& Object::addString(const std::string& name, const char* value) {
    if (contains(name)) {
        throw NoSuchKey();
    }

    auto* str = new String();
    if (value) {
        str->setString(value);
    }
    mEntities.push_back(KeyAndEntity(name, str));
    mEntityByKey[name] = str;
    return *str;
}

Boolean& Object::addBoolean(const std::string& name, bool b) {
    if (contains(name)) {
        throw NoSuchKey();
    }

    auto* boolean = new Boolean();
    boolean->setBool(b);
    mEntities.push_back(KeyAndEntity(name, boolean));
    mEntityByKey[name] = boolean;
    return *boolean;
}

Null& Object::addNull(const std::string& name) {
    if (contains(name)) {
        throw NoSuchKey();
    }

    auto* null = new Null();
    mEntities.push_back(KeyAndEntity(name, null));
    mEntityByKey[name] = null;
    return *null;
}

Number& Object::setInt(const std::string& name, int i) {
    auto* ent = entityForKey(name);
    if (!ent) {
        return addInt(name, i);
    }

    if (!ent->isNumber()) {
        remove(name);
        return addInt(name, i);
    }
    ent->number().setInt(i);
    return ent->number();
}

Number& Object::setFloat(const std::string& name, float f) {
    auto* ent = entityForKey(name);
    if (!ent) {
        return addFloat(name, f);
    }

    if (!ent->isNumber()) {
        remove(name);
        return addFloat(name, f);
    }
    ent->number().setFloat(f);
    return ent->number();
}

Number& Object::setDouble(const std::string& name, double d) {
    auto* ent = entityForKey(name);
    if (!ent) {
        return addDouble(name, d);
    }

    if (!ent->isNumber()) {
        remove(name);
        return addDouble(name, d);
    }
    ent->number().setDouble(d);
    return ent->number();
}

String& Object::setString(const std::string& name, const char* value) {
    auto* ent = entityForKey(name);
    if (!ent) {
        return addString(name, value);
    }

    if (!ent->isString()) {
        remove(name);
        return addString(name, value);
    }
    ent->string().setString(value);
    return ent->string();
}

Boolean& Object::setBoolean(const std::string& name, bool b) {
    auto* ent = entityForKey(name);
    if (!ent) {
        return addBoolean(name, b);
    }

    if (!ent->isBoolean()) {
        remove(name);
        return addBoolean(name, b);
    }
    ent->boolean().setBool(b);
    return ent->boolean();
}

Entity& Object::entityAtIndex(size_t idx) {
    return *mEntities[idx].mEntity;
}

const Entity& Object::entityAtIndex(size_t idx) const {
    return *mEntities[idx].mEntity;
}

const std::string& Object::keyByIndex(size_t idx) const {
    return mEntities[idx].mKey;
}

static void writeCommaIfNeeded(std::string& str, const std::vector<Object::KeyAndEntity>& entities, size_t index) {
    if (entities.at(index).mEntity->type() == Entity::Type::comment) {
        return;
    }

    bool needsComma = false;
    for (size_t i = index + 1; i < entities.size(); i++) {
        if (entities.at(i).mEntity->type() != Entity::Type::comment) {
            needsComma = true;
            break;
        }
    }

    if (needsComma) {
        str += ",";
    }
}

std::string Object::toString(bool prettyPrint, const std::string& indentation, size_t level) const {
    std::string s;
    if (prettyPrint) {
        std::string prefix;
        for (size_t i = 0; i < level; i++) {
            prefix += indentation;
        }

        if (level > 0) {
            s += "\n";
        }

        s += prefix + "{";
        s += "\n";

        for (size_t i = 0; i < mEntities.size(); i++) {
            const auto& entityAndKey = mEntities.at(i);

            s += prefix + indentation;
            if (entityAndKey.mEntity->type() != Entity::Type::comment) {
                s += "\"";
                s += EscapeString(entityAndKey.mKey);
                s += "\"";
                s += ":";
            }
            s += entityAndKey.mEntity->toString(prettyPrint, indentation, level + 1);

            writeCommaIfNeeded(s, mEntities, i);
            s += "\n";

        }
        s += "\n";
        s += prefix;
    } else {
        s += "{";

        for (size_t i = 0; i < mEntities.size(); i++) {
            const auto& entityAndKey = mEntities.at(i);
            if (entityAndKey.mEntity->type() == Type::comment) {
                continue;
            }

            s += "\"";
            s += EscapeString(entityAndKey.mKey);
            s += "\"";
            s += ":";
            s += entityAndKey.mEntity->toString(prettyPrint, indentation, level + 1);
            writeCommaIfNeeded(s, mEntities, i);
        }
    }
    s += "}";
    return s;
}

const std::string& Object::stringValueForKey(const std::string& name, const std::string& defaultValue) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second || !it->second->isString()) {
        return defaultValue;
    }
    return static_cast<String*>(it->second)->value();
}

Number* Object::numberForKey(const std::string& name) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second || !it->second->isNumber()) {
        return nullptr;
    }
    return static_cast<Number*>(it->second);
}

int Object::intValueForKey(const std::string& name, int defaultValue) const {
    auto* number = numberForKey(name);
    if (!number) {
        return defaultValue;
    }
    return number->valueInt();
}

float Object::floatValueForKey(const std::string& name, float defaultValue) const {
    auto* number = numberForKey(name);
    if (!number) {
        return defaultValue;
    }
    return number->valueFloat();
}

double Object::doubleValueForKey(const std::string& name, double defaultValue) const {
    auto* number = numberForKey(name);
    if (!number) {
        return defaultValue;
    }
    return number->valueDouble();
}

Array* Object::arrayForKey(const std::string& name) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second || !it->second->isArray()) {
        return NULL;
    }
    return static_cast<Array*>(it->second);
}

Object* Object::objectForKey(const std::string& name) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second || !it->second->isObject()) {
        return nullptr;
    }
    return static_cast<Object*>(it->second);
}

Boolean* Object::boolForKey(const std::string& name) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second || !it->second->isBoolean()) {
        return nullptr;
    }
    return static_cast<Boolean*>(it->second);
}

bool Object::boolValueForKey(const std::string& name, bool defaultValue) const {
    auto* b = boolForKey(name);
    if (!b) {
        return defaultValue;
    }
    return b->value();
}

Null* Object::nullForKey(const std::string& name) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second || !it->second->isNull()) {
        return nullptr;
    }
    return static_cast<Null*>(it->second);
}

Entity* Object::entityForKey(const std::string& name) const {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end() || !it->second) {
        return nullptr;
    }
    return it->second;
}

bool Object::remove(const std::string& name) {
    auto it = mEntityByKey.find(name);
    if (it == mEntityByKey.end()) {
        return false;
    }

    auto* ent = it->second;
    mEntityByKey.erase(it);

    auto it2 = std::find_if(mEntities.begin(), mEntities.end(), [name](const KeyAndEntity& ent) {
        return ent.mKey == name;
    });
    mEntities.erase(it2);
    delete ent;
    return true;
}

Entity* Object::clone() const {
    auto* clone = new Object();
    clone->mEntities.resize(mEntities.size());
    for (size_t i = 0; i < mEntities.size(); i++) {
        auto childClone = mEntities[i].mEntity->clone();
        clone->mEntities[i] = KeyAndEntity(mEntities[i].mKey, childClone);
        clone->mEntityByKey[mEntities[i].mKey] = childClone;
    }
    return clone;
}

void Boolean::setBool(bool b) {
    mValue = b;
}

std::string Boolean::toString(bool prettyPrint, const std::string& indentation, size_t level) const {
    (void)prettyPrint;
    (void)indentation;
    (void)level;

    return mValue ? std::string("true") : std::string("false");
}

Entity* Boolean::clone() const {
    auto* clone = new Boolean();
    clone->mValue = mValue;
    return clone;
}

std::string Null::toString(bool prettyPrint, const std::string& indentation, size_t level) const {
    (void)prettyPrint;
    (void)indentation;
    (void)level;

    return std::string("null");
}

Entity* Null::clone() const {
    return new Null();
}

std::string Comment::toString(bool prettyPrint, const std::string& indentation, size_t level) const {
    (void)prettyPrint;
    (void)indentation;
    (void)level;

    return std::string("//") + mText;
}

Entity* Comment::clone() const {
    auto* clone = new Comment();
    clone->mText = mText;
    return clone;
}

JSON::JSON(std::unique_ptr<Entity> root) : mRoot(std::move(root)) {
}

JSON::~JSON() {
}

Entity& JSON::root() {
    return *mRoot;
}

const Entity& JSON::root() const {
    return *mRoot;
}

Object& JSON::object() {
    return mRoot->object();
}

const Object& JSON::object() const {
    return mRoot->object();
}

Array& JSON::array() {
    return mRoot->array();
}

const Array& JSON::array() const {
    return mRoot->array();
}

JSON JSON::load(const std::string& path, const std::set<Option>& options) {
    const bool enableCommands = options.find(Option::enableComments) != options.end();
    return Parser::parseFile(path, enableCommands);
}

JSON JSON::fromString(const std::string& json, const std::set<Option>& options) {
    const bool enableCommands = options.find(Option::enableComments) != options.end();
    return Parser::parseString(json, enableCommands);
}

void JSON::save(const Entity& entity, const std::string& path, const std::set<Option>& options) const {
    const bool prettyPrint = options.find(Option::prettyPrint) != options.end();

    std::string indent = "  ";
    if (options.find(Option::indent4Spaces) != options.end()) {
        indent = "    ";
    } else if (options.find(Option::indentTab) != options.end()) {
        indent = "\t";
    }

    Writer::writeToFile(path, entity, prettyPrint, indent);
}

std::string JSON::toString(const Entity& entity, const std::set<Option>& options) const {
    const bool prettyPrint = options.find(Option::prettyPrint) != options.end();

    std::string indent = "  ";
    if (options.find(Option::indent4Spaces) != options.end()) {
        indent = "    ";
    } else if (options.find(Option::indentTab) != options.end()) {
        indent = "\t";
    }

    return entity.toString(prettyPrint, indent);
}


Parser::Parser(){
}

Parser::~Parser() {
}

void Parser::allowComments(bool allow) {
    mAllowComments = allow;
}

void Parser::setMaxDepth(size_t maxDepth) {
    mMaxDepth = maxDepth;
}

void Parser::skipWhitespaces() {
    while ( mPosition < mLength
           &&
           (mText[mPosition] == ' '
            || mText[mPosition] == '\t'
            || mText[mPosition] == '\r'
            || mText[mPosition] == '\n')) {
        mPosition++;
    }
}

// returns '\0' if end of the document is reached
char Parser::curChar() {
    if (mPosition == mLength) {
        return 0;
    }

    const char c = mText[mPosition];
    return c;
}

char Parser::consumeChar() {
    if (mPosition == mLength) {
        throw ParseError(mText, mLength, mPosition, "Empty input");
    }

    const char c = mText[mPosition];
    mPosition++;
    return c;
}


bool Parser::tryToConsume(const char* txt) {
    size_t storedPos = mPosition;
    int i = 0;
    bool found = false;
    while (mPosition < mLength) {
        char c = mText[mPosition];
        if (c != txt[i]) {
            break;
        }

        mPosition++;
        i++;

        if (txt[i] == 0) {
            found = true;
            break;
        }
    }
    if (!found) {
        mPosition = storedPos;
    }
    return found;
}

void Parser::consumeOrDie(const char* txt) {
    size_t origPos = mPosition;
    bool b = tryToConsume(txt);
    if (!b) {
        throw ParseError(mText, mLength, origPos, "Syntax error: Expected '%s' at or after position %d", txt, static_cast<int>(origPos));
    }
}

static int writeUTF8Chars(char* buf, uint32_t c) {
    if (c < 128) {
        buf[0] = static_cast<char>(c);
        return 1;
    } else if (c < 2048) {
        buf[0] = static_cast<char>(0xc0 | ((c >> 6) & 0xff));
        buf[1] = static_cast<char>(0x80 | (c & 63));
        return 2;
    } else {
        buf[0] = static_cast<char>(0xe0 | ((c >> 12) & 0xff));
        buf[1] = static_cast<char>(0x80 | ((c >> 6)  & 63));
        buf[2] = static_cast<char>(0x80 | (c & 63));
        return 3;
    }
}

static int hexDigitToValue(char d) {
    if (d >= '0' && d <= '9') {
        return d - '0';
    }

    if (d >= 'a' && d <= 'f') {
        return d - 'a' + 10;
    }

    if (d >= 'A' && d <= 'F') {
        return d - 'A' + 10;
    }

    return -1;
}

uint32_t Parser::parse4DigitHexNumber(const char* num) {

    uint32_t value = 0;
    for (size_t i = 0; i < 4; i++) {
        const auto hexValue = hexDigitToValue(num[i]);
        if (hexValue == -1) {
            throw ParseError(mText, mLength, mPosition, "Hex digit expected");
        }
        value |= static_cast<uint32_t>(hexValue) << ((3 - i) * 4);
    }
    return value;
}

// NOTE: does NOT support empty strings, caller needs to check that!
std::string Parser::parseStringLiteral() {
    std::string str;
    str.reserve(1024);

    tryToConsume("\""); // NOTE: required because caller *may* have consumed this already, but does not have to. NOTE that due to this, we cannot support empty strings (this call would consume the closing \")
    size_t origPos = mPosition;
    char c = mText[mPosition];
    size_t utf8ByteCount = 0;
    while (c != '\"') {
        if (mPosition == mLength) {
            throw ParseError(mText, mLength, origPos, "Closing \" not found");
        }

        if (utf8ByteCount == 0) {
            if (c == '\\'
                && mPosition + 1 < mLength) {
                mPosition++;
                c = mText[mPosition];
                switch (c) {
                case 'b': c = '\b'; break;
                case 'r': c = '\r'; break;
                case 'n': c = '\n'; break;
                case 'f': c = '\f'; break;
                case 't': c = '\t'; break;
                case '\\': c = '\\'; break;
                case '/': c = '/'; break;
                case '\"': c = '\"'; break;
                case 'u': {
                        mPosition++;
                        if (mPosition + 4 > mLength) {
                            throw ParseError(mText, mLength, origPos, "Invalid \\u escaping");
                        }
                        const uint32_t utf8Char = parse4DigitHexNumber(&mText[mPosition]);
                        char utf8Buf[16];
                        int len = writeUTF8Chars(utf8Buf, utf8Char);
                        for (int i = 0; i < len-1; i++) {
                            str += utf8Buf[i];
                        }
                        c = utf8Buf[len-1];
                        mPosition += 3;
                    }
                    break;
                default:
                    throw ParseError(mText, mLength, origPos, "Invalid character after escape character");
                }
            } else {
                if ((c & 0b10000000) == 0) {
                    utf8ByteCount = 0;
                    if (c < 32) {
                        throw ParseError(mText, mLength, mPosition, "Control character in string literal");
                    }
                } else if ((c & 0b11110000) == 0b11110000) {
                    utf8ByteCount = 3;
                } else if ((c & 0b11100000) == 0b11100000) {
                    utf8ByteCount = 2;
                } else if ((c & 0b11000000) == 0b11000000) {
                    utf8ByteCount = 1;
                } else {
                    throw ParseError(mText, mLength, origPos, "Invalid utf8 code point");
                }
            }
        } else if (utf8ByteCount > 0) {
            if ((c & 0b10000000) != 0b10000000) {
                throw ParseError(mText, mLength, origPos, "Invalid UTF8 code point");
            }
            utf8ByteCount--;
        }

        str += c;
        mPosition++;
        if (mPosition == mLength) {
            throw ParseError(mText, mLength, origPos, "Closing \" not found");
        }
        c = mText[mPosition];
    }
    mPosition++;
    return str;
}

Comment* Parser::parseComment() {
    if (!mAllowComments) {
        throw ParseError(mText, mLength, mPosition, "Comments are disabled");
    }

    std::string text;
    while (mPosition < mLength) {
        const auto c = consumeChar();
        if (c == '\n') {
            break;
        }
        text += c;
    }

    auto comment = std::make_unique<Comment>();
    comment->mText = text;
    return comment.release();
}

Entity* Parser::parseValue(size_t depth) {
    Entity* data = nullptr;
    if (tryToConsume("\"")) {
        if (tryToConsume("\"")) {
            // special case: empty string
            data = new String();
        } else {
            data = parseString();
        }
    } else if (tryToConsume("[")) {
        data = parseArray(depth + 1);
    } else if (tryToConsume("{")) {
        data = parseObject(depth + 1);
    } else if (tryToConsume("true")) {
        auto* b = new Boolean();
        b->setBool(true);
        data = b;
    } else if (tryToConsume("false")) {
        auto* b = new Boolean();
        b->setBool(false);
        data = b;
    } else if (tryToConsume("null")) {
        data = new Null();
    } else {
        data = parseNumber();
    }
    return data;
}

Array* Parser::parseArray(size_t depth) {
    if (depth > mMaxDepth) {
        throw TooManyNestings(mText, mLength, mPosition);
    }

    auto arr = std::make_unique<Array>();
    while (true) {
        skipWhitespaces();

        if (tryToConsume("//")) {
            auto* comment = parseComment();
            arr->mValues.push_back(comment);
            skipWhitespaces();
        }

        // empty array?
        if (arr->count() == 0
            && tryToConsume("]")) {
            break;
        }

        auto* ent = parseValue(depth);
        arr->mValues.push_back(ent);

        skipWhitespaces();

        if (tryToConsume("//")) {
            auto* comment = parseComment();
            arr->mValues.push_back(comment);
            skipWhitespaces();
        }

        if (!tryToConsume(",")) {
            consumeOrDie("]");
            break;
        }
    }
    return arr.release();
}

Object* Parser::parseObject(size_t depth) {
    if (depth > mMaxDepth) {
        throw TooManyNestings(mText, mLength, mPosition);
    }

    auto obj = std::make_unique<Object>();
    while (true) {
        skipWhitespaces();

        // empty object?
        if (obj->count() == 0
            && tryToConsume("}")) {
            break;
        }

        if (tryToConsume("//")) {
            auto* comment = parseComment();
            obj->mEntities.push_back(Object::KeyAndEntity("", comment));
            skipWhitespaces();
        }

        std::string key = parseStringLiteral();
        skipWhitespaces();
        consumeOrDie(":");
        skipWhitespaces();
        auto* ent = parseValue(depth);

        obj->mEntities.push_back(Object::KeyAndEntity(key, ent));
        obj->mEntityByKey[key] = ent;

        skipWhitespaces();

        if (tryToConsume("//")) {
            auto* comment = parseComment();
            obj->mEntities.push_back(Object::KeyAndEntity("", comment));
            skipWhitespaces();
        }

        if (!tryToConsume(",")) {
            consumeOrDie("}");
            break;
        }
    }


    return obj.release();
}

size_t Parser::readDigits(std::string& dest) {
    size_t count = 0;
    while (mPosition < mLength) {
        char c = mText[mPosition];
        if (c < '0' || c > '9') {
            break;
        }
        dest += c;
        mPosition++;
        count++;
    }
    return count;
}

Number* Parser::parseNumber() {
    auto num = std::make_unique<Number>();
    std::string str;
    str.reserve(32);

    if (tryToConsume("-")) {
        str += "-";
    }

    if (tryToConsume("0")) {
        str += "0";
    } else {
        const char c = consumeChar();
        if (c < '1' || c > '9') {
            throw ParseError(mText, mLength, mPosition, "Expecting digit 1...9");
        }

        str += c;
        readDigits(str);
    }

    // optional fraction part
    if (tryToConsume(".")) {
        str += ".";
        const auto count = readDigits(str);
        if (count == 0) {
            throw ParseError(mText, mLength, mPosition, "Expecting 0..9");
        }
    }

    // optional exponent part
    char c = curChar();
    if (c == 'e' || c == 'E') {
        str += c;
        mPosition++;

        c = curChar();
        if (c == '+' || c == '-') {
            str += c;
            mPosition++;
        }

        const auto count = readDigits(str);
        if (count == 0) {
            throw ParseError(mText, mLength, mPosition, "Expecting 0..9");
        }
    }

    num->mNumber = str;
    return num.release();
}


// NOTE: does NOT support empty strings, caller needs to check that!
String* Parser::parseString() {
    return new String(parseStringLiteral());
}

JSON Parser::parse(const char* txt, size_t length) {
    mText = txt;
    mLength = length;
    mPosition = 0;

    std::unique_ptr<Entity> root;
    skipWhitespaces();
    if (mPosition == mLength) {
        throw ParseError(mText, mLength, mPosition, "Empty input");
    }

    root.reset(parseValue(0));

    skipWhitespaces();
    if (mPosition != mLength) {
        throw ParseError(mText, mLength, mPosition, "Extra bytes at end of json");
    }
    return JSON(std::move(root));
}

JSON Parser::parse(const char* txt) {
    return parse(txt, strlen(txt));
}

JSON Parser::parse(const std::string& txt) {
    return parse(txt.c_str());
}

JSON Parser::parseString(const char* txt, bool allowComments) {
    Parser parser;
    parser.allowComments(allowComments);
    return parser.parse(txt);
}

JSON Parser::parseString(const char* txt, size_t length, bool allowComments) {
    Parser parser;
    parser.allowComments(allowComments);
    return parser.parse(txt, length);
}

JSON Parser::parseString(const std::string& txt, bool allowComments) {
    Parser parser;
    parser.allowComments(allowComments);
    return parser.parse(txt);
}

JSON Parser::parseFile(const std::string& path, bool allowComments) {
    struct FileCloser {
        FILE* mFile;
        FileCloser(FILE* f) {
            mFile = f;
        }

        ~FileCloser() {
            fclose(mFile);
        }
    };

    auto* f = fopen(path.c_str(), "rb");
    if (!f) {
        throw IOError("Failed to open file %s", path.c_str());
    }

    FileCloser file(f); // close the file when leaving this method

    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    if (size < 0) {
        int err = errno;
        throw IOError("Read error in file %s, errno: %d (%s)", path.c_str(), err, strerror(err));
    }
    fseek(f, 0, SEEK_SET);
    char* buf = new char[static_cast<size_t>(size)];
    size_t rd = fread(buf, 1, static_cast<size_t>(size), f);

    if (rd != (size_t)size) {
        delete[] buf;
        throw IOError("Failed to read %zu bytes from file (read=%zu)", size, (size_t)rd);
    }
    try {
        auto context = parseString(buf, static_cast<size_t>(size), allowComments);
        delete[] buf;
        return context;
    }
    catch (...) {
        delete[] buf;
        throw;
    }
}

Writer::Writer(bool prettyPrint, const std::string& indentation, size_t level)
: mPrettyPrint(prettyPrint),
  mIndentation(indentation),
  mLevel(level) {
}

void Writer::write(const std::string& path, const Entity& ent) {
    std::string json = ent.toString(mPrettyPrint, mIndentation, mLevel);

    auto* f = fopen(path.c_str(), "wb");
    if (!f) {
        throw IOError("Failed to open file for writing");
    }

    size_t wr = fwrite(json.c_str(), 1, json.length(), f);
    fclose(f);

    if (wr != json.length()) {
        throw IOError("Failed to write all bytes to file");
    }
}

void Writer::writeToFile(const std::string& path, const Entity& ent, bool prettyPrint, const std::string& indentation, size_t level) {

    Writer writer(prettyPrint, indentation, level);
    writer.write(path, ent);
}

} // cson
