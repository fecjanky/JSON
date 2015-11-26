#include <iostream>
#include "JSON.h"
#include "JSONParser.h"

int main(int, char**) try{

    //auto p = JSON::Create<JSON::Number>(4.4255);
    //auto list = { JSON::Object::Entry{ "number",JSON::Create<JSON::Number>(4.4255) } , JSON::Object::Entry{"message",JSON::Create<JSON::String>("Hello")} };
    auto v = __cplusplus;

    //JSON::Create<JSON::Object>(std::move(list));
    //JSON::Create<JSON::Number>("4.4255");
    JSON::Number n2(3.14);
    JSON::Number n(std::string("4.4255"));
    //JSON::Bool b(std::string("false"));
    JSON::String s("jsdf;ldskfs");
    auto pp = "4.4255";

    auto p = JSON::Create<JSON::Object>(
        //JSON::Object::Entry{ "number",JSON::Create<JSON::Number>(4.4255) } ,
        JSON::Object::Entry( "message",JSON::Create<JSON::String>("Hello") )
    );

    auto& pr = *p;
    //std::cout << pr["number"].getValue() << std::endl;

    //pr["number"] = "kutyafasza";

    //std::cout << pr["number"]["kutyafasza"]. getValue() << std::endl;
    std::string json_text
    {R"--(
        {
            "name"  : "Ferenc",
            "age"   : 30
        }
    )--"};

    std::string json_text2
    { R"--(
        null false true
    )--" };


    auto objs = JSON::parse(json_text2.begin(), json_text2.end());
    return 0;
}
catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
}
catch (...) {
    return -1;
}
