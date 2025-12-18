#include "word_embeddings.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <chrono>

WordEmbeddings::WordEmbeddings() : embedding_dimension(0) {}

void WordEmbeddings::normalize_vector(std::vector<float>& vec) const {
    float norm = 0.0f;
    for (float val : vec) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm < 1e-10) return; // Avoid division by zero
    
    for (auto& val : vec) {
        val /= norm;
    }
}

bool WordEmbeddings::load_embeddings_binary(const std::string& binary_file) {
    std::ifstream file(binary_file, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open binary embedding file: " << binary_file << '\n';
        return false;
    }
    
    // Read header
    u32 num_words, dim;
    file.read(reinterpret_cast<char*>(&num_words), sizeof(num_words));
    file.read(reinterpret_cast<char*>(&dim), sizeof(dim));
    
    embedding_dimension = dim;
    std::cout << "Loading " << num_words << " embeddings (dim=" << dim << ")...\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (u32 i = 0; i < num_words; i++) {
        // Read word length and word
        u32 word_len;
        file.read(reinterpret_cast<char*>(&word_len), sizeof(word_len));
        
        std::string word(word_len, '\0');
        file.read(&word[0], word_len);
        
        // Read embedding
        std::vector<float> embedding(dim);
        file.read(reinterpret_cast<char*>(embedding.data()), dim * sizeof(float));
        
        word_embeddings[word] = std::move(embedding);
        loaded_words.insert(word);
        
        if ((i + 1) % 10000 == 0) {
            std::cout << "  Loaded " << (i + 1) << "/" << num_words << '\n';
        }
    }
    
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "✓ Loaded " << word_embeddings.size() << " word embeddings in " 
              << duration.count() << " ms\n";
    
    return true;
}

bool WordEmbeddings::load_embeddings_for_lexicon(
    const std::string& embedding_file,
    const std::unordered_set<std::string>& lexicon_words) {
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::ifstream file(embedding_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open embedding file: " << embedding_file << '\n';
        return false;
    }
    
    std::string line;
    u32 total_lines = 0;
    u32 loaded = 0;
    
    std::cout << "\n=== Loading Embeddings for Lexicon ===\n";
    std::cout << "Target words: " << lexicon_words.size() << '\n';
    std::cout << "Reading from: " << embedding_file << '\n';
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string word;
        
        if (!(iss >> word)) continue;
        
        total_lines++;
        
        // Skip if word not in lexicon
        if (lexicon_words.find(word) == lexicon_words.end()) {
            continue;
        }
        
        std::vector<float> embedding;
        float value;
        
        while (iss >> value) {
            embedding.push_back(value);
        }
        
        if (embedding.empty()) continue;
        
        // Set dimension from first word
        if (embedding_dimension == 0) {
            embedding_dimension = embedding.size();
            std::cout << "Embedding dimension detected: " << embedding_dimension << '\n';
        }
        
        // Normalize embedding to unit length
        normalize_vector(embedding);
        word_embeddings[word] = std::move(embedding);
        loaded_words.insert(word);
        loaded++;
        
        if (loaded % 5000 == 0) {
            float progress = (loaded * 100.0f) / lexicon_words.size();
            std::cout << "  Progress: " << loaded << "/" << lexicon_words.size() 
                      << " (" << progress << "%)\n";
        }
    }
    
    file.close();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    u32 not_found = lexicon_words.size() - loaded;
    
    std::cout << "\n=== Loading Complete ===\n";
    std::cout << "✓ Loaded: " << loaded << " embeddings\n";
    std::cout << "✗ Not found: " << not_found << " words\n";
    std::cout << "  Time: " << duration.count() << " ms\n";
    std::cout << "  Coverage: " << (loaded * 100.0f / lexicon_words.size()) << "%\n\n";
    
    return loaded > 0;
}

const std::vector<float>* WordEmbeddings::get_word_embedding(const std::string& word) const {
    auto it = word_embeddings.find(word);
    if (it != word_embeddings.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<float> WordEmbeddings::get_average_embedding(
    const std::vector<std::string>& words) {
    
    if (embedding_dimension == 0) {
        return std::vector<float>();
    }
    
    std::vector<float> avg_embedding(embedding_dimension, 0.0f);
    u32 found_count = 0;
    
    for (const auto& word : words) {
        const auto* emb = get_word_embedding(word);
        if (emb) {
            for (u32 i = 0; i < embedding_dimension; i++) {
                avg_embedding[i] += (*emb)[i];
            }
            found_count++;
        }
    }
    
    if (found_count > 0) {
        for (u32 i = 0; i < embedding_dimension; i++) {
            avg_embedding[i] /= found_count;
        }
        normalize_vector(avg_embedding);
    }
    
    return avg_embedding;
}

float WordEmbeddings::cosine_similarity(const std::vector<float>& vec1, 
                                        const std::vector<float>& vec2) const {
    if (vec1.size() != vec2.size() || vec1.empty()) {
        return 0.0f;
    }
    
    float dot_product = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    for (size_t i = 0; i < vec1.size(); i++) {
        dot_product += vec1[i] * vec2[i];
        norm1 += vec1[i] * vec1[i];
        norm2 += vec2[i] * vec2[i];
    }
    
    // Vectors should already be normalized, but be safe
    float denominator = std::sqrt(norm1) * std::sqrt(norm2);
    if (denominator < 1e-10) return 0.0f;
    
    return dot_product / denominator;
}


