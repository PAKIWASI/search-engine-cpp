#include "search_api.hpp"
#include "query_processor.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"
#include "text_processor.hpp"

#include <iostream>
#include <string>

// Communication protocol:
// Node.js sends: {"command": "search", "data": {...}}
// C++ responds: {"status": "success", "data": {...}}

int main(int argc, char* argv[]) {
    
    std::string indices_path = "indices/";
    
    if (argc > 1) {
        indices_path = argv[1];
    }
    
    std::cerr << "=== COVID-19 Search Engine Starting ===\n";
    std::cerr << "Loading indices from: " << indices_path << "\n";
    
    try {
        // Initialize components
        std::cerr << "Loading lexicon...\n";
        Lexicon lexicon;
        if (!lexicon.load_from_file_binary(indices_path + "lexicon_cordR1.bin")) {
            std::cerr << "Failed to load lexicon\n";
            return 1;
        }
        
        std::cerr << "Loading forward index...\n";
        ForwardIndex forward_index;
        if (!forward_index.load_from_file(indices_path + "forward_index_cordR1.bin")) {
            std::cerr << "Failed to load forward index\n";
            return 1;
        }
        
        std::cerr << "Loading inverted index (barrel support)...\n";
        InvertedIndex inverted_index(indices_path);
        
        std::cerr << "Initializing text processor...\n";
        TextProcessor text_processor(
            lexicon,
            "python/.venv/bin/python3",
            "python/lemmatizer_daemon.py"
        );
        
        std::cerr << "Initializing query processor...\n";
        QueryProcessor query_processor(
            lexicon,
            forward_index,
            inverted_index,
            text_processor
        );
        
        std::cerr << "Initializing API...\n";
        SearchAPI api(query_processor, forward_index, lexicon);
        
        std::cerr << "=== Search Engine Ready ===\n";
        std::cerr << "Waiting for commands on stdin...\n\n";
        
        // Main command loop
        std::string line;
        while (std::getline(std::cin, line)) {
            
            if (line.empty()) continue;
            
            try {
                json request = json::parse(line);
                std::string command = request.value("command", "");
                std::string data = request.value("data", "{}");
                
                std::string response;
                
                if (command == "search") {
                    response = api.handle_search(data);
                }
                else if (command == "get_document") {
                    response = api.handle_get_document(data);
                }
                else if (command == "stats") {
                    response = api.handle_get_stats();
                }
                else if (command == "health") {
                    response = api.handle_health_check();
                }
                else if (command == "shutdown") {
                    std::cerr << "Shutdown command received\n";
                    break;
                }
                else {
                    json error;
                    error["status"] = "error";
                    error["message"] = "Unknown command: " + command;
                    response = error.dump();
                }
                
                // Send response
                std::cout << response << '\n';
                std::cout.flush();
                
            } catch (const std::exception& e) {
                json error;
                error["status"] = "error";
                error["message"] = std::string("Request error: ") + e.what();
                std::cout << error.dump() << '\n';
                std::cout.flush();
            }
        }
        
        std::cerr << "Search engine shutting down...\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}


