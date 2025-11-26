#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

#include "word_data.hpp"



class Lexicon {
private:
    // word -> (word_id, frequency)
    std::unordered_map<std::string, WordData> data;
    uint32_t next_word_id = 0;

public:
    Lexicon() = default;
    
    // Add or update a word in the lexicon
    // Returns the word_id assigned to this word
    uint32_t add_word(const std::string& word, uint32_t frequency = 1);
    
    // Get word_data for a word 
    const WordData* get_word_info(const std::string& word) const;
    
    // Get word_id only 
    uint32_t get_word_id(const std::string& word) const;
    
    // Get frequency for a word
    uint32_t get_frequency(const std::string& word) const;
    
    // Check if word exists
    bool contains(const std::string& word) const;
    
    // Get lexicon size
    size_t size() const { return data.size(); }
    
    // Get all data (for iteration)
    const std::unordered_map<std::string, WordData>& get_data() const {
        return data;
    }
    
    // Merge another lexicon or temp lexicon into this one
    void merge(const std::unordered_map<std::string, WordData>& temp_lex);
    
    // Update temp_lex with actual word IDs from this lexicon
    void update_ids(std::unordered_map<std::string, WordData>& temp_lex) const;
    
    // Save lexicon to CSV file
    void save_to_file(const std::string& output_path) const;
    
    // Load lexicon from CSV file
    bool load_from_file(const std::string& input_path);
    
    // Print top N most frequent words
    void print_top_words(int n) const;
    
    // Clear the lexicon
    void clear();
};


