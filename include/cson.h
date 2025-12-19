#pragma once
#include <string>
#include <map>
#include <vector>

namespace cson {

class Entity;
class Object;
class Array;
class Number;
class String;
class Boolean;
class Null;

class Exception {
public:
    Exception(const char* txt, ...) __attribute__((format(printf, 2, 3)));
    virtual ~Exception();

    const std::string& message() const { return mMessage; }

protected:
    Exception();
protected:
    std::string mMessage;
};

class ParseErrorException : public Exception {
public:
    ParseErrorException(const char* data, size_t dataLength, size_t position, const char* txt, ...) __attribute__((format(printf, 5, 6)));
    ~ParseErrorException() override;

    size_t position() const { return mPosition; }
    size_t line() const { return mLine; }
    size_t column() const { return mColumn; }
    const std::string& surrounding() const { return mSurrounding; }

private:
    size_t mPosition = 0;
    size_t mLine = 0;
    size_t mColumn = 0;
    std::string mSurrounding; // if possible: 2 lines before error ; error line ; 'marker' line ; 2 lines after error
};

class IOException : public Exception
{
public:
    IOException(const char* txt, ...) __attribute__((format(printf, 2, 3)));
};

class Entity {
public:

    Entity();
    virtual ~Entity();

    const Object& object() const;
    Object& object();

    const Array& array() const;
    Array& array();

    const Number& number() const;
    Number& number();

    const String& string() const;
    String& string();

    const Boolean& boolean() const;
    Boolean& boolean();

    const Null& null() const;
    Null& null();

    virtual bool contains(const std::string& key) const { (void)key; return false; }

    virtual const std::string& keyByIndex(size_t index) const;

    bool isObject() const;
    bool isArray() const;
    bool isString() const;
    bool isNumber() const;
    bool isBoolean() const;
    bool isNull() const;


    virtual size_t count() const;
    const std::string& stringValue() const;
    float floatValue() const;
    double doubleValue() const;
    int intValue() const;
    bool boolValue() const;

    const Entity& operator[] (size_t idx) const;
    const Entity& operator[] (const std::string& key) const;

    Entity& operator[] (size_t idx);
    Entity& operator[] (const char* key);
    Entity& operator[] (const std::string& key);

    virtual std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const = 0;
    virtual Entity* clone() const = 0;
protected:
    static std::string s_EmptyString;
};

class Object : public Entity {
public:
    Object();
    virtual ~Object();

    bool contains(const std::string& key) const override;
    bool remove(const std::string& name);

    Array* addArray(const std::string& name);
    Object* addObject(const std::string& name);
    Number* addNumber(const std::string& name);
    Number* addInt(const std::string& name, int i);
    Number* addFloat(const std::string& name, float f);
    Number* addDouble(const std::string& name, double d);
    String* addString(const std::string& name, const char* value = NULL);
    Boolean* addBoolean(const std::string& name, bool b = false);
    Null* addNull(const std::string& name);

    Number* setInt(const std::string& name, int i);
    Number* setFloat(const std::string&name, float f);
    Number* setDouble(const std::string& name, double d);
    String* setString(const std::string& name, const char* value = NULL);
    Boolean* setBoolean(const std::string& name, bool value = false);

    const std::string& stringValueForKey(const char* name, const std::string& defaultValue = s_EmptyString) const { return stringValueForKey(std::string(name), defaultValue); }
    const std::string& stringValueForKey(const std::string& name, const std::string& defaultValue = s_EmptyString) const;
    int intValueForKey(const char* name, int defaultValue = 0) const { return intValueForKey(std::string(name), defaultValue); }
    int intValueForKey(const std::string& name, int defaultValue = 0) const;
    float floatValueForKey(const char* name, float defaultValue = 0.0f) const { return floatValueForKey(std::string(name), defaultValue); }
    float floatValueForKey(const std::string& name, float defaultValue = 0.0f) const;
    double doubleValueForKey(const char* name, double defaultValue = 0.0f) const { return doubleValueForKey(std::string(name), defaultValue); }
    double doubleValueForKey(const std::string& name, double defaultValue = 0.0f) const;
    Number* numberForKey(const char* name) const { return numberForKey(std::string(name)); }
    Number* numberForKey(const std::string& name) const;
    Array* arrayForKey(const char* name) const { return arrayForKey(std::string(name)); }
    Array* arrayForKey(const std::string& name) const;
    Object* objectForKey(const char* name) const { return objectForKey(std::string(name)); }
    Object* objectForKey(const std::string& name) const;
    Boolean* boolForKey(const char* name) const { return boolForKey(std::string(name)); }
    Boolean* boolForKey(const std::string& name) const;
    bool boolValueForKey(const std::string& name, bool defaultValue = false) const;
    Null* nullForKey(const std::string& name) const;
    Entity* entityForKey(const std::string& name) const;

    const std::string& keyByIndex(size_t index) const override;

    size_t count() const override { return mEntities.size(); }
    Entity& entityAtIndex(size_t idx);
    const Entity& entityAtIndex(size_t idx) const;

    std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const override;

    Entity* clone() const override;

    void mergeFrom(const Object& obj, bool overwrite);

    struct KeyAndEntity {
        KeyAndEntity() = default;

        KeyAndEntity(const std::string& key_, Entity* entity_) : mKey(key_), mEntity(entity_) {
        }

        const std::string& key() const { return mKey; }

        Entity& entity() { return *mEntity; }

        std::string mKey;
        Entity* mEntity;
    };

    class Iterator {
    public:

        Iterator(std::vector<KeyAndEntity>::iterator it) : mIterator(it) {
        }

        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = KeyAndEntity;
        using pointer           = value_type*;  // or also value_type*
        using reference         = value_type&;  // or also value_type&

        reference operator*() { return *mIterator; }
        pointer operator->() { return mIterator.operator->(); }

        // Prefix increment
        Iterator& operator++() {
            ++mIterator;
            return *this;
        }

        // Postfix increment
        Iterator operator++(int) { Iterator tmp = *this; ++(*this); return tmp; }

        friend bool operator== (const Iterator& a, const Iterator& b) { return a.mIterator == b.mIterator; };
        friend bool operator!= (const Iterator& a, const Iterator& b) { return a.mIterator != b.mIterator; };
    private:
        std::vector<KeyAndEntity>::iterator mIterator;
   };

    Iterator begin() { return Iterator(mEntities.begin()); }
    Iterator end()   { return Iterator(mEntities.end()); } // 200 is out of bounds

private:
    std::vector<KeyAndEntity> mEntities;
    std::map<std::string, Entity*> mEntityByKey;
    friend class Parser;
};

class Array : public Entity {
public:
    Array();
    ~Array() override;

    void removeAtIndex(size_t index);

    Array* addArray();
    Object* addObject();
    Number* addInt(int value);
    Number* addFloat(float value);
    Number* addDouble(double value);
    String* addString(const char* str);
    String* addString(const std::string& str);
    Boolean* addBool(bool value);
    Null* addNull();

    const std::string& stringValueAtIndex(size_t index, const std::string& defaultValue = s_EmptyString) const;
    int intValueAtIndex(size_t index, int defaultValue = 0) const;
    float floatValueAtIndex(size_t index, float defaultValue = 0.0f) const;
    double doubleValueAtIndex(size_t index, double defaultValue = 0.0f) const;
    bool boolValueAtIndex(size_t index, bool defaultValue = false) const;

    Number* numberAtIndex(size_t index) const;
    Array* arrayAtIndex(size_t index) const;
    Object* objectAtIndex(size_t index) const;
    Boolean* boolAtIndex(size_t index) const;
    Null* nullAtIndex(size_t index) const;
    Entity& entityAtIndex(size_t index);
    const Entity& entityAtIndex(size_t index) const;

    std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const override;
    Entity* clone() const override;

    size_t count() const override{ return mValues.size(); }
private:
    std::vector<Entity*> mValues;

    friend class Parser;
};

class String : public Entity
{
public:
    String();
    ~String() override;

    void setString(const char* str);
    void setString(const std::string& str);

    std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const override;
    Entity* clone() const override;

    const std::string& value() const { return mValue; }

private:
    std::string mValue;
    friend class Parser;
};

class Number : public Entity
{
public:
    void setInt(int i);
    void setFloat(float f);
    void setDouble(double d);
    void setString(const std::string& num);

    std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const override;
    Entity* clone() const override;

    const std::string& value() const { return mNumber; }
    int valueInt() const;
    float valueFloat() const;
    double valueDouble() const;
private:
    std::string mNumber;

    friend class Parser;
};

class Boolean : public Entity
{
public:
    Boolean();
    ~Boolean() override;

    void setBool(bool b);

    std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const override;

    Entity* clone() const override;

    bool value() const { return mValue; }
private:
    bool mValue = false;

    friend class Parser;
};

class Null : public Entity {
public:
    std::string toString(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0) const override;

    Entity* clone() const override;

private:
    friend class Parser;
};

class JsonContext {
public:

    JsonContext(JsonContext&& ctx) = default;

    ~JsonContext();

    Entity& root();

    Object& object();

    Array& array();

private:
    JsonContext() = default;

    JsonContext(std::unique_ptr<Entity> root);

    JsonContext(const JsonContext&) = delete;

    void operator=(const JsonContext&) = delete;

    std::unique_ptr<Entity> mRoot;

    friend class Parser;
};

class Parser final {
public:
    Parser();

    ~Parser();

    JsonContext parse(const char* txt);
    JsonContext parse(const char* txt, size_t length);
    JsonContext parse(const std::string& txt);

    // static convenience functions
    static JsonContext parseString(const char* txt);
    static JsonContext parseString(const char* txt, size_t length);
    static JsonContext parseString(const std::string& txt);

    static JsonContext parseFromFile(const char* path);
    static JsonContext parseFromFile(const std::string& path);

private:
    void skipWhitespaces();
    bool tryToConsume(const char* txt);
    void consumeOrDie(const char* txt);
    std::string parseStringLiteral();

    Entity* parseValue();

    Array* parseArray();

    Object* parseObject();

    Number* parseNumber();

    String* parseString();

    size_t mPosition = 0;
    size_t mLength = 0;
    const char* mText = nullptr;
};

class Writer
{
public:
    Writer(bool prettyPrint = true, const std::string& indentation = std::string("  "), int level = 0);

    void writeToFile(const std::string& path, const Entity& ent);
private:
    bool mPrettyPrint = false;

    std::string mIndentation;

    int mLevel = 0;
};

} // cson

