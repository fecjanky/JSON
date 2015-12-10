#include <iostream>
#include <sstream>

#include "JSON.h"
#include "JSONParser.h"
#include "JSONIterator.h"




int main(int, char**) try{
///*
    std::string json_text3
    { u8R"--( 
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
  //*/
    /*
    std::string json_text3
    { u8R"--( 
    {
        "id": "file",
        "value": 3
    }   
)--" };
*/
    {
        auto objs = ::JSON::parse(json_text3.begin(), json_text3.end());
        auto objs2 = ::JSON::parse(json_text3.begin(), json_text3.end());
        
        bool eq = (*objs[0] == *objs2[0]);
        //std::string s = obj["menu"]["popup"]["name"];
        int i = 0;
        //JSON::IObject::iterator tit{};
        auto& obj = *objs[0];
        for (auto& x : obj) {
            std::cout << x << std::endl;
        }

        std::stringstream ss;

        /*std::cout << obj << std::endl;
        obj.serialize(std::cout);
        obj.serialize(ss);
        auto json_text_rb = ss.str();
        auto objs2 = ::JSON::parse(json_text_rb.begin(), json_text_rb.end());
        auto& obj2 = *objs2[0];
        std::cout << obj2 << std::endl;*/
    }

    return 0;
}
catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
}
catch (...) {
    return -1;
}
