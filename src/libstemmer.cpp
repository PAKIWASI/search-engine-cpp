#include "libstemmer.hpp"
#include <libstemmer.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <regex>

// Constructor
LibStemmer::LibStemmer() : stemmer(nullptr) {
    stemmer = sb_stemmer_new("english", "UTF_8");
    if (!stemmer) {
        std::cerr << "Failed to create stemmer for english\n";
    }
    
    init_medical_terms();
    init_stop_words();
    init_common_words();
    init_stemming_corrections();
    
    std::cout << "Enhanced LibStemmer initialized\n";
}

// Destructor
LibStemmer::~LibStemmer() {
    if (stemmer) {
        sb_stemmer_delete(stemmer);
    }
}

void LibStemmer::init_medical_terms() {
    medical_preserve = {
        "covid", "covid-19", "covid19", "sars", "sars-cov", "sars-cov-2",
        "mers", "mers-cov", "coronavirus", "coronaviruses",
        "antibody", "antibodies", "antigen", "antigens", "vaccine", "vaccines", "vaccination",
        "virus", "viruses", "viral", "virion", "virions", "protein", "proteins", "peptide", "peptides",
        "rna", "dna", "mrna", "trna", "rrna", "genome", "genomic", "gene", "genes",
        "cell", "cells", "cellular", "cytokine", "cytokines", "pneumonia", "influenza", "respiratory",
        "pulmonary", "syndrome", "disease", "infection", "infectious", "transmission", "transmissible",
        "contagious", "symptom", "symptoms", "asymptomatic", "symptomatic", "diagnosis", "diagnostic",
        "prognosis", "prognostic", "treatment", "therapeutic", "therapy", "therapies", "immune", "immunity",
        "immunology", "immunological", "pathogen", "pathogens", "pathogenic", "pathogenesis", "pandemic",
        "epidemic", "endemic", "outbreak", "mortality", "morbidity", "fatality", "patient", "patients",
        "clinical", "hospital", "icu", "intensive", "ventilator", "ventilation", "oxygen", "hypoxia",
        "hypoxic", "inflammation", "inflammatory", "receptor", "receptors", "ace2", "spike", "nucleocapsid",
        "membrane", "antibacterial", "antiviral", "antimicrobial"
    };
    
    medical_abbrev = {
        "who", "cdc", "fda", "nih", "niaid", "ema", "pcr", "rt-pcr", "qrt-pcr", "elisa",
        "ards", "icu", "ecmo", "il", "il-6", "il-1", "tnf", "tnf-alpha", "ifn", "igg", "igm",
        "iga", "ige", "hiv", "aids", "tb", "ebv", "cmv", "ct", "mri", "xray", "ecg", "ekg",
        "mg", "ml", "kg", "mcg", "ng", "pg", "µg", "usa", "uk", "eu"
    };
    
    noise_words = {
        "vs", "ad", "ed", "ll", "b", "n", "a", "r", "see", "october", "bs", "epal", "z"
    };
}

void LibStemmer::init_stop_words() {
    stop_words = {
        "a", "an", "the", "and", "or", "but", "in", "on", "at", "to", "for",
        "of", "with", "by", "about", "against", "between", "into", "through",
        "during", "before", "after", "above", "below", "from", "up", "down",
        "out", "off", "over", "under", "again", "further", "then", "once",
        "here", "there", "when", "where", "why", "how", "all", "any", "both",
        "each", "few", "more", "most", "other", "some", "such", "no", "nor",
        "not", "only", "own", "same", "so", "than", "too", "very", "s", "t",
        "can", "will", "just", "don", "should", "now", "is", "are", "was",
        "were", "be", "been", "being", "have", "has", "had", "having", "do",
        "does", "did", "doing", "would", "could", "should", "may", "might",
        "must", "shall", "will", "that", "use", "this", "these", "which", "also",
        "one", "two", "three", "four" , "five", "six", "seven", "eight", "nine", "ten"

    };
}

void LibStemmer::init_common_words() {
    common_words_dict = {
        // Common nouns
        "sequence", "analysis", "structure", "figure", "patient", "model",
        "protein", "system", "region", "domain", "cell", "gene", "virus",
        "vaccine", "antibody", "treatment", "therapy", "hospital", "study",
        "research", "science", "medical", "clinical", "health", "disease",
        "infection", "symptom", "transmission", "mortality", "morbidity",
        "diagnosis", "prognosis", "treatment", "prevention", "control",
        
        // Common adjectives
        "different", "various", "several", "viral", "bacterial", "medical",
        "clinical", "scientific", "important", "significant", "critical",
        "severe", "mild", "acute", "chronic", "positive", "negative",
        
        // Common verbs
        "develop", "analyze", "study", "research", "test", "measure",
        "compare", "evaluate", "assess", "determine", "identify", "detect",
        "treat", "prevent", "control", "manage", "monitor", "observe"
    };
}

void LibStemmer::init_stemming_corrections() {
    stemming_corrections = {
        {"sequenc", "sequence"},
        {"analysi", "analysis"},
        {"structur", "structure"},
        {"figur", "figure"},
        {"howev", "however"},
        {"differ", "different"},
        {"inform", "information"},
        {"activ", "active"},
        {"product", "produce"},
        {"detect", "detection"},
        {"infect", "infection"},
        {"develop", "development"},
        {"measur", "measure"},
        {"test", "testing"},
        {"treat", "treatment"},
        {"prevent", "prevention"},
        {"control", "control"},
        {"manag", "manage"},
        {"monitor", "monitoring"},
        {"observ", "observe"},
        
        // Medical term specific corrections
        {"pneumonia", "pneumonia"},
        {"influenza", "influenza"},
        {"respiratori", "respiratory"},
        {"immun", "immune"},
        {"cytokin", "cytokine"},
        {"pathogen", "pathogen"},
        {"genom", "genome"},
        {"prote", "protein"},
        {"peptid", "peptide"},
        {"vaccin", "vaccine"},
        {"antibodi", "antibody"},
        {"antigen", "antigen"},
        {"receptor", "receptor"},
        {"membrane", "membrane"}
    };
}

bool LibStemmer::is_likely_noise(const std::string& text) const {
    if (text.length() == 1 && text != "a" && text != "i" && 
        medical_abbrev.find(text) == medical_abbrev.end()) {
        return true;
    }
    
    if (noise_words.find(text) != noise_words.end()) {
        return true;
    }
    
    if (text.empty()) return false;
    
    if (text[0] == '-' || text[0] == '.' || text[0] == ',' || 
        text[0] == '|' || text[0] == '/' || text[0] == '\\' ||
        text.back() == '-' || text.back() == '.' || text.back() == ',' ||
        text.back() == '|' || text.back() == '/' || text.back() == '\\') {
        return true;
    }
    
    std::regex url_pattern("https?://|www\\.|\\.(com|org|edu|gov|net|html?|php|asp|pdf)");
    if (std::regex_search(text, url_pattern)) {
        return true;
    }
    
    if (text.length() > 25) {
        return true;
    }
    
    int digit_count = std::count_if(text.begin(), text.end(), ::isdigit);
    return digit_count > text.length() * 0.3;
}

bool LibStemmer::is_stop_word(const std::string& word) const {
    return stop_words.find(word) != stop_words.end();
}

std::string LibStemmer::clean_token(const std::string& token) {
    std::string cleaned = token;
    
    // Remove punctuation from beginning and end
    while (!cleaned.empty() && ispunct(static_cast<unsigned char>(cleaned.front()))) {
        cleaned.erase(0, 1);
    }
    while (!cleaned.empty() && ispunct(static_cast<unsigned char>(cleaned.back()))) {
        cleaned.pop_back();
    }
    
    return cleaned;
}

bool LibStemmer::should_preserve(const std::string& word) const {
    return medical_preserve.find(word) != medical_preserve.end() ||
           medical_abbrev.find(word) != medical_abbrev.end();
}

std::string LibStemmer::improved_stem(const std::string& word) {
    if (word.empty()) return word;
    
    // Check if we should preserve this word
    if (should_preserve(word)) {
        return word;
    }
    
    // Check if it's a common word we want to keep
    if (common_words_dict.find(word) != common_words_dict.end()) {
        // For common words, only remove simple plural 's'
        if (word.length() > 3 && word.back() == 's') {
            std::string singular = word.substr(0, word.length() - 1);
            if (common_words_dict.find(singular) != common_words_dict.end()) {
                return singular;
            }
        }
        return word;
    }
    
    // Apply Snowball stemming
    std::string stemmed = stem_word(word);
    
    // Apply corrections if needed
    auto it = stemming_corrections.find(stemmed);
    if (it != stemming_corrections.end()) {
        return it->second;
    }
    
    // Check if stemmed word looks reasonable
    if (stemmed.length() >= 3 && stemmed != word) {
        // Only accept stemming if it looks like a real word
        if (stemmed.find_first_not_of("abcdefghijklmnopqrstuvwxyz-") != std::string::npos) {
            return word; // Keep original if stemmed looks weird
        }
    }
    
    return stemmed;
}

std::string LibStemmer::correct_stemming(const std::string& stemmed) {
    auto it = stemming_corrections.find(stemmed);
    return (it != stemming_corrections.end()) ? it->second : stemmed;
}

void LibStemmer::process_text_enhanced(const std::string& text,
                                      std::unordered_map<std::string, u32>& term_frequencies) 
{
    term_frequencies.clear();
    
    if (text.empty()) { return; }
    
    // Clean text
    std::string cleaned_text = text;
    std::replace(cleaned_text.begin(), cleaned_text.end(), '\n', ' ');
    std::replace(cleaned_text.begin(), cleaned_text.end(), '\r', ' ');
    std::replace(cleaned_text.begin(), cleaned_text.end(), '\t', ' ');
    
    // Remove control characters
    cleaned_text.erase(
        std::remove_if(cleaned_text.begin(), cleaned_text.end(),
            [](unsigned char c) {
                return c == '\0' || (c < 32 && c != ' ');
            }
        ),
        cleaned_text.end()
    );
    
    // Convert to lowercase
    std::transform(cleaned_text.begin(), cleaned_text.end(), cleaned_text.begin(), ::tolower);
    
    // Tokenize
    std::istringstream iss(cleaned_text);
    std::string token;
    
    while (iss >> token) {
        std::string cleaned = clean_token(token);
        
        if (cleaned.empty() || is_likely_noise(cleaned)) {
            continue;
        }
        
        // Skip very short words (unless medical abbreviation)
        if (cleaned.length() < 3 && !medical_abbrev.contains(cleaned)) {
            continue;
        }
        
        // Remove stop words more aggressively
        if (is_stop_word(cleaned) && !should_preserve(cleaned)) {
            continue;
        }
        
        std::string final_term;
        
        // Apply appropriate processing
        if (should_preserve(cleaned)) {
            final_term = cleaned; // Keep medical terms as-is
        } else if (common_words_dict.contains(cleaned)) {
            // For common words, use improved stemming
            final_term = improved_stem(cleaned);
        } else {
            // For other words, use standard stemming with correction
            std::string stemmed = stem_word(cleaned);
            final_term = correct_stemming(stemmed);
        }
        
        // Final validation
        if (final_term.empty() || final_term.length() < 2 || 
            is_likely_noise(final_term)) {
            continue;
        }
        
        // Add to frequencies
        term_frequencies[final_term]++;
    }
}

void LibStemmer::process_text(const std::string& text, 
                             std::unordered_map<std::string, u32>& term_frequencies) 
{
    process_text_enhanced(text, term_frequencies);
}

std::string LibStemmer::stem_word(const std::string& word) {
    if ((stemmer == nullptr) || word.empty()) {
        return word;
    }
    
    const sb_symbol* stemmed = sb_stemmer_stem(
        stemmer, 
        reinterpret_cast<const sb_symbol*>(word.c_str()), 
        static_cast<int>(word.length())
    );
    
    return stemmed ? std::string(reinterpret_cast<const char*>(stemmed)) : word;
}

std::vector<std::string> LibStemmer::stem_words(const std::vector<std::string>& words) {
    std::vector<std::string> result;
    result.reserve(words.size());
    
    for (const auto& word : words) {
        result.push_back(stem_word(word));
    }
    
    return result;
}

// Porter2 style stemmer (optional)
std::string LibStemmer::porter2_stem(const std::string& word) {
    if (word.length() < 3) return word;
    
    std::string result = word;
    
    // Remove plural 's'
    if (result.back() == 's') {
        std::string without_s = result.substr(0, result.length() - 1);
        if (without_s.length() >= 2 && 
            !(without_s.back() == 's' || without_s.back() == 'i')) {
            result = without_s;
        }
    }
    
    // Remove 'ing' suffix
    if (result.length() > 5 && result.substr(result.length() - 3) == "ing") {
        std::string base = result.substr(0, result.length() - 3);
        if (base.length() >= 2 && base.back() != base[base.length() - 2]) {
            result = base;
        }
    }
    
    // Remove 'ed' suffix
    if (result.length() > 4 && result.substr(result.length() - 2) == "ed") {
        std::string base = result.substr(0, result.length() - 2);
        if (base.length() >= 2) {
            result = base;
        }
    }
    
    // Remove 'ly' suffix
    if (result.length() > 4 && result.substr(result.length() - 2) == "ly") {
        result = result.substr(0, result.length() - 2);
    }
    
    return result;
}

std::string LibStemmer::stem_word_porter2(const std::string& word) {
    if (should_preserve(word) || common_words_dict.find(word) != common_words_dict.end()) {
        return word;
    }
    
    std::string stemmed = porter2_stem(word);
    
    // Apply corrections
    auto it = stemming_corrections.find(stemmed);
    if (it != stemming_corrections.end()) {
        return it->second;
    }
    
    return stemmed;
}
