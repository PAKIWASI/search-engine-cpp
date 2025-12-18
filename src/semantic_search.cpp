#include "semantic_search.hpp"

#include <iostream>
#include <iomanip>
#include <algorithm>

SemanticSearchEngine::SemanticSearchEngine(WordEmbeddings& emb, SearchEngine& bm25,
                                          Lexicon& lex, ForwardIndex& fwd)
    : embeddings(emb), bm25_engine(bm25), lexicon(lex), forward_index(fwd) {
    
    std::cout << "\n╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║           Semantic Search Engine Initialized                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
    std::cout << "  Embedding dimension: " << embeddings.get_dimension() << "\n";
    std::cout << "  Vocabulary size: " << embeddings.get_vocabulary_size() << "\n";
    std::cout << "  Semantic weight: " << semantic_weight << "\n";
    std::cout << "  BM25 weight: " << bm25_weight << "\n\n";
}

void SemanticSearchEngine::set_weights(float semantic_w, float bm25_w) {
    float total = semantic_w + bm25_w;
    semantic_weight = semantic_w / total;
    bm25_weight = bm25_w / total;
    
    std::cout << "Updated weights - Semantic: " << semantic_weight 
              << ", BM25: " << bm25_weight << "\n";
}

std::vector<std::string> SemanticSearchEngine::process_query_tokens(
    const std::string& query) {
    
    std::vector<std::string> tokens;
    
    // Use LibStemmer to process query
    std::unordered_map<std::string, u32> term_frequencies;
    stemmer.process_text(query, term_frequencies);
    
    // Extract tokens
    for (const auto& [word, freq] : term_frequencies) {
        tokens.push_back(word);
    }
    
    return tokens;
}

std::vector<float> SemanticSearchEngine::get_document_embedding(u32 doc_id) {
    // Check cache first
    auto it = doc_embedding_cache.find(doc_id);
    if (it != doc_embedding_cache.end()) {
        return it->second;
    }
    
    // Get document terms from forward index
    const auto* doc_terms = forward_index.get_document_terms(doc_id);
    if (doc_terms == nullptr) {
        return std::vector<float>();
    }
    
    // Extract words from document
    std::vector<std::string> words;
    words.reserve(doc_terms->size());
    
    for (const auto& word_data : *doc_terms) {
        std::string* word = lexicon.get_word(word_data.word_id);
        if (word) {
            // Add word multiple times based on frequency (weighted averaging)
            for (u32 i = 0; i < std::min(word_data.freq, 10u); i++) {
                words.push_back(*word);
            }
        }
    }
    
    // Get average embedding
    std::vector<float> embedding = embeddings.get_average_embedding(words);
    
    // Cache it
    if (!embedding.empty()) {
        doc_embedding_cache[doc_id] = embedding;
    }
    
    return embedding;
}

std::vector<SemanticSearchResult> SemanticSearchEngine::semantic_search(
    const std::string& query, 
    u32 max_results) {
    
    std::vector<SemanticSearchResult> results;
    
    std::cout << "\n[Semantic Search] Processing query...\n";
    
    // Process query tokens
    std::vector<std::string> query_tokens = process_query_tokens(query);
    
    if (query_tokens.empty()) {
        std::cout << "No valid query tokens after processing\n";
        return results;
    }
    
    std::cout << "[Semantic Search] Query tokens: ";
    for (const auto& token : query_tokens) {
        std::cout << token << " ";
    }
    std::cout << "\n";
    
    // Get query embedding
    std::vector<float> query_embedding = embeddings.get_average_embedding(query_tokens);
    
    if (query_embedding.empty()) {
        std::cout << "Warning: Could not create query embedding (words not in vocabulary)\n";
        return results;
    }
    
    std::cout << "[Semantic Search] Computing similarity for all documents...\n";
    
    // Score all documents
    u32 total_docs = forward_index.size();
    u32 processed = 0;
    u32 valid = 0;
    
    for (u32 doc_id = 0; doc_id < total_docs; doc_id++) {
        // Get document embedding
        std::vector<float> doc_embedding = get_document_embedding(doc_id);
        
        if (doc_embedding.empty()) continue;
        
        processed++;
        
        // Compute cosine similarity
        float score = embeddings.cosine_similarity(query_embedding, doc_embedding);
        
        if (score > 0.0f) {
            SemanticSearchResult result;
            result.doc_id = doc_id;
            
            const std::string* cord_uid = forward_index.get_doc_cord_uid(doc_id);
            if (cord_uid) {
                result.cord_uid = *cord_uid;
            }
            
            result.semantic_score = score;
            result.bm25_score = 0.0f;
            result.combined_score = score;
            
            results.push_back(result);
            valid++;
        }
        
        if (processed % 1000 == 0) {
            std::cout << "  Processed " << processed << " documents...\n";
        }
    }
    
    // Sort by semantic score
    std::sort(results.begin(), results.end());
    
    // Keep top K
    if (results.size() > max_results) {
        results.resize(max_results);
    }
    
    std::cout << "[Semantic Search] Found " << valid 
              << " documents with similarity > 0, returning top " 
              << results.size() << "\n";
    
    return results;
}

std::vector<SemanticSearchResult> SemanticSearchEngine::hybrid_search(
    const std::string& query, 
    u32 max_results) {
    
    std::cout << "\n[Hybrid Search] Starting hybrid search...\n";
    
    // Step 1: Get BM25 results
    std::cout << "[Hybrid Search] Step 1: BM25 search...\n";
    auto bm25_results = bm25_engine.search(query, max_results * 3, true);
    
    // Step 2: Get semantic results
    std::cout << "[Hybrid Search] Step 2: Semantic search...\n";
    auto semantic_results = semantic_search(query, max_results * 3);
    
    // Step 3: Combine results
    std::cout << "[Hybrid Search] Step 3: Combining results...\n";
    std::unordered_map<u32, SemanticSearchResult> combined;
    
    // Add BM25 results
    float max_bm25 = bm25_results.empty() ? 1.0f : bm25_results[0].score;
    if (max_bm25 < 0.01f) max_bm25 = 1.0f;
    
    for (const auto& result : bm25_results) {
        SemanticSearchResult sr;
        sr.doc_id = result.doc_id;
        sr.cord_uid = result.cord_uid;
        sr.bm25_score = result.score / max_bm25;  // Normalize to 0-1
        sr.semantic_score = 0.0f;
        sr.term_frequencies = result.term_frequencies;
        
        combined[result.doc_id] = sr;
    }
    
    // Add/update semantic results
    float max_semantic = semantic_results.empty() ? 1.0f : semantic_results[0].semantic_score;
    if (max_semantic < 0.01f) max_semantic = 1.0f;
    
    for (const auto& result : semantic_results) {
        float norm_semantic = result.semantic_score / max_semantic;
        
        auto it = combined.find(result.doc_id);
        if (it != combined.end()) {
            // Update existing entry
            it->second.semantic_score = norm_semantic;
        } else {
            // Add new entry
            SemanticSearchResult sr;
            sr.doc_id = result.doc_id;
            sr.cord_uid = result.cord_uid;
            sr.semantic_score = norm_semantic;
            sr.bm25_score = 0.0f;
            combined[result.doc_id] = sr;
        }
    }
    
    // Calculate combined scores
    std::vector<SemanticSearchResult> final_results;
    for (auto& [doc_id, result] : combined) {
        result.combined_score = (semantic_weight * result.semantic_score) +
                               (bm25_weight * result.bm25_score);
        final_results.push_back(result);
    }
    
    // Sort by combined score
    std::sort(final_results.begin(), final_results.end());
    
    // Keep top K
    if (final_results.size() > max_results) {
        final_results.resize(max_results);
    }
    
    std::cout << "[Hybrid Search] Returning " << final_results.size() << " results\n\n";
    
    return final_results;
}

void SemanticSearchEngine::display_results(
    const std::vector<SemanticSearchResult>& results,
    const std::string& query,
    bool verbose) {
    
    if (results.empty()) {
        std::cout << "\n\033[1;33mNo results found for query: \"" << query << "\"\033[0m\n";
        return;
    }
    
    std::cout << "\n\033[1;32m✓ Found " << results.size() << " result(s) for: \"" 
              << query << "\"\033[0m\n\n";
    
    std::cout << "┌──────────────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│ \033[1;36mRank │ Semantic │ BM25    │ Combined │ CORD UID                      \033[0m │\n";
    std::cout << "├──────────────────────────────────────────────────────────────────────────────────┤\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        std::cout << "│ " 
                  << std::setw(4) << std::left << (i + 1) << " │ "
                  << std::setw(8) << std::fixed << std::setprecision(4) 
                  << result.semantic_score << " │ "
                  << std::setw(7) << result.bm25_score << " │ "
                  << std::setw(8) << result.combined_score << " │ "
                  << std::setw(30) << std::left 
                  << (result.cord_uid.empty() ? "N/A" : result.cord_uid.substr(0, 30)) 
                  << " │\n";
        
        if (verbose && i < 5 && !result.term_frequencies.empty()) {
            std::cout << "│      │          │         │          │ \033[90mMatched terms: \033[0m";
            
            bool first = true;
            u32 count = 0;
            for (const auto& [word_id, freq] : result.term_frequencies) {
                if (count >= 5) break;
                std::string* word = lexicon.get_word(word_id);
                if (word) {
                    if (!first) std::cout << ", ";
                    std::cout << *word << ":" << freq;
                    first = false;
                    count++;
                }
            }
            std::cout << " │\n";
        }
    }
    
    std::cout << "└──────────────────────────────────────────────────────────────────────────────────┘\n";
}


