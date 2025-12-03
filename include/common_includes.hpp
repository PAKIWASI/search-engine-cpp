#pragma once

#include <cstdint>



typedef uint32_t u32;


struct WordData {
    u32 word_id;     // assigned id 
    u32 freq;        // freq of the word
};


struct InvertedEntry {
    u32 doc_id;     // assigned id
    u32 freq;       // freq of a word in that doc
};


