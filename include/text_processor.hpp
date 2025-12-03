#pragma once

#include "lexicon.hpp"

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

    bool process_with_daemon2(const std::string& text,
                    std::unordered_map<std::string, WordData>& temp_lex);

public:
    explicit TextProcessor(Lexicon& lex,
                           const std::string& python_path = "python/.venv/bin/python3",
                           const std::string& script_path = "python/lemmatizer_daemon.py");
    
    ~TextProcessor();
    
    // Process text through daemon
    bool lemmatize_text(std::string& text, 
                       std::unordered_map<std::string, WordData>& temp_lex);
    
    size_t get_lexicon_size() const { return lexicon.size(); }
};



