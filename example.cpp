#include "type_streamer.h"
#include <iostream>
#include <utility>
#include <map>
#include <tuple>

enum class MyEnum { aaa, bbb };
enum class YourEnum { eee, fff };

struct Bar
{
    unsigned x;
    std::map<int, std::string> m;
    MyEnum e1;
    YourEnum e2;
};

struct Foo
{
    int x;
    std::vector<std::string> v;
    std::pair<int, std::string> m;
    Bar b;
    std::tuple<int, std::string> t;
};

std::ostream& operator<<(std::ostream& out, MyEnum value)
{
    switch(value)
    {
        case MyEnum::aaa:
            return out << "aaa";
        case MyEnum::bbb:
            return out << "bbb";
    }
    return out;
}


std::ostream& operator<<(std::ostream& out, const Bar& value)
{
    using namespace type_streamer;
    return out << Struct{out, "Bar"}
        .var("x", value.x)
        .var("m", value.m)
        .var("e1", value.e1)
        .var("e1", value.e2)
    ;
}

std::ostream& operator<<(std::ostream& out, const Foo& value)
{
    using namespace type_streamer;
    return out << Struct{out, "Foo"}
        .var("x", value.x)
        .var("v", value.v)
        .var("m", value.m)
        .var("b", value.b)
        //.var("t", value.t)
    ;
}

int main()
{
    std::cout << Foo{10, {"abc", "xyz"}, {7, "7"}, {1, {{1, "1"}, {2, "2"}}, MyEnum::bbb, YourEnum::fff}} << "\n";
}
