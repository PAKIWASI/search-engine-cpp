#pragma once

#include "query_processor.hpp"
#include "nlohmann_json.hpp"

#include <string>

using json = nlohmann::json;

// API interface for web server communication
class SearchAPI {
private:
    QueryProcessor& query_processor;
    ForwardIndex& forward_index;
    Lexicon& lexicon;
    
    // Convert search results to JSON
    json results_to_json(const std::vector<SearchResult>& results,
                        const std::string& query);
    
    // Get document details for a specific doc_id
    json get_document_json(u32 doc_id);

public:
    SearchAPI(QueryProcessor& qp, 
              ForwardIndex& fwd_idx,
              Lexicon& lex);
    
    // Handle search request (JSON in, JSON out)
    std::string handle_search(const std::string& json_request);
    
    // Handle document details request
    std::string handle_get_document(const std::string& json_request);
    
    // Get system statistics
    std::string handle_get_stats();
    
    // Health check
    std::string handle_health_check();
};
