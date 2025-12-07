#include "search_api.hpp"


SearchAPI::SearchAPI(QueryProcessor& qp,
                     ForwardIndex& fwd_idx,
                     Lexicon& lex)
    : query_processor(qp), forward_index(fwd_idx), lexicon(lex)
{}


json SearchAPI::results_to_json(const std::vector<SearchResult>& results,
                                const std::string& query) 
{
    json response;
    response["status"] = "success";
    response["query"] = query;
    response["total_results"] = results.size();
    
    json results_array = json::array();
    
    for (const auto& result : results) {
        json doc;
        doc["doc_id"] = result.doc_id;
        doc["cord_uid"] = result.cord_uid;
        doc["score"] = result.score;
        
        // Get document terms for preview
        const std::vector<WordData>* terms = 
            forward_index.get_document_terms(result.doc_id);
        
        if (terms && !terms->empty()) {
            // Create a preview snippet (first 100 words)
            std::string preview;
            u32 term_count = 0;
            for (const auto& term_data : *terms) {
                if (term_count >= 100) break;
                
                std::string* word = lexicon.get_word(term_data.word_id);
                if (word) {
                    preview += *word + " ";
                    term_count++;
                }
            }
            
            if (terms->size() > 100) {
                preview += "...";
            }
            
            doc["preview"] = preview;
            doc["term_count"] = terms->size();
        } else {
            doc["preview"] = "";
            doc["term_count"] = 0;
        }
        
        results_array.push_back(doc);
    }
    
    response["results"] = results_array;
    
    return response;
}


json SearchAPI::get_document_json(u32 doc_id) 
{
    json doc;
    
    // Get cord_uid
    const std::string* cord_uid = forward_index.get_doc_cord_uid(doc_id);
    if (!cord_uid) {
        doc["status"] = "error";
        doc["message"] = "Document not found";
        return doc;
    }
    
    doc["status"] = "success";
    doc["doc_id"] = doc_id;
    doc["cord_uid"] = *cord_uid;
    
    // Get all terms
    const std::vector<WordData>* terms = 
        forward_index.get_document_terms(doc_id);
    
    if (terms) {
        doc["term_count"] = terms->size();
        
        // Reconstruct full text
        std::string full_text;
        for (const auto& term_data : *terms) {
            std::string* word = lexicon.get_word(term_data.word_id);
            if (word) {
                full_text += *word + " ";
            }
        }
        doc["full_text"] = full_text;
        
        // Top terms in this document
        json top_terms = json::array();
        
        // Create vector for sorting
        std::vector<std::pair<u32, u32>> term_freq_pairs;
        for (const auto& term_data : *terms) {
            term_freq_pairs.push_back({term_data.word_id, term_data.freq});
        }
        
        // Sort by frequency
        std::sort(term_freq_pairs.begin(), term_freq_pairs.end(),
                 [](const auto& a, const auto& b) {
                     return a.second > b.second;
                 });
        
        // Get top 20
        for (size_t i = 0; i < std::min(size_t(20), term_freq_pairs.size()); ++i) {
            std::string* word = lexicon.get_word(term_freq_pairs[i].first);
            if (word) {
                json term_obj;
                term_obj["word"] = *word;
                term_obj["frequency"] = term_freq_pairs[i].second;
                top_terms.push_back(term_obj);
            }
        }
        
        doc["top_terms"] = top_terms;
    } else {
        doc["term_count"] = 0;
        doc["full_text"] = "";
        doc["top_terms"] = json::array();
    }
    
    return doc;
}


std::string SearchAPI::handle_search(const std::string& json_request) 
{
    try {
        json request = json::parse(json_request);
        
        // Extract parameters
        std::string query = request.value("query", "");
        u32 top_k = request.value("top_k", 10);
        
        if (query.empty()) {
            json error;
            error["status"] = "error";
            error["message"] = "Empty query";
            return error.dump();
        }
        
        // Perform search
        std::vector<SearchResult> results = query_processor.search(query, top_k);
        
        // Convert to JSON
        json response = results_to_json(results, query);
        
        return response.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["status"] = "error";
        error["message"] = std::string("Search failed: ") + e.what();
        return error.dump();
    }
}


std::string SearchAPI::handle_get_document(const std::string& json_request) 
{
    try {
        json request = json::parse(json_request);
        
        u32 doc_id = request.value("doc_id", UINT32_MAX);
        
        if (doc_id == UINT32_MAX) {
            json error;
            error["status"] = "error";
            error["message"] = "Invalid doc_id";
            return error.dump();
        }
        
        json doc = get_document_json(doc_id);
        return doc.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["status"] = "error";
        error["message"] = std::string("Failed to get document: ") + e.what();
        return error.dump();
    }
}


std::string SearchAPI::handle_get_stats() 
{
    try {
        json stats;
        stats["status"] = "success";
        
        const CollectionStats& col_stats = query_processor.get_stats();
        
        stats["total_documents"] = col_stats.total_docs;
        stats["average_document_length"] = col_stats.avg_doc_length;
        stats["total_unique_terms"] = lexicon.size();
        
        return stats.dump();
        
    } catch (const std::exception& e) {
        json error;
        error["status"] = "error";
        error["message"] = std::string("Failed to get stats: ") + e.what();
        return error.dump();
    }
}


std::string SearchAPI::handle_health_check() 
{
    json health;
    health["status"] = "healthy";
    health["message"] = "Search engine is running";
    return health.dump();
}
