#pragma once

#include "lexicon.hpp"
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

class TextProcessor {
private:
    Lexicon& lexicon;       // main lex
    
    const std::string python_script_path;   // path to python script
    
    std::string python_interpreter; // path to python interpreter
    
    // helper function to call python lemmatizer
    bool call_python_lemmatizer_with_text( std::ofstream& lemma_input, 
        std::unordered_map<std::string, WordData>& temp_lex);

public:
    explicit TextProcessor( Lexicon& lex,
        const std::string& python_path = "python/.venv/bin/python3",
        const std::string& script_path = "python/lemmatizer_2.py");
    
    // process text: pass to Python lemmatizer and update lexicon
    bool lemmatize_text( std::ofstream& full_text, 
        std::unordered_map<std::string, WordData>& temp_lex);
    
    size_t get_lexicon_size() const { return lexicon.size(); }
};


