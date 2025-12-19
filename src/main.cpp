/*
#include "metadata_parser.hpp"
#include "search_engine.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"
#include "semantic_search.hpp"
#include "word_embeddings.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <memory>

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
    std::cout << "║   semantic <query>    Semantic search using word embeddings        ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║ CONFIGURATION                                                      ║\n";
    std::cout << "║   results <n>         Set max results to display (1-100)           ║\n";
    std::cout << "║   verbose on/off      Toggle detailed output                       ║\n";
    std::cout << "║   bm25 on/off         Toggle BM25 ranking (default: on)            ║\n";
    std::cout << "║                                                                    ║\n";
    std::cout << "║ INFORMATION                                                        ║\n";
    std::cout << "║   stats               Show last search statistics                  ║\n";
    std::cout << "║   info                Show system information                      ║\n";
    std::cout << "║   embeddings          Show embeddings status                       ║\n";
    std::cout << "║   check <word>        Check if word exists in lexicon/embeddings   ║\n";
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
    std::cout << "  semantic viral transmission patterns  # Semantic similarity search\n";
    std::cout << "  check virus                           # Check word in lexicon\n";
    std::cout << "  embeddings                            # Show embeddings status\n";
    std::cout << "  results 20                            # Show 20 results\n";
    std::cout << "  verbose on                            # Enable detailed output\n";
    std::cout << "\n";
}

void print_system_info(const Lexicon& lexicon, const ForwardIndex& forward_index,
                       WordEmbeddings* embeddings = nullptr) {
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
    std::cout << "    Available:           TF-IDF, BM25, Semantic Embeddings\n";
    std::cout << "    Current:             BM25 (configurable)\n";
    std::cout << "\n";
    
    if (embeddings) {
        std::cout << "  Semantic Features:\n";
        std::cout << "    Embedding Dimension: " << embeddings->get_dimension() << "\n";
        std::cout << "    Vocabulary Size:     " << embeddings->get_vocabulary_size() << "\n";
        std::cout << "    Status:               Ready for semantic search\n";
        std::cout << "\n";
    } else {
        std::cout << "  Semantic Features:\n";
        std::cout << "    Status:               Not available (embeddings not loaded)\n";
        std::cout << "\n";
    }
    
    std::cout << "  Query Features:\n";
    std::cout << "    Single-word:         Supported\n";
    std::cout << "    Multi-word (AND):    Supported\n";
    std::cout << "    Case-sensitive:      No (normalized)\n";
    std::cout << "    Semantic search:     " << (embeddings ? "Enabled" : "Disabled") << "\n";
    std::cout << "\n";
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void interactive_mode(SearchEngine& engine, SemanticSearchEngine* semantic_engine,
                     Lexicon& lexicon, ForwardIndex& forward_index) 
{
    std::string line;
    u32 max_results = 10;
    bool verbose = false;
    bool use_bm25 = true;
    
    std::vector<SearchResult> last_results;
    std::vector<SemanticSearchResult> last_semantic_results;
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
        else if (command == "semantic") {
            // Semantic search command
            if (!semantic_engine) {
                std::cout << "\033[1;31m✗ Error:\033[0m Semantic search is not available\n";
                std::cout << "  Word embeddings were not loaded successfully\n";
                std::cout << "  Check if embeddings.bin exists in the index directory\n";
                continue;
            }
            
            std::string query;
            std::getline(iss, query);
            
            // Trim leading whitespace
            size_t start = query.find_first_not_of(" \t");
            if (start != std::string::npos) {
                query = query.substr(start);
            }
            
            if (query.empty()) {
                std::cout << "\033[1;31m✗ Error:\033[0m Please provide a search query\n";
                std::cout << "  Usage: semantic <query>\n";
                std::cout << "  Example: semantic viral transmission patterns\n";
                continue;
            }
            
            last_query = query;
            std::cout << "\033[1;33m[Semantic Search Mode]\033[0m Processing query...\n";
            
            last_semantic_results = semantic_engine->semantic_search(query, max_results);
            semantic_engine->display_results(last_semantic_results, query, verbose);
            
            // Clear BM25 results to avoid confusion
            last_results.clear();
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
            if (last_results.empty() && last_semantic_results.empty()) {
                std::cout << "\033[1;33m!\033[0m No search performed yet\n";
                std::cout << "  Run a search first using: search <query> or semantic <query>\n";
            } else if (!last_results.empty()) {
                engine.print_search_stats(last_results, last_query_terms);
            } else if (!last_semantic_results.empty()) {
                // Show semantic search stats
                std::cout << "\n\033[1;36m=== SEMANTIC SEARCH STATISTICS ===\033[0m\n\n";
                std::cout << "Query: \"" << last_query << "\"\n";
                std::cout << "Results: " << last_semantic_results.size() << "\n";
                
                if (!last_semantic_results.empty()) {
                    float max_score = last_semantic_results[0].semantic_score;
                    float min_score = last_semantic_results.back().semantic_score;
                    float avg_score = 0.0f;
                    
                    for (const auto& result : last_semantic_results) {
                        avg_score += result.semantic_score;
                    }
                    avg_score /= last_semantic_results.size();
                    
                    std::cout << "Score range: " << min_score << " - " << max_score << "\n";
                    std::cout << "Average score: " << avg_score << "\n";
                }
            }
        }
        else {
            std::cout << "\033[1;31m✗ Unknown command:\033[0m '" << command << "'\n";
            std::cout << "  Type '\033[1mhelp\033[0m' for available commands\n";
        }
        
        // Display execution time for search commands
        if (command == "search" || command == "s" || command == "semantic") {
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
    
    std::cout << " Index Path: " << index_path << "\n";
    std::cout << " Loading indices...\n\n";
    
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
        
        // TODO: Load word embeddings and initialize semantic search
        std::unique_ptr<WordEmbeddings> word_embeddings;
        std::unique_ptr<SemanticSearchEngine> semantic_engine;
        
        std::cout << "  Loading word embeddings... ";
        start = std::chrono::high_resolution_clock::now();
        word_embeddings = std::make_unique<WordEmbeddings>();
        if (word_embeddings->load_embeddings_binary(index_path + "glove.6B.100d.bin")) {
            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "\033[1;32m✓\033[0m (" << word_embeddings->get_vocabulary_size() 
                      << " embeddings, " << duration.count() << " ms)\n";
            
            // Initialize semantic search engine
            semantic_engine = std::make_unique<SemanticSearchEngine>(
                *word_embeddings, search_engine, lexicon, forward_index);
        } else {
            std::cout << "\033[1;31m✗\033[0m (embeddings.bin not found)\n";
            std::cout << "  Note: Semantic search will be disabled\n";
        }
        
        std::cout << "\n";
        std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║               Search Engine Ready!                               ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
        
        // Check for command-line query (batch mode)
        if (argc > 2) {
            // Batch mode: search and exit
            std::string query;
            for (int i = 2; i < argc; i++) {
                if (i > 2) { query += " "; }
                query += argv[i];
            }
            
            std::cout << "\n Batch mode query: \"" << query << "\"\n";
            
            // Check if it's a semantic query
            std::string cmd = argv[2];
            if (cmd == "semantic" && argc > 3 && semantic_engine) {
                // Semantic search
                query = "";
                for (int i = 3; i < argc; i++) {
                    if (i > 3) { query += " "; }
                    query += argv[i];
                }
                auto results = semantic_engine->semantic_search(query, 10);
                semantic_engine->display_results(results, query, false);
            } else {
                // Regular BM25 search
                auto results = search_engine.search(query, 10, true);
                search_engine.display_results(results, query, false);
            }
            
            return 0;
        }
        
        // Interactive mode
        interactive_mode(search_engine, semantic_engine.get(), lexicon, forward_index);
        
    } catch (const std::exception& e) {
        std::cerr << "\n\033[1;31m✗ Fatal Error:\033[0m " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
*/

#include "search_engine.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"
#include "semantic_search.hpp"
#include "word_embeddings.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <memory>
#include <cstdlib>
#include <algorithm>
#include "nlohmann_json.hpp"

using json = nlohmann::json;

// Global variables for the search engine
std::unique_ptr<Lexicon> lexicon;
std::unique_ptr<ForwardIndex> forward_index;
std::unique_ptr<InvertedIndex> inverted_index;
std::unique_ptr<SearchEngine> search_engine;
std::unique_ptr<WordEmbeddings> word_embeddings;
std::unique_ptr<SemanticSearchEngine> semantic_engine;

// Initialize the search engine
bool initialize_engine(const std::string& index_path) 
{
    try {
        std::cout << "Loading indices from: " << index_path << std::endl;
        
        // Load lexicon
        lexicon = std::make_unique<Lexicon>();
        if (!lexicon->load_from_file_binary(index_path + "lexicon_cordR1.bin")) {
            std::cerr << "Failed to load lexicon" << std::endl;
            return false;
        }
        std::cout << " Loaded lexicon with " << lexicon->size() << " terms" << std::endl;
        
        // Load forward index
        forward_index = std::make_unique<ForwardIndex>();
        if (!forward_index->load_from_file(index_path + "forward_index_cordR1.bin")) {
            std::cerr << "Failed to load forward index" << std::endl;
            return false;
        }
        std::cout << " Loaded forward index with " << forward_index->size() << " documents" << std::endl;
        
        // Load inverted index
        inverted_index = std::make_unique<InvertedIndex>(index_path);
        std::cout << " Loaded inverted index (barrel system)" << std::endl;
        
        // Initialize search engine
        search_engine = std::make_unique<SearchEngine>(*lexicon, *forward_index, *inverted_index);
        
        // Try to load word embeddings
        word_embeddings = std::make_unique<WordEmbeddings>();
        if (word_embeddings->load_embeddings_binary(index_path + "glove.6B.100d.bin")) {
            std::cout << " oaded word embeddings" << std::endl;
            semantic_engine = std::make_unique<SemanticSearchEngine>(
                *word_embeddings, *search_engine, *lexicon, *forward_index);
        } else {
            std::cout << " Word embeddings not available" << std::endl;
        }
        
        std::cout << " Search engine initialized successfully!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error initializing engine: " << e.what() << std::endl;
        return false;
    }
}

// Format search results as JSON
json format_results(const std::vector<SearchResult>& results, 
                    const std::string& query, 
                    long long time_ms) {
    json j;
    j["success"] = true;
    j["query"] = query;
    j["time_ms"] = time_ms;
    j["count"] = results.size();
    
    json results_array = json::array();
    for (const auto& result : results) {
        json r;
        r["docId"] = result.doc_id;
        r["score"] = result.score;
        
        // Try to get document metadata
        const DocumentMetadata* meta = forward_index->get_document_metadata(result.doc_id);
        if (meta) {
            r["cord_uid"] = meta->cord_uid;
            r["title"] = meta->title;
            r["abstract"] = meta->abstract.substr(0, 200) + (meta->abstract.length() > 200 ? "..." : "");
            r["pmcid"] = meta->pmcid;
        } else {
            r["title"] = "Document " + std::to_string(result.doc_id);
            r["cord_uid"] = "";
            r["abstract"] = "";
            r["pmcid"] = "";
        }
        
        // Add snippet from first term frequency
        if (!result.term_frequencies.empty()) {
            auto it = result.term_frequencies.begin();
            std::string* word = lexicon->get_word(it->first);
            if (word) {
                r["snippet"] = "Contains '" + *word + "' (" + std::to_string(it->second) + " occurrences)";
            }
        } else {
            r["snippet"] = "No term information available";
        }
        
        results_array.push_back(r);
    }
    j["results"] = results_array;
    
    return j;
}

// Search command
void handle_search(const std::string& query, int max_results = 10) 
{
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<SearchResult> results = search_engine->search(query, max_results, true);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Duration: " << duration << '\n';
    std::cout.flush();
    
    json output = format_results(results, query, duration.count());
    std::cout << output.dump(2) << '\n';
}

// Suggestions command
void handle_suggest(const std::string& query) {
    // Simple suggestion algorithm based on lexicon
    json j;
    j["success"] = true;
    
    std::vector<std::string> suggestions;
    
    // Get all words from lexicon
    const auto& lex = lexicon->get_lexicon();
    
    // Filter words that start with the query
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    for (const auto& [word, data] : lex) {
        std::string word_lower = word;
        std::transform(word_lower.begin(), word_lower.end(), word_lower.begin(), ::tolower);
        
        if (word_lower.find(query_lower) == 0 && word_lower.length() > query_lower.length()) {
            suggestions.push_back(word);
            if (suggestions.size() >= 10) break;
        }
    }
    
    // Add COVID-19 related suggestions if not enough
    if (suggestions.size() < 5) {
        std::vector<std::string> covid_terms = {
            "covid", "covid-19", "coronavirus", "vaccine", "treatment",
            "pandemic", "sars", "mers", "infection", "virus",
            "clinical trial", "research", "study", "paper", "document"
        };
        
        for (const auto& term : covid_terms) {
            if (term.find(query_lower) != std::string::npos || 
                query_lower.find(term) != std::string::npos) {
                suggestions.push_back(term);
            }
            if (suggestions.size() >= 10) break;
        }
    }
    
    // Remove duplicates
    std::sort(suggestions.begin(), suggestions.end());
    suggestions.erase(std::unique(suggestions.begin(), suggestions.end()), suggestions.end());
    
    j["data"] = suggestions;
    std::cout << j.dump(2) << std::endl;
}

// Document command
void handle_document(int doc_id) {
    json j;
    
    const DocumentMetadata* meta = forward_index->get_document_metadata(doc_id);
    if (meta != nullptr) {
        j["success"] = true;
        j["docId"] = doc_id;
        j["cord_uid"] = meta->cord_uid;
        j["title"] = meta->title;
        j["abstract"] = meta->abstract;
        j["pmcid"] = meta->pmcid;
        
        // Get document terms
        const std::vector<WordData>* terms = forward_index->get_document_terms(doc_id);
        if (terms != nullptr) {
            json terms_array = json::array();
            for (const auto& term : *terms) {
                std::string* word = lexicon->get_word(term.word_id);
                if (word != nullptr) {
                    json t;
                    t["word"] = *word;
                    t["freq"] = term.freq;
                    terms_array.push_back(t);
                }
            }
            j["terms"] = terms_array;
            j["total_words"] = forward_index->get_total_words(doc_id);
        }
    } else {
        j["success"] = false;
        j["error"] = "Document not found";
        j["docId"] = doc_id;
    }
    
    std::cout << j.dump(2) << '\n';
}

// Test command
void handle_test() {
    json j;
    j["success"] = true;
    j["status"] = "ready";
    j["documents"] = forward_index->size();
    j["terms"] = lexicon->size();
    j["service"] = "CORD-19 Search Engine";
    j["version"] = "1.0.0";
    
    std::cout << j.dump(2) << '\n';
}

// Health command
void handle_health() {
    json j;
    j["success"] = true;
    j["status"] = "online";
    j["documents"] = forward_index->size();
    j["terms"] = lexicon->size();
    j["embeddings_loaded"] = (semantic_engine != nullptr);
    j["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
    
    std::cout << j.dump(2) << '\n';
}

// Interactive mode (original)
void interactive_mode() {
    std::string line;
    u32 max_results = 10;
    
    std::cout << "\n🔍 CORD-19 Search Engine Ready!\n";
    std::cout << "Type 'search <query>' to search or 'help' for commands\n\n";
    
    while (true) {
        std::cout << "search> ";
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
        
        if (command == "quit" || command == "exit") {
            break;
        }
        else if (command == "help") {
            std::cout << "\nCommands:\n";
            std::cout << "  search <query>     - Search for papers\n";
            std::cout << "  suggest <query>    - Get search suggestions\n";
            std::cout << "  doc <id>          - Get document details\n";
            std::cout << "  health            - Check system health\n";
            std::cout << "  test              - Test connection\n";
            std::cout << "  quit/exit         - Exit program\n\n";
        }
        else if (command == "search") {
            std::string query;
            std::getline(iss, query);
            
            // Trim leading whitespace
            size_t start = query.find_first_not_of(" \t");
            if (start != std::string::npos) {
                query = query.substr(start);
            }
            
            if (query.empty()) {
                std::cout << "Error: Please provide a search query\n";
                continue;
            }
            
            handle_search(query, max_results);
        }
        else if (command == "suggest") {
            std::string query;
            std::getline(iss, query);
            
            // Trim leading whitespace
            size_t start = query.find_first_not_of(" \t");
            if (start != std::string::npos) {
                query = query.substr(start);
            }
            
            handle_suggest(query);
        }
        else if (command == "doc") {
            int doc_id;
            if (iss >> doc_id) {
                handle_document(doc_id);
            } else {
                std::cout << "Error: Please provide a document ID\n";
            }
        }
        else if (command == "health") {
            handle_health();
        }
        else if (command == "test") {
            handle_test();
        }
        else {
            std::cout << "Unknown command: '" << command << "'\n";
            std::cout << "Type 'help' for available commands\n";
        }
    }
}

int main(int argc, char* argv[]) 
{
    // Default index path
    std::string index_path = "indices/";
    
    // Check for index path argument
    if (argc > 1) {
        index_path = argv[1];
        if (index_path.back() != '/') {
            index_path += '/';
        }
    }
    
    // Initialize the search engine
    if (!initialize_engine(index_path)) {
        std::cerr << "Failed to initialize search engine. Exiting." << '\n';
        return 1;
    }
    
    // Check command line arguments for web mode
    if (argc > 2) {
        std::string command = argv[2];
        
        if (command == "search" && argc > 3) {

            auto start = std::chrono::high_resolution_clock::now();

            // Web search: main search <query>
            std::string query;
            for (int i = 3; i < argc; i++) {
                if (i > 3) { query += " "; }
                query += argv[i];
            }
            handle_search(query);

            auto end = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration<double>(end - start).count();

            std::cout << "\nTIME FOR QUERY: " << duration << '\n';
        }
        else if (command == "suggest") {
            // Web suggestions: main suggest <query>
            std::string query = (argc > 3) ? argv[3] : "";
            handle_suggest(query);
        }
        else if (command == "doc" && argc > 3) {
            // Web document: main doc <id>
            try {
                int doc_id = std::stoi(argv[3]);
                handle_document(doc_id);
            } catch (...) {
                json j;
                j["success"] = false;
                j["error"] = "Invalid document ID";
                std::cout << j.dump(2) << '\n';
            }
        }
        else if (command == "test") {
            // Web test: main test
            handle_test();
        }
        else if (command == "health") {
            // Web health: main health
            handle_health();
        }
        else {
            // Unknown command or interactive mode
            if (argc == 2 && std::string(argv[1]) == "--interactive") {
                interactive_mode();
            } else {
                // Print usage
                std::cout << "Usage:\n";
                std::cout << "  " << argv[0] << " [index_path] --interactive  # Interactive mode\n";
                std::cout << "  " << argv[0] << " [index_path] search <query> # Search (web mode)\n";
                std::cout << "  " << argv[0] << " [index_path] suggest <query># Suggestions (web mode)\n";
                std::cout << "  " << argv[0] << " [index_path] doc <id>      # Get document (web mode)\n";
                std::cout << "  " << argv[0] << " [index_path] test          # Test (web mode)\n";
                std::cout << "  " << argv[0] << " [index_path] health        # Health check (web mode)\n";
                return 1;
            }
        }
    } else {
        // No arguments, start interactive mode
        interactive_mode();
    }

    
    return 0;
}


