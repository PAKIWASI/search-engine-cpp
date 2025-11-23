#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

class TextProcessor
{
private:
    // Lexicon: word -> frequency count
    std::unordered_map<std::string, uint32_t> lexicon;
    
    // Path to Python script
    const std::string python_script_path;
    
    // Path to Python interpreter
    std::string python_interpreter;
    
    // Helper functions
    bool call_python_lemmatizer_with_file(const std::string& text,
                                          std::unordered_map<std::string, uint32_t>& temp_lexicon);
    
    void merge_lexicon(const std::unordered_map<std::string, uint32_t>& temp_lexicon);

public:
    // Constructor with paths to Python interpreter and script
    explicit TextProcessor(const std::string& python_path = "python/.venv/bin/python3",
                  const std::string& script_path = "python/lemmatizer.py");
    
    // Process text: pass to Python lemmatizer and merge results
    bool process_text(const std::string& text);
    
    // Get the lexicon
    const std::unordered_map<std::string, uint32_t>& get_lexicon() const;
    
    // Save lexicon to file
    void save_lexicon(const std::string& output_path);
    
    // Print top N most frequent words
    void print_top_words(int n);
    
    // Get lexicon size
    size_t get_lexicon_size() const { return lexicon.size(); }
    
    // Clear the lexicon
    void clear_lexicon();
    
};


