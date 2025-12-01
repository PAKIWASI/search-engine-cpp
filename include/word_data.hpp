#pragma once

#include <cstdint>



struct WordData {
    uint32_t word_id;     // assigned id 
    uint32_t freq;        // freq of the word
};


struct InvertedEntry {
    uint32_t doc_id;     // assigned id
    uint32_t freq;       // freq of a word in that doc
};


