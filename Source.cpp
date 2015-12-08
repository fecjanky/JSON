#include <iostream>
#include <sstream>

#include "JSON.h"
#include "JSONParser.h"




int main(int, char**) try{

    //auto p = JSON::Create<JSON::Number>(4.4255);
    //auto list = { JSON::Object::Entry{ "number",JSON::Create<JSON::Number>(4.4255) } , JSON::Object::Entry{"message",JSON::Create<JSON::String>("Hello")} };

    //JSON::Create<JSON::Object>(std::move(list));
    //JSON::Create<JSON::Number>("4.4255");
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
      
    std::wstring wjson { LR"--( 
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

    std::u16string ujson{ uR"--( 
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


    std::u32string Ujson{ UR"--( 
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
        auto objs = ::JSON::parse(json_text3.begin(), json_text3.end());
        auto wobjs = ::JSON::parse(wjson.begin(), wjson.end());

        auto& obj = *objs[0];
        auto& wobj = *wobjs[0];

        auto c = 'Ä';
        std::string ccu8 = u8"ÄÄ";
        std::string cc = "ÄÄ";
        std::locale my("");
        std::cout.imbue(my);
        std::cout << ccu8 << std::endl;
        std::cout << cc << std::endl;
        auto bo = cc == ccu8;

        std::string s = obj["menu"]["popup"]["name"];
        std::stringstream ss;
        std::wstringstream wss;

        std::cout << obj << std::endl;
        obj.serialize(std::cout);
        std::wcout << wobj << std::endl;
        obj.serialize(ss);
        wss << wobj << std::endl;
        auto json_text_rb = ss.str();
        auto wjson_text_rb = wss.str();
        auto objs2 = ::JSON::parse(json_text_rb.begin(), json_text_rb.end());
        auto wobjs2 = ::JSON::parse(wjson_text_rb.begin(), wjson_text_rb.end());
        auto& obj2 = *objs2[0];
        auto& wobj2 = *wobjs2[0];
        std::cout << obj2 << std::endl;
        std::wcout << wobj2 << std::endl;
        wobj2.serialize(std::wcout);
    }

    return 0;
}
catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
}
catch (...) {
    return -1;
}
