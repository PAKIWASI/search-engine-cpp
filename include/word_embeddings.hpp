#pragma once

#include "common_includes.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

class WordEmbeddings {
private:
    // word -> embedding vector (100-dim for GloVe 6B.100d)
    std::unordered_map<std::string, std::vector<float>> word_embeddings;
    u32 embedding_dimension;
    std::unordered_set<std::string> loaded_words;
    
    // Normalize vector to unit length (L2 norm)
    void normalize_vector(std::vector<float>& vec) const;

public:
    WordEmbeddings();
    
    // Load pre-trained embeddings from binary format
    bool load_embeddings_binary(const std::string& binary_file);
    
    // Load only embeddings for words in lexicon (memory efficient)
    bool load_embeddings_for_lexicon(
        const std::string& embedding_file,
        const std::unordered_set<std::string>& lexicon_words);
    
    // Get embedding for a single word
    const std::vector<float>* get_word_embedding(const std::string& word) const;
    
    // Compute average embedding for multiple words (document/query representation)
    std::vector<float> get_average_embedding(const std::vector<std::string>& words);
    
    // Compute cosine similarity between two vectors
    float cosine_similarity(const std::vector<float>& vec1, 
                           const std::vector<float>& vec2) const;
    
    u32 get_dimension() const { return embedding_dimension; }
    size_t get_vocabulary_size() const { return word_embeddings.size(); }
    bool has_word(const std::string& word) const { 
        return loaded_words.find(word) != loaded_words.end(); 
    }
};
