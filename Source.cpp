#include <iostream>
#include "JSON.h"
#include "JSONParser.h"

int main(int, char**) try{

    //auto p = JSON::Create<JSON::Number>(4.4255);
    //auto list = { JSON::Object::Entry{ "number",JSON::Create<JSON::Number>(4.4255) } , JSON::Object::Entry{"message",JSON::Create<JSON::String>("Hello")} };

    //JSON::Create<JSON::Object>(std::move(list));
    //JSON::Create<JSON::Number>("4.4255");
    JSON::Number n2(3.14);
    JSON::Number n(std::string("4.4255"));
    //JSON::Bool b(std::string("false"));
    JSON::String s("jsdf;ldskfs");

    auto p = JSON::Create<JSON::Object>(
        //JSON::Object::Entry{ "number",JSON::Create<JSON::Number>(4.4255) } ,
        JSON::Object::Entry( "message",JSON::Create<JSON::String>("Hello") )
    );

    auto& pr = *p;
    //std::cout << pr["number"].getValue() << std::endl;


    std::string json_text
    {R"--(
        {
            "name"  : "Ferenc",
            "age"   : 30
        }
    )--"};

    //null false true -0.145e11 
    std::string json_text2
    { R"--( 
    null false true -0.145e11
    "jdbc:microsoft:sqlserver://LOCALHOST:1433;DatabaseName=goon"    
    )--" };
    
    std::string json_text3
    { R"--( 
    {
        "menu": {
            "id": "file",
            "value": 3,
            "array" : [1,"test",true ],
            "popup": {
                "menuitem": "CreateNewDoc()",
                "name": "Create new document..."
            }
        }
    }   
)--" };

    {
        auto objs = JSON::parse(json_text3.begin(), json_text3.end());
        auto& obj = *objs[0];
        std::string s = obj["menu"]["popup"]["name"];
        std::stringstream ss;
        std::cout << obj << std::endl;
        ss << obj << std::endl;
        auto json_text_rb = ss.str();
        auto objs2 = JSON::parse(json_text_rb.begin(), json_text_rb.end());
        auto& obj2 = *objs2[0];
        std::cout << obj2 << std::endl;
    }
    return 0;
}
catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
}
catch (...) {
    return -1;
}
