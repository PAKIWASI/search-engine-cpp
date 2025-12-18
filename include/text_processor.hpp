#pragma once

#include "common_includes.hpp"
#include "lexicon.hpp"
#include "libstemmer.hpp"

#include <string>
#include <unordered_map>
#include <cstdio>



class TextProcessor {
private:
    Lexicon& lexicon;
    
    // python daemon process
    FILE* python_in;    // write to python
    FILE* python_out;   // read from python
    pid_t python_pid;   // process id of python
    bool daemon_active; // check if daemon is active
    
    // start the python daemon
    bool start_daemon(const std::string& python_path, 
                     const std::string& script_path);
    
    // stop the python daemon
    void stop_daemon();
    
    // send text to daemon and read results
    bool process_with_daemon(const std::string& text,
                            std::unordered_map<std::string, WordData>& temp_lex);
    

    // LibStemmer - alternate lemmatizer (pure cpp)
    LibStemmer stemmer;
    
public:
    explicit TextProcessor(Lexicon& lex) : lexicon(lex) {}
    explicit TextProcessor(Lexicon& lex,
                           const std::string& python_path,
                           const std::string& script_path);
    
    ~TextProcessor();
    
    // Process text through daemon
    bool lemmatize_text(std::string& text, 
                        std::unordered_map<std::string, WordData>& temp_lex);

    bool lemmatize_libstemmer(const std::string& text,
                            std::unordered_map<std::string, WordData>& temp_lex);
    
    size_t get_lexicon_size() const { return lexicon.size(); }
};



