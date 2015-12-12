﻿#include <iostream>
#include <sstream>

#include "JSON.h"
#include "JSONParser.h"
#include "JSONIterator.h"

int main(int, char**)
try {
///*
    std::string json_text3 { u8R"--( 
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
                auto o1 = JSON::Create<JSON::String>("Hello");
                auto o2 = JSON::Create<JSON::String>("Hello");
                auto a1 = JSON::Create<JSON::Array>();
                auto a2 = JSON::Create<JSON::Array>();
                auto& a1r = static_cast<JSON::Array&>(*a1);
                auto& a2r = static_cast<JSON::Array&>(*a2);
                a1r.emplace(JSON::Create<JSON::String>("Hello"));
                a1r.emplace(JSON::Create<JSON::String>("World"));
                a2r.emplace(JSON::Create<JSON::String>("Helloa"));
                a2r.emplace(JSON::Create<JSON::String>("World"));
                bool eqa = *a1 == *a2;

                bool eqs = *o1 == *o2;
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
