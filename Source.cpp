#include <iostream>
#include <sstream>
#include <chrono>

#include "JSON.h"
#include "JSONParser.h"

class TestAllocator {
public:
    using value_type = uint8_t;

    TestAllocator(size_t size) : _size{ size }, memory{ new uint8_t[_size] }, free{ memory } {
    }

    ~TestAllocator() {
        delete[] memory;
    }

    TestAllocator(const TestAllocator& other) :_size{ other._size }, memory{ new uint8_t[_size] }, free{ memory } {
    }
    TestAllocator(TestAllocator&& other) noexcept:_size{}, memory{}, free{} {
        std::swap(_size, other._size);
        std::swap(memory, other.memory);
        std::swap(free, other.free);
    }

    TestAllocator& operator=(const TestAllocator& rhs) {
        if (this != &rhs) {
            tidy();
            _size = rhs._size;
            memory = free = new uint8_t[_size];
        }
        return *this;
    }

    size_t max_size()const noexcept {
        return std::numeric_limits<size_t>::max();
    }

    TestAllocator& operator=(TestAllocator&& rhs) noexcept{
        std::swap(_size, rhs._size);
        std::swap(memory, rhs.memory);
        std::swap(free, rhs.free);
        return *this;
    }

    uint8_t* allocate(size_t n,const uint8_t*) {
        if (free + n >= memory + _size) throw std::bad_alloc{};
        auto ret = free;
        free += n;
        return ret;
    }

    void deallocate(uint8_t *,size_t) {
    }

private:
    void tidy() noexcept{
        delete[] memory;
        free = memory = nullptr;
        _size = 0;
    }

    size_t _size;
    uint8_t* memory;
    uint8_t* free;

};

int main(int, char**)
try {
///*
    std::string json_text3 { u8R"--( 
    {
        "menu": {
            "id": "file",
            "value": 3.01e-12,
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
                auto test_allocator = estd::to_poly_allocator(TestAllocator(1024 * 1024 * 1024));
                //std::vector<JSON::IObject> objstorage;
                auto start = std::chrono::steady_clock::now();
                int loops = 0;
                while (std::chrono::steady_clock::now() - start < std::chrono::seconds{ 5 }) {
                    auto objs = ::JSON::parse(test_allocator,json_text3.begin(), json_text3.end());
                    loops++;
                }
                auto end = std::chrono::steady_clock::now();
                std::cout << loops << " " 
                    << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() 
                    << " ms" << std::endl;

                return 0;
                estd::poly_alloc_impl<std::allocator<uint8_t>> a;
                

                auto objs = ::JSON::parse(test_allocator,json_text3.begin(), json_text3.end());
                auto objs2 = ::JSON::parse(a,json_text3.begin(), json_text3.end());
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
                for (auto& x : obj["menu"]) {
                    std::cout << x << std::endl;
                }

                std::cout << obj << std::endl;
                obj["menu"]["id"] = a1;
                std::cout << obj << std::endl;
                
                for (auto i2 = obj.begin(); i2 != obj.end(); ++i2) {

                }

                for (auto poi = JSON::MakePreorderIterator(obj.begin()); poi ; ++poi) {
                    std::cout << *poi << std::endl;
                }


                std::vector<JSON::IObject::Ptr, estd::poly_alloc_wrapper<JSON::IObject::Ptr>> pv(a);
                //std::stringstream ss;

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
