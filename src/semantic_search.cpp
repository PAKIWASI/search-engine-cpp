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

// In semantic_search.cpp, update get_document_embedding:
std::vector<float> SemanticSearchEngine::get_document_embedding(u32 doc_id) {
    // Check cache first
    auto it = doc_embedding_cache.find(doc_id);
    if (it != doc_embedding_cache.end()) {
        return it->second;
    }
    
    // Get document metadata
    const DocumentMetadata* meta = forward_index.get_document_metadata(doc_id);
    if (!meta) {
        return std::vector<float>();
    }
    
    // Extract text from document (title + abstract)
    std::string full_text;
    if (!meta->title.empty()) {
        full_text += meta->title + " ";
    }
    if (!meta->abstract.empty()) {
        full_text += meta->abstract;
    }
    
    if (full_text.empty()) {
        return std::vector<float>();
    }
    
    // Simple tokenization (similar to theirs)
    std::vector<std::string> tokens;
    std::istringstream iss(full_text);
    std::string token;
    
    while (iss >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        // Remove non-alphanumeric (keep hyphens for medical terms)
        token.erase(
            std::remove_if(token.begin(), token.end(),
                          [](char c) { 
                              return !std::isalnum(c) && c != '-'; 
                          }),
            token.end());
        
        if (!token.empty() && token.length() >= 2) {
            tokens.push_back(token);
        }
    }
    
    // Get average embedding
    std::vector<float> embedding = embeddings.get_average_embedding(tokens);
    
    // Check if embedding is valid (not all zeros)
    bool all_zeros = std::all_of(embedding.begin(), embedding.end(),
                               [](float f) { return f == 0.0f; });
    
    if (all_zeros) {
        return std::vector<float>(); // Return empty for invalid embeddings
    }
    
    // Cache it
    doc_embedding_cache[doc_id] = embedding;
    
    return embedding;
}

// In semantic_search.cpp, update the semantic_search method:

std::vector<SemanticSearchResult> SemanticSearchEngine::semantic_search(
    const std::string& query, 
    u32 max_results) {
    
    std::vector<SemanticSearchResult> results;
    
    std::cout << "\n[Semantic Search] Processing query...\n";
    
    // Simple query tokenization (similar to theirs)
    std::vector<std::string> query_tokens;
    std::istringstream iss(query);
    std::string token;
    
    while (iss >> token) {
        // Convert to lowercase
        std::transform(token.begin(), token.end(), token.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        // Remove non-alphanumeric (keep hyphens)
        token.erase(
            std::remove_if(token.begin(), token.end(),
                          [](char c) { 
                              return !std::isalnum(c) && c != '-'; 
                          }),
            token.end());
        
        if (!token.empty() && token.length() >= 2) {
            query_tokens.push_back(token);
        }
    }
    
    if (query_tokens.empty()) {
        std::cout << "No valid query tokens\n";
        return results;
    }
    
    std::cout << "[Semantic Search] Query tokens: ";
    for (const auto& token : query_tokens) {
        std::cout << token << " ";
    }
    std::cout << "\n";
    
    // Get query embedding
    std::cout << "[Semantic Search] Computing query embedding...\n";
    std::vector<float> query_embedding = embeddings.get_average_embedding(query_tokens);
    
    // Check if query embedding is valid
    if (query_embedding.empty()) {
        std::cout << "Warning: Could not create query embedding\n";
        return results;
    }
    
    bool all_zeros = std::all_of(query_embedding.begin(), query_embedding.end(),
                               [](float f) { return f == 0.0f; });
    
    if (all_zeros) {
        std::cout << "Warning: Query embedding is zero (words not in vocabulary)\n";
        
        // Debug: Show which words aren't in vocabulary
        std::cout << "Words not in embeddings vocabulary:\n";
        for (const auto& token : query_tokens) {
            const auto* emb = embeddings.get_word_embedding(token);
            if (!emb) {
                std::cout << "  - '" << token << "'\n";
            }
        }
        return results;
    }
    
    std::cout << "[Semantic Search] Scoring all documents...\n";
    
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
        
        // Use their simple threshold
        if (score > 0.0f) {
            SemanticSearchResult result;
            result.doc_id = doc_id;
            
            const DocumentMetadata* meta = forward_index.get_document_metadata(doc_id);
            if (meta) {
                result.cord_uid = meta->cord_uid;
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
    
    // Sort by semantic score (descending)
    std::sort(results.begin(), results.end());
    
    // Keep top K
    if (results.size() > max_results) {
        results.resize(max_results);
    }
    
    std::cout << "[Semantic Search] Found " << valid 
              << " semantically similar documents, returning top " 
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
    bool verbose) 
{
    
    if (results.empty()) {
        std::cout << "\n\033[1;33mNo results found for query: \"" << query << "\"\033[0m\n";
        return;
    }
    
    std::cout << "\n\033[1;32m✓ Found " << results.size() << " result(s) for: \"" 
              << query << "\"\033[0m\n\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        // Get document metadata
        const DocumentMetadata* meta = forward_index.get_document_metadata(result.doc_id);
        
        // Top border
        std::cout << "┌──────────────────────────────────────────────────────────────────────────────────┐\n";
        
        // Result number and score
        std::cout << "│ \033[1;36m#" << std::setw(2) << (i + 1) << "\033[0m";
        std::cout << "  Semantic Score: \033[1m" << std::fixed << std::setprecision(4) 
                  << result.semantic_score << "\033[0m";
        
        // Show BM25 score if available
        if (result.bm25_score > 0.0f) {
            std::cout << "  │  BM25: \033[1;33m" << std::fixed << std::setprecision(4) 
                     << result.bm25_score << "\033[0m";
        }
        
        if (result.combined_score > 0.0f && result.combined_score != result.semantic_score) {
            std::cout << "  Combined: \033[1;32m" << std::fixed << std::setprecision(4) 
                     << result.combined_score << "\033[0m";
        }
        
        std::cout << std::string(30, ' ') << "│\n";
        
        // Separator
        std::cout << "├──────────────────────────────────────────────────────────────────────────────────┤\n";
        
        // Title
        std::string title = "[No title available]";
        if (meta && !meta->title.empty()) {
            title = meta->title;
        }
        
        if (title.length() > 93) {
            title = title.substr(0, 90) + "...";
        }
        std::cout << "│ \033[1m" << std::setw(93) << std::left << title << "\033[0m │\n";
        
        // Abstract (if available)
        if (meta && !meta->abstract.empty()) {
            std::cout << "├──────────────────────────────────────────────────────────────────────────────────┤\n";
            
            std::string abstract_preview = meta->abstract;
            if (!verbose && abstract_preview.length() > 180) {
                abstract_preview = abstract_preview.substr(0, 177) + "...";
            }
            
            // Wrap abstract text
            std::stringstream ss(abstract_preview);
            std::string word;
            std::string line;
            size_t line_length = 0;
            bool first_line = true;
            
            while (ss >> word) {
                if (line_length + word.length() + 1 > 93) {
                    if (!first_line) {
                        std::cout << "│ \033[90m" << std::setw(93) << std::left << line << "\033[0m │\n";
                    } else {
                        std::cout << "│ \033[90m" << std::setw(93) << std::left << line << "\033[0m │\n";
                        first_line = false;
                    }
                    line = word;
                    line_length = word.length();
                } else {
                    if (!line.empty()) {
                        line += " ";
                        line_length++;
                    }
                    line += word;
                    line_length += word.length();
                }
            }
            
            if (!line.empty()) {
                std::cout << "│ \033[90m" << std::setw(93) << std::left << line << "\033[0m │\n";
            }
            
            if (!verbose && meta->abstract.length() > 180) {
                std::cout << "│ \033[90m" << std::setw(93) << std::left << "..." 
                          << "\033[0m │\n";
            }
        }
        
        // Separator before metadata
        std::cout << "├──────────────────────────────────────────────────────────────────────────────────┤\n";
        
        // CORD UID
        std::cout << "│ \033[90mCORD UID:\033[0m ";
        std::cout << std::setw(85) << std::left << result.cord_uid << " │\n";
        
        // PMC ID/URL (if available)
        if (meta && !meta->pmcid.empty()) {
            std::cout << "│ \033[90mPMC ID:\033[0m   ";
            std::cout << std::setw(85) << std::left << meta->pmcid << " │\n";
        }
        
        // Bottom border
        std::cout << "└──────────────────────────────────────────────────────────────────────────────────┘\n";
        
        // Add spacing between results
        if (i < results.size() - 1) {
            std::cout << "\n";
        }
    }
    
    // Summary footer
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << std::setw(96) << std::left 
              << ("Showing " + std::to_string(results.size()) + " semantic result(s)") 
              << " ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════════╝\n";
}


