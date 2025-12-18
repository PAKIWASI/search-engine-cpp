#include "search_engine.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>

// Constants for BM25 ranking
const double K1 = 1.2;
const double B = 0.75;
const double EPSILON = 0.25;

SearchEngine::SearchEngine(Lexicon& lex, ForwardIndex& fwd, InvertedIndex& inv)
    : lexicon(lex), forward_index(fwd), inverted_index(inv), stemmer() {
    total_documents = forward_index.size();
    
    if (!stemmer.is_valid()) {
        std::cerr << "Warning: Stemmer initialization failed in SearchEngine\n";
    }
}

std::vector<QueryTerm> SearchEngine::process_query(const std::string& query) 
{
    std::vector<QueryTerm> terms;
    
    // Use LibStemmer to process the query text
    std::unordered_map<std::string, u32> term_frequencies;
    stemmer.process_text(query, term_frequencies);
    
    // Convert processed terms to QueryTerm format
    for (const auto& [word, freq] : term_frequencies) {
        QueryTerm term;
        term.word = word;
        term.word_id = lexicon.get_word_id(word);
        term.found = (term.word_id != UINT32_MAX);
        
        if (term.found) {
            term.doc_freq = inverted_index.get_total_docs(term.word_id);
        } else {
            term.doc_freq = 0;
        }
        
        terms.push_back(term);
    }
    
    return terms;
}

std::vector<SearchResult> SearchEngine::single_word_search(const QueryTerm& term, u32 max_results) 
{
    std::vector<SearchResult> results;
    
    if (!term.found) {
        return results;
    }
    
    // Load barrel for this word if needed
    if (!inverted_index.load_barrel(term.word_id)) {
        return results;
    }
    
    const auto* postings = inverted_index.get_word_terms(term.word_id);
    if (postings == nullptr) {
        return results;
    }
    
    for (const auto& entry : *postings) {
        SearchResult result;
        result.doc_id = entry.doc_id;
        result.score = 0.0; // Will be computed during ranking
        result.term_frequencies[term.word_id] = entry.freq;
        
        const std::string* cord_uid = forward_index.get_doc_cord_uid(entry.doc_id);
        if (cord_uid != nullptr) {
            result.cord_uid = *cord_uid;
        }
        
        results.push_back(result);
    }
    
    return results;
}

std::vector<SearchResult> SearchEngine::multi_word_search(const std::vector<QueryTerm>& terms, 
                                                         u32 max_results) 
{
    std::vector<SearchResult> results;
    
    // Find documents that contain ALL terms (AND query)
    std::unordered_map<u32, SearchResult> doc_map;
    
    for (const auto& term : terms) {
        if (!term.found) {
            // If any term is not found, return empty results for AND query
            return results;
        }
        
        // Load barrel for this word if needed
        if (!inverted_index.load_barrel(term.word_id)) {
            continue;
        }
        
        const auto* postings = inverted_index.get_word_terms(term.word_id);
        if (postings == nullptr) {
            continue;
        }
        
        if (doc_map.empty()) {
            // First term: initialize with all documents
            for (const auto& entry : *postings) {
                SearchResult result;
                result.doc_id = entry.doc_id;
                result.term_frequencies[term.word_id] = entry.freq;
                
                const std::string* cord_uid = forward_index.get_doc_cord_uid(entry.doc_id);
                if (cord_uid != nullptr) {
                    result.cord_uid = *cord_uid;
                }
                
                doc_map[entry.doc_id] = result;
            }
        } else {
            // Intersect with existing documents
            std::unordered_map<u32, SearchResult> new_map;
            for (const auto& entry : *postings) {
                auto it = doc_map.find(entry.doc_id);
                if (it != doc_map.end()) {
                    it->second.term_frequencies[term.word_id] = entry.freq;
                    new_map[entry.doc_id] = it->second;
                }
            }
            doc_map.swap(new_map);
        }
        
        // Early exit if no documents match all terms
        if (doc_map.empty()) {
            return results;
        }
    }
    
    // Convert map to vector
    for (auto& pair : doc_map) {
        results.push_back(pair.second);
    }
    
    return results;
}

double SearchEngine::compute_tf(u32 term_freq, u32 doc_length) {
    if (doc_length == 0) { return 0.0; }
    return static_cast<double>(term_freq) / doc_length;
}

double SearchEngine::compute_idf(u32 doc_freq, u32 total_docs) {
    if (doc_freq == 0 || total_docs == 0) { return 0.0; }
    return log(static_cast<double>(total_docs) / doc_freq);
}

double SearchEngine::compute_tf_idf(u32 term_freq, u32 doc_length, u32 doc_freq) const {
    double tf = compute_tf(term_freq, doc_length);
    double idf = compute_idf(doc_freq, total_documents);
    return tf * idf;
}

void SearchEngine::rank_results_tfidf(std::vector<SearchResult>& results,
                                     const std::vector<QueryTerm>& query_terms) 
{
    for (auto& result : results) {
        result.score = 0.0;
        
        for (const auto& term : query_terms) {
            if (!term.found) { continue; }
            
            auto it = result.term_frequencies.find(term.word_id);
            if (it != result.term_frequencies.end()) {
                u32 term_freq = it->second;
                
                // Get document length (total words, not unique terms)
                u32 doc_length = forward_index.get_total_words(result.doc_id);
                if (doc_length == 0) doc_length = 1;
                
                double tfidf = compute_tf_idf(term_freq, doc_length, term.doc_freq);
                result.score += tfidf;
            }
        }
    }
}

void SearchEngine::rank_results_bm25(std::vector<SearchResult>& results,
                                    const std::vector<QueryTerm>& query_terms) 
{
    // Calculate average document length (using total words, not unique terms)
    double avg_doc_length = 0.0;
    u32 total_docs_with_terms = 0;
    
    for (const auto& result : results) {
        u32 doc_length = forward_index.get_total_words(result.doc_id);
        if (doc_length > 0) {
            avg_doc_length += doc_length;
            total_docs_with_terms++;
        }
    }
    
    if (total_docs_with_terms > 0) {
        avg_doc_length /= total_docs_with_terms;
    } else {
        avg_doc_length = 100.0; // Default fallback
    }
    
    // Rank using BM25
    for (auto& result : results) {
        result.score = 0.0;
        
        u32 doc_length = forward_index.get_total_words(result.doc_id);
        if (doc_length == 0) doc_length = 1;
        
        for (const auto& term : query_terms) {
            if (!term.found) { continue; }
            
            auto it = result.term_frequencies.find(term.word_id);
            if (it != result.term_frequencies.end()) {
                u32 term_freq = it->second;
                u32 doc_freq = term.doc_freq;
                
                // BM25 formula
                double idf = log(((total_documents - doc_freq + 0.5) / (doc_freq + 0.5)) + 1.0);
                double numerator = term_freq * (K1 + 1);
                double denominator = term_freq + (K1 * (1 - B + B * (doc_length / avg_doc_length)));
                
                result.score += idf * (numerator / denominator);
            }
        }
    }
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, 
                                              u32 max_results,
                                              bool use_bm25) 
{
    std::vector<SearchResult> results;
    
    // Process query into terms using LibStemmer
    std::vector<QueryTerm> query_terms = process_query(query);
    
    // Check if any terms were found
    bool any_found = false;
    for (const auto& term : query_terms) {
        if (term.found) {
            any_found = true;
            break;
        }
    }
    
    if (!any_found) {
        std::cout << "No valid search terms found in query.\n";
        return results;
    }
    
    // Single word search
    if (query_terms.size() == 1) {
        results = single_word_search(query_terms[0], max_results);
    } else {
        // Multi-word search (AND logic)
        results = multi_word_search(query_terms, max_results);
    }
    
    if (results.empty()) {
        return results;
    }
    
    // Rank results
    if (use_bm25) {
        rank_results_bm25(results, query_terms);
    } else {
        rank_results_tfidf(results, query_terms);
    }
    
    // Sort by score (descending)
    std::sort(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b) {
            return a.score > b.score;
        });
    
    // Limit results
    if (results.size() > max_results) {
        results.resize(max_results);
    }
    
    return results;
}

void SearchEngine::display_results(const std::vector<SearchResult>& results, 
                                  const std::string& query,
                                  bool verbose) {
    if (results.empty()) {
        std::cout << "\n\033[1;33mNo results found for query: \"" << query << "\"\033[0m\n";
        return;
    }
    
    std::cout << "\n\033[1;32m✓ Found " << results.size() << " result(s) for: \"" 
              << query << "\"\033[0m\n\n";
    
    std::cout << "┌────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ \033[1;36mRank │ Score   │ CORD UID                           │ Matches\033[0m          │\n";
    std::cout << "├────────────────────────────────────────────────────────────────────────┤\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        // Format score
        char score_str[16];
        snprintf(score_str, sizeof(score_str), "%.4f", result.score);
        
        // Count matching terms
        u32 match_count = static_cast<u32>(result.term_frequencies.size());
        
        std::cout << "│ " 
                  << std::setw(4) << std::left << (i + 1) << " │ "
                  << std::setw(7) << std::left << score_str << " │ "
                  << std::setw(35) << std::left << (result.cord_uid.empty() ? "N/A" : result.cord_uid) << " │ "
                  << std::setw(8) << std::left << match_count << " terms │\n";
        
        if (verbose && i < 5) { // Show details for top 5 in verbose mode
            std::cout << "│     │         │                                    │ ";
            std::cout << "\033[90mTerm frequencies: \033[0m";
            
            bool first = true;
            for (const auto& tf_pair : result.term_frequencies) {
                std::string* word = lexicon.get_word(tf_pair.first);
                if (word) {
                    if (!first) std::cout << ", ";
                    std::cout << *word << ":" << tf_pair.second;
                    first = false;
                }
            }
            std::cout << " │\n";
        }
    }
    
    std::cout << "└────────────────────────────────────────────────────────────────────────┘\n";
}

void SearchEngine::print_search_stats(const std::vector<SearchResult>& results,
                                     const std::vector<QueryTerm>& query_terms) 
{
    std::cout << "\n\033[1;36m=== SEARCH STATISTICS ===\033[0m\n\n";
    
    // Query term analysis
    std::cout << "Query Terms Analysis:\n";
    std::cout << "────────────────────\n";
    
    u32 found_terms = 0;
    for (const auto& term : query_terms) {
        std::cout << "  \"" << term.word << "\": ";
        if (term.found) {
            std::cout << "\033[1;32mFound\033[0m (ID: " << term.word_id 
                      << ", Docs: " << term.doc_freq << ")";
            found_terms++;
        } else {
            std::cout << "\033[1;31mNot Found\033[0m";
        }
        std::cout << "\n";
    }
    
    std::cout << "\nSearch Results:\n";
    std::cout << "────────────────\n";
    std::cout << "  Total documents matching: " << results.size() << "\n";
    
    if (!results.empty()) {
        double avg_score = 0.0;
        double max_score = results[0].score;
        double min_score = results.back().score;
        
        for (const auto& result : results) {
            avg_score += result.score;
        }
        avg_score /= results.size();
        
        std::cout << "  Score range: " << min_score << " - " << max_score << "\n";
        std::cout << "  Average score: " << avg_score << "\n";
        
        // Term distribution in top results
        std::unordered_map<u32, u32> term_appearances;
        for (const auto& result : results) {
            for (const auto& tf_pair : result.term_frequencies) {
                term_appearances[tf_pair.first]++;
            }
        }
        
        std::cout << "\nTerm Frequency in Top Results:\n";
        for (const auto& term : query_terms) {
            if (term.found) {
                u32 appearances = term_appearances[term.word_id];
                double percentage = (results.size() > 0) ? 
                    (100.0 * appearances / results.size()) : 0.0;
                
                std::string* word = lexicon.get_word(term.word_id);
                if (word) {
                    std::cout << "  \"" << *word << "\": " << appearances 
                              << "/" << results.size() 
                              << " docs (" << std::fixed << std::setprecision(1) 
                              << percentage << "%)\n";
                }
            }
        }
    }
    
    std::cout << "\nIndex Information:\n";
    std::cout << "──────────────────\n";
    std::cout << "  Total documents in index: " << total_documents << "\n";
    std::cout << "  Unique terms in lexicon: " << lexicon.size() << "\n";
}


