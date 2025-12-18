#pragma once

#include "common_includes.hpp"
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"
#include "libstemmer.hpp"

#include <string>
#include <vector>
#include <unordered_map>

// Result for a single document
struct SearchResult {
    u32 doc_id;
    std::string cord_uid;
    std::string title;        
    std::string abstract;     
    std::string pmcid;          
    double score;
    std::unordered_map<u32, u32> term_frequencies;
    
    bool operator<(const SearchResult& other) const {
        return score < other.score;
    }
};

// Query term information
struct QueryTerm {
    std::string word;
    u32 word_id;
    u32 doc_freq; // number of docs containing this term
    bool found;
};

class SearchEngine {
private:
    Lexicon& lexicon;
    ForwardIndex& forward_index;
    InvertedIndex& inverted_index;
    LibStemmer stemmer;  // Add stemmer for query processing
    
    u32 total_documents;
    
    // Helper functions for ranking
    static double compute_tf(u32 term_freq, u32 doc_length);
    static double compute_idf(u32 doc_freq, u32 total_docs);
    double compute_tf_idf(u32 term_freq, u32 doc_length, u32 doc_freq) const;
    
    // Process query string into terms
    std::vector<QueryTerm> process_query(const std::string& query);
    
    // Search implementations
    std::vector<SearchResult> single_word_search(const QueryTerm& term, u32 max_results);
    std::vector<SearchResult> multi_word_search(const std::vector<QueryTerm>& terms, u32 max_results);
    
    // Ranking
    void rank_results_tfidf(std::vector<SearchResult>& results, 
                           const std::vector<QueryTerm>& query_terms);
    void rank_results_bm25(std::vector<SearchResult>& results,
                          const std::vector<QueryTerm>& query_terms);

public:
    SearchEngine(Lexicon& lex, ForwardIndex& fwd, InvertedIndex& inv);
    
    // Main search interface
    std::vector<SearchResult> search(const std::string& query, 
                                    u32 max_results = 10,
                                    bool use_bm25 = true);
    
    // Display results
    void display_results(const std::vector<SearchResult>& results, 
                        const std::string& query,
                        bool verbose = false);
    
    // Statistics
    void print_search_stats(const std::vector<SearchResult>& results,
                          const std::vector<QueryTerm>& query_terms);
    
    // Getters for access to indices
    InvertedIndex& get_inverted_index() { return inverted_index; }
};


