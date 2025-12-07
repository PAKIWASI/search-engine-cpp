#include "query_processor.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>


QueryProcessor::QueryProcessor(Lexicon& lex,
                               ForwardIndex& fwd_idx,
                               InvertedIndex& inv_idx,
                               TextProcessor& txt_proc)
    : lexicon(lex), forward_index(fwd_idx), 
      inverted_index(inv_idx), text_processor(txt_proc)
{
    compute_collection_stats();
}


void QueryProcessor::compute_collection_stats() 
{
    std::cout << "\nComputing collection statistics...\n";
    
    stats.total_docs = forward_index.get_document_count();
    
    u32 total_terms = 0;
    
    // Compute document lengths
    for (u32 doc_id = 0; doc_id < stats.total_docs; ++doc_id) {
        u32 doc_len = forward_index.get_total_words(doc_id);
        stats.doc_lengths[doc_id] = doc_len;
        total_terms += doc_len;
    }
    
    stats.avg_doc_length = static_cast<double>(total_terms) / stats.total_docs;
    
    std::cout << "  Total documents: " << stats.total_docs << '\n';
    std::cout << "  Average document length: " << stats.avg_doc_length << '\n';
}


double QueryProcessor::compute_bm25_score(
    u32 term_freq_in_doc,
    u32 doc_length,
    u32 docs_with_term,
    u32 total_docs,
    double avg_doc_length
) const 
{
    // IDF component: log((N - df + 0.5) / (df + 0.5))
    double idf = std::log(
        (total_docs - docs_with_term + 0.5) / (docs_with_term + 0.5)
    );
    
    // Avoid negative IDF for very common terms
    if (idf < 0.0) idf = 0.0;
    
    // Length normalization
    double norm = 1.0 - b + b * (doc_length / avg_doc_length);
    
    // TF component with saturation
    double tf = (term_freq_in_doc * (k1 + 1.0)) / 
                (term_freq_in_doc + k1 * norm);
    
    return idf * tf;
}


bool QueryProcessor::process_query(const std::string& query_text,
                                   std::vector<u32>& query_term_ids) 
{
    query_term_ids.clear();
    
    if (query_text.empty()) {
        std::cerr << "Empty query\n";
        return false;
    }
    
    // Lemmatize query using text processor
    std::unordered_map<std::string, WordData> query_lex;
    std::string query_copy = query_text;
    
    if (!text_processor.lemmatize_text(query_copy, query_lex)) {
        std::cerr << "Failed to lemmatize query\n";
        return false;
    }
    
    // Extract term IDs from lemmatized query
    for (const auto& [term, data] : query_lex) {
        u32 word_id = lexicon.get_word_id(term);
        if (word_id != UINT32_MAX) {
            query_term_ids.push_back(word_id);
        } else {
            std::cout << "Query term not in lexicon: " << term << '\n';
        }
    }
    
    if (query_term_ids.empty()) {
        std::cerr << "No valid query terms found\n";
        return false;
    }
    
    std::cout << "Processed query into " << query_term_ids.size() 
              << " terms\n";
    
    return true;
}


void QueryProcessor::get_candidate_documents(
    const std::vector<u32>& query_term_ids,
    std::unordered_map<u32, std::vector<std::pair<u32, u32>>>& candidates
) 
{
    candidates.clear();
    
    // For each query term, get its posting list
    for (u32 term_id : query_term_ids) {
        
        // Load the appropriate barrel if needed
        if (!inverted_index.load_barrel(term_id)) {
            std::cerr << "Failed to load barrel for term " << term_id << '\n';
            continue;
        }
        
        const std::vector<InvertedEntry>* postings = 
            inverted_index.get_word_terms(term_id);
        
        if (postings == nullptr || postings->empty()) {
            std::string* word = lexicon.get_word(term_id);
            if (word) {
                std::cout << "No documents found for term: " << *word << '\n';
            }
            continue;
        }
        
        // Add term frequencies for each document
        for (const auto& entry : *postings) {
            candidates[entry.doc_id].push_back({term_id, entry.freq});
        }
    }
    
    std::cout << "Found " << candidates.size() << " candidate documents\n";
}


void QueryProcessor::score_documents(
    const std::vector<u32>& query_term_ids,
    const std::unordered_map<u32, std::vector<std::pair<u32, u32>>>& candidates,
    std::vector<SearchResult>& results
) 
{
    results.clear();
    
    // Score each candidate document
    for (const auto& [doc_id, term_freqs] : candidates) {
        
        double total_score = 0.0;
        u32 doc_length = stats.doc_lengths[doc_id];
        
        // Sum BM25 scores for all query terms in this document
        for (const auto& [term_id, term_freq] : term_freqs) {
            
            u32 docs_with_term = inverted_index.get_total_docs(term_id);
            
            double bm25 = compute_bm25_score(
                term_freq,
                doc_length,
                docs_with_term,
                stats.total_docs,
                stats.avg_doc_length
            );
            
            total_score += bm25;
        }
        
        // Get document metadata
        const std::string* cord_uid = forward_index.get_doc_cord_uid(doc_id);
        std::string uid = cord_uid ? *cord_uid : "unknown";
        
        results.emplace_back(doc_id, uid, total_score);
    }
    
    // Sort by score (descending)
    std::sort(results.begin(), results.end());
}


std::vector<SearchResult> QueryProcessor::search(const std::string& query,
                                                u32 top_k) 
{
    std::cout << "\n=== SEARCH QUERY: \"" << query << "\" ===\n";
    
    std::vector<SearchResult> results;
    
    // Step 1: Process query
    std::vector<u32> query_term_ids;
    if (!process_query(query, query_term_ids)) {
        return results;  // Empty results
    }
    
    // Step 2: Get candidate documents
    std::unordered_map<u32, std::vector<std::pair<u32, u32>>> candidates;
    get_candidate_documents(query_term_ids, candidates);
    
    if (candidates.empty()) {
        std::cout << "No matching documents found\n";
        return results;
    }
    
    // Step 3: Score documents
    score_documents(query_term_ids, candidates, results);
    
    // Step 4: Return top-k results
    if (results.size() > top_k) {
        results.resize(top_k);
    }
    
    std::cout << "Returning top " << results.size() << " results\n";
    
    return results;
}


std::string QueryProcessor::get_document_preview(
    u32 doc_id, 
    const std::vector<u32>& query_terms,
    u32 max_terms
) 
{
    const std::vector<WordData>* doc_terms = 
        forward_index.get_document_terms(doc_id);
    
    if (doc_terms == nullptr) {
        return "[No preview available]";
    }
    
    std::string preview;
    u32 terms_added = 0;
    
    // Create a set of query terms for quick lookup
    std::unordered_set<u32> query_set(query_terms.begin(), query_terms.end());
    
    // Add terms to preview, highlighting query matches
    for (const auto& term_data : *doc_terms) {
        if (terms_added >= max_terms) break;
        
        std::string* word = lexicon.get_word(term_data.word_id);
        if (word) {
            // Mark query terms with ** for emphasis
            if (query_set.count(term_data.word_id)) {
                preview += "**" + *word + "** ";
            } else {
                preview += *word + " ";
            }
            terms_added++;
        }
    }
    
    if (doc_terms->size() > max_terms) {
        preview += "...";
    }
    
    return preview;
}


void QueryProcessor::print_results(const std::vector<SearchResult>& results,
                                   u32 max_display) 
{
    if (results.empty()) {
        std::cout << "\nNo results found.\n";
        return;
    }
    
    std::cout << "\n=== SEARCH RESULTS ===\n";
    std::cout << "Total results: " << results.size() << "\n\n";
    
    u32 display_count = std::min(max_display, static_cast<u32>(results.size()));
    
    for (u32 i = 0; i < display_count; ++i) {
        const auto& result = results[i];
        
        std::cout << std::setw(2) << (i + 1) << ". "
                  << "Score: " << std::fixed << std::setprecision(4) 
                  << result.score << " | "
                  << "Doc ID: " << result.doc_id << " | "
                  << "CORD UID: " << result.cord_uid << '\n';
    }
    
    if (results.size() > max_display) {
        std::cout << "\n... and " << (results.size() - max_display) 
                  << " more results\n";
    }
}


