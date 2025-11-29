#pragma once

#include <cstdint>



struct WordData {
    uint32_t word_id;     
    uint32_t freq;
};


struct InvertedEntry {
    uint32_t doc_id;
    uint32_t freq;
};


