#include <iostream>
#include <nlohmann_json.hpp>

// external modern json library (nlohmann/json)
using json = nlohmann::json;

int main() 
{
    json j = {
    {"name", "John"},
    {"age", "30"},
    {"cities", {"New York", "London"}}
    };

    std::cout << j.dump(4);
    return 0;
}
