#include "search_engine.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>

void print_banner() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         CORD-19 Search Engine Server v1.0                          ║\n";
    std::cout << "║         COVID-19 Research Paper Search System                      ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
}

void print_help() {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                          AVAILABLE COMMANDS                        ║\n";
    std::cout << "╠════════════════════════════════════════════════════════════════════╣\n";
    std::cout << "║ SEARCH COMMANDS                                                    ║\n";
    std::cout << "║   search <query>      Search for documents (multi-word supported)  ║\n";
    std::cout << "║   s <query>           Short form of search                         ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║ CONFIGURATION                                                      ║\n";
    std::cout << "║   results <n>         Set max results to display (1-100)           ║\n";
    std::cout << "║   verbose on/off      Toggle detailed output                       ║\n";
    std::cout << "║   bm25 on/off         Toggle BM25 ranking (default: on)            ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║ INFORMATION                                                        ║\n";
    std::cout << "║   stats               Show last search statistics                  ║\n";
    std::cout << "║   info                Show system information                      ║\n";
    std::cout << "║   help                Show this help message                       ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║ SYSTEM                                                             ║\n";
    std::cout << "║   clear               Clear screen                                 ║\n";
    std::cout << "║   quit / exit         Exit the search engine                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  search covid                          # Single-word search\n";
    std::cout << "  search coronavirus vaccine            # Multi-word AND search\n";
    std::cout << "  s sars transmission symptoms          # Short form\n";
    std::cout << "  results 20                            # Show 20 results\n";
    std::cout << "  verbose on                            # Enable detailed output\n";
    std::cout << "\n";
}

void print_system_info(const Lexicon& lexicon, const ForwardIndex& forward_index) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                        SYSTEM INFORMATION                          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "  Index Statistics:\n";
    std::cout << "    Total Documents:     " << forward_index.get_document_count() << "\n";
    std::cout << "    Unique Terms:        " << lexicon.size() << "\n";
    std::cout << "    Barrel System:       Enabled (4 barrels)\n";
    std::cout << "\n";
    std::cout << "  Ranking Algorithms:\n";
    std::cout << "    Available:           TF-IDF, BM25\n";
    std::cout << "    Current:             BM25 (configurable)\n";
    std::cout << "\n";
    std::cout << "  Query Features:\n";
    std::cout << "    Single-word:         Supported\n";
    std::cout << "    Multi-word (AND):    Supported\n";
    std::cout << "    Case-sensitive:      No (normalized)\n";
    std::cout << "\n";
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void interactive_mode(SearchEngine& engine, Lexicon& lexicon, ForwardIndex& forward_index) 
{
    std::string line;
    u32 max_results = 10;
    bool verbose = false;
    bool use_bm25 = true;
    
    std::vector<SearchResult> last_results;
    std::string last_query;
    std::vector<QueryTerm> last_query_terms;
    
    print_help();
    
    while (true) {
        std::cout << "\n\033[1;36msearch>\033[0m ";
        std::cout.flush();
        
        if (!std::getline(std::cin, line)) {
            break;
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        if (line.empty()) { continue; }
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        // Convert command to lowercase
        std::transform(command.begin(), command.end(), command.begin(), ::tolower);
        
        // Measure command execution time
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (command == "quit" || command == "exit" || command == "q") {
            break;
        }
        else if (command == "help" || command == "h" || command == "?") {
            print_help();
        }
        else if (command == "info" || command == "i") {
            print_system_info(lexicon, forward_index);
        }
        else if (command == "clear" || command == "cls") {
            clear_screen();
            print_banner();
        }
        else if (command == "search" || command == "s") {
            // Get rest of line as query
            std::string query;
            std::getline(iss, query);
            
            // Trim leading whitespace
            size_t start = query.find_first_not_of(" \t");
            if (start != std::string::npos) {
                query = query.substr(start);
            }
            
            if (query.empty()) {
                std::cout << "\033[1;31m✗ Error:\033[0m Please provide a search query\n";
                std::cout << "  Usage: search <query>\n";
                std::cout << "  Example: search covid vaccine\n";
                continue;
            }
            
            last_query = query;
            last_results = engine.search(query, max_results, use_bm25);
            engine.display_results(last_results, query, verbose);
            
            // Store query terms for stats
            last_query_terms.clear();
            std::istringstream query_iss(query);
            std::string word;
            while (query_iss >> word) {
                QueryTerm term;
                term.word = word;
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                term.word_id = lexicon.get_word_id(word);
                term.found = (term.word_id != UINT32_MAX);
                if (term.found) {
                    term.doc_freq = engine.get_inverted_index().get_total_docs(term.word_id);
                }
                last_query_terms.push_back(term);
            }
        }
        else if (command == "results") {
            u32 n;
            if (iss >> n && n > 0 && n <= 100) {
                max_results = n;
                std::cout << "\033[1;32m✓\033[0m Max results set to \033[1m" << max_results << "\033[0m\n";
            } else {
                std::cout << "\033[1;31m✗ Error:\033[0m Please provide a valid number (1-100)\n";
                std::cout << "  Usage: results <number>\n";
                std::cout << "  Example: results 20\n";
            }
        }
        else if (command == "verbose" || command == "v") {
            std::string mode;
            iss >> mode;
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            
            if (mode == "on" || mode == "true" || mode == "1") {
                verbose = true;
                std::cout << "\033[1;32m✓\033[0m Verbose mode \033[1menabled\033[0m\n";
            } else if (mode == "off" || mode == "false" || mode == "0") {
                verbose = false;
                std::cout << "\033[1;32m✓\033[0m Verbose mode \033[1mdisabled\033[0m\n";
            } else {
                std::cout << "\033[1;31m✗ Error:\033[0m Invalid mode\n";
                std::cout << "  Usage: verbose on/off\n";
            }
        }
        else if (command == "bm25") {
            std::string mode;
            iss >> mode;
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
            
            if (mode == "on" || mode == "true" || mode == "1") {
                use_bm25 = true;
                std::cout << "\033[1;32m✓\033[0m BM25 ranking \033[1menabled\033[0m\n";
            } else if (mode == "off" || mode == "false" || mode == "0") {
                use_bm25 = false;
                std::cout << "\033[1;32m✓\033[0m BM25 ranking \033[1mdisabled\033[0m (using TF-IDF)\n";
            } else {
                std::cout << "\033[1;31m✗ Error:\033[0m Invalid mode\n";
                std::cout << "  Usage: bm25 on/off\n";
            }
        }
        else if (command == "stats") {
            if (last_results.empty()) {
                std::cout << "\033[1;33m!\033[0m No search performed yet\n";
                std::cout << "  Run a search first using: search <query>\n";
            } else {
                engine.print_search_stats(last_results, last_query_terms);
            }
        }
        else {
            std::cout << "\033[1;31m✗ Unknown command:\033[0m '" << command << "'\n";
            std::cout << "  Type '\033[1mhelp\033[0m' for available commands\n";
        }
        
        // Display execution time for search commands
        if (command == "search" || command == "s") {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "\n⏱  Query completed in " << duration.count() << " ms\n";
        }
    }
}

int main(int argc, char* argv[]) 
{
    print_banner();
    
    // Determine index path
    std::string index_path = "indices/";
    if (argc > 1) {
        index_path = argv[1];
        if (index_path.back() != '/') {
            index_path += '/';
        }
    }
    
    std::cout << "📂 Index Path: " << index_path << "\n";
    std::cout << "⏳ Loading indices...\n\n";
    
    try {
        // Load lexicon
        Lexicon lexicon;
        std::cout << "  [1/3] Loading lexicon... ";
        std::cout.flush();
        
        auto start = std::chrono::high_resolution_clock::now();
        if (!lexicon.load_from_file_binary(index_path + "lexicon_cordR1.bin")) {
            std::cerr << "\n\033[1;31m✗ Failed to load lexicon\033[0m\n";
            std::cerr << "  Check if file exists: " << index_path << "lexicon_cordR1.bin\n";
            return 1;
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\033[1;32m✓\033[0m (" << lexicon.size() << " terms, " 
                  << duration.count() << " ms)\n";
        
        // Load forward index
        ForwardIndex forward_index;
        std::cout << "  [2/3] Loading forward index... ";
        std::cout.flush();
        
        start = std::chrono::high_resolution_clock::now();
        if (!forward_index.load_from_file(index_path + "forward_index_cordR1.bin")) {
            std::cerr << "\n\033[1;31m✗ Failed to load forward index\033[0m\n";
            std::cerr << "  Check if file exists: " << index_path << "forward_index_cordR1.bin\n";
            return 1;
        }
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\033[1;32m✓\033[0m (" << forward_index.size() << " documents, "
                  << duration.count() << " ms)\n";
        
        // Load inverted index (with barrel support)
        std::cout << "  [3/3] Loading inverted index... ";
        std::cout.flush();
        
        start = std::chrono::high_resolution_clock::now();
        InvertedIndex inverted_index(index_path);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "\033[1;32m✓\033[0m (barrel system ready, "
                  << duration.count() << " ms)\n";
        
        // Initialize search engine
        std::cout << "\n🔍 Initializing search engine...\n";
        SearchEngine search_engine(lexicon, forward_index, inverted_index);
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              🚀 Search Engine Ready!                               ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
        
        // Check for command-line query (batch mode)
        if (argc > 2) {
            // Batch mode: search and exit
            std::string query;
            for (int i = 2; i < argc; i++) {
                if (i > 2) { query += " "; }
                query += argv[i];
            }
            
            std::cout << "\n📋 Batch mode query: \"" << query << "\"\n";
            auto results = search_engine.search(query, 10, true);
            search_engine.display_results(results, query, false);
            
            return 0;
        }
        
        // Interactive mode
        interactive_mode(search_engine, lexicon, forward_index);
        
    } catch (const std::exception& e) {
        std::cerr << "\n\033[1;31m✗ Fatal Error:\033[0m " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}


// INT MAIN FOR MAKING INDICES

/*
int main() 
{
    
    MetadataParser parser("data/2020-04-10");

    parser.metadata_parse();
    

    return 0;
}
*/


