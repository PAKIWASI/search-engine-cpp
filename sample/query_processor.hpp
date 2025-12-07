#pragma once

#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"
#include "text_processor.hpp"

#include <string>
#include <vector>
#include <unordered_map>

// Result structure for a single document
struct SearchResult {
    u32 doc_id;
    std::string cord_uid;
    double score;
    
    // Constructor
    SearchResult(u32 id, const std::string& uid, double s)
        : doc_id(id), cord_uid(uid), score(s) {}
    
    // For sorting by score (descending)
    bool operator<(const SearchResult& other) const {
        return score > other.score;  // Higher scores first
    }
};

// Statistics needed for ranking
struct CollectionStats {
    u32 total_docs;
    double avg_doc_length;
    std::unordered_map<u32, u32> doc_lengths;  // doc_id -> length
};


class QueryProcessor {
private:
    Lexicon& lexicon;
    ForwardIndex& forward_index;
    InvertedIndex& inverted_index;
    TextProcessor& text_processor;
    
    CollectionStats stats;
    
    // BM25 parameters
    const double k1 = 1.2;      // term frequency saturation
    const double b = 0.75;      // length normalization
    
    // Precompute collection statistics
    void compute_collection_stats();
    
    // BM25 scoring function
    double compute_bm25_score(
        u32 term_freq_in_doc,
        u32 doc_length,
        u32 docs_with_term,
        u32 total_docs,
        double avg_doc_length
    ) const;
    
    // Process query text into term IDs
    bool process_query(const std::string& query_text,
                      std::vector<u32>& query_term_ids);
    
    // Get candidate documents for query terms
    void get_candidate_documents(
        const std::vector<u32>& query_term_ids,
        std::unordered_map<u32, std::vector<std::pair<u32, u32>>>& candidates
    );
    
    // Score all candidate documents
    void score_documents(
        const std::vector<u32>& query_term_ids,
        const std::unordered_map<u32, std::vector<std::pair<u32, u32>>>& candidates,
        std::vector<SearchResult>& results
    );

public:
    QueryProcessor(Lexicon& lex,
                   ForwardIndex& fwd_idx,
                   InvertedIndex& inv_idx,
                   TextProcessor& txt_proc);
    
    // Main search function
    std::vector<SearchResult> search(const std::string& query,
                                     u32 top_k = 10);
    
    // Get document snippet/preview (for display)
    std::string get_document_preview(u32 doc_id, 
                                    const std::vector<u32>& query_terms,
                                    u32 max_terms = 50);
    
    // Print search results (for testing)
    void print_results(const std::vector<SearchResult>& results,
                      u32 max_display = 10);
    
    // Get statistics
    const CollectionStats& get_stats() const { return stats; }
};


