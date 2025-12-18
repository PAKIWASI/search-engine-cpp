#pragma once

#include "common_includes.hpp"
#include "word_embeddings.hpp"
#include "search_engine.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "libstemmer.hpp"

#include <string>
#include <vector>
#include <unordered_map>

// Extended search result with semantic scores
struct SemanticSearchResult {
    u32 doc_id;
    std::string cord_uid;
    float semantic_score;
    float bm25_score;
    float combined_score;
    
    // For display
    std::unordered_map<u32, u32> term_frequencies;
    
    bool operator<(const SemanticSearchResult& other) const {
        return combined_score > other.combined_score; // Descending order
    }
};

class SemanticSearchEngine {
private:
    WordEmbeddings& embeddings;
    SearchEngine& bm25_engine;
    Lexicon& lexicon;
    ForwardIndex& forward_index;
    LibStemmer stemmer;
    
    // Weights for hybrid search
    float semantic_weight = 0.6f;  // 60% semantic
    float bm25_weight = 0.4f;      // 40% BM25
    
    // Cache for document embeddings (doc_id -> embedding)
    std::unordered_map<u32, std::vector<float>> doc_embedding_cache;
    
    // Get document embedding (with caching)
    std::vector<float> get_document_embedding(u32 doc_id);
    
    // Process query into tokens using LibStemmer
    std::vector<std::string> process_query_tokens(const std::string& query);

public:
    SemanticSearchEngine(WordEmbeddings& emb, SearchEngine& bm25, 
                        Lexicon& lex, ForwardIndex& fwd);
    
    // Set hybrid search weights
    void set_weights(float semantic_w, float bm25_w);
    
    // Pure semantic search using word embeddings
    std::vector<SemanticSearchResult> semantic_search(
        const std::string& query, 
        u32 max_results = 10);
    
    // Hybrid search: BM25 + Semantic
    std::vector<SemanticSearchResult> hybrid_search(
        const std::string& query, 
        u32 max_results = 10);
    
    // Display semantic search results
    void display_results(const std::vector<SemanticSearchResult>& results,
                        const std::string& query,
                        bool verbose = false);
    
    // Clear document embedding cache
    void clear_cache() { doc_embedding_cache.clear(); }
};


