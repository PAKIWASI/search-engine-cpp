#pragma once

#include "common_includes.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <unordered_set>

class LibStemmer {
private:
    struct sb_stemmer* stemmer;
    
    // Medical and noise filters
    std::unordered_set<std::string> medical_preserve;
    std::unordered_set<std::string> medical_abbrev;
    std::unordered_set<std::string> noise_words;
    std::unordered_set<std::string> stop_words;
    
    // Enhanced dictionaries
    std::unordered_set<std::string> common_words_dict;
    std::unordered_map<std::string, std::string> stemming_corrections;
    
    // Helper methods - ALL DECLARED HERE
    void init_medical_terms();
    void init_stop_words();
    void init_common_words();
    void init_stemming_corrections();
    
    bool is_likely_noise(const std::string& text) const;
    bool is_stop_word(const std::string& word) const;
    static std::string clean_token(const std::string& token);
    bool should_preserve(const std::string& word) const;
    
    // Enhanced stemming methods
    std::string improved_stem(const std::string& word);
    std::string correct_stemming(const std::string& stemmed);
    std::string porter2_stem(const std::string& word);
    
public:
    explicit LibStemmer();
    ~LibStemmer();
    
    void process_text(const std::string& text, 
                     std::unordered_map<std::string, u32>& term_frequencies);
    
    // Enhanced processing
    void process_text_enhanced(const std::string& text,
                              std::unordered_map<std::string, u32>& term_frequencies);
    
    std::string stem_word(const std::string& word);
    std::vector<std::string> stem_words(const std::vector<std::string>& words);
    
    // Porter2 stemmer
    std::string stem_word_porter2(const std::string& word);
    
    bool is_valid() const { return stemmer != nullptr; }
};
