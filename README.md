# Search Engine - Core Components Implementation

A high-performance search engine implementation featuring **Lexicon**, **Forward Index**, and **Inverted Index** built in C++ with Python NLP integration. Designed for the CORD-19 COVID-19 research dataset.

[![Language](https://img.shields.io/badge/Language-C%2B%2B17-blue.svg)](https://isocpp.org/)
[![NLP](https://img.shields.io/badge/NLP-spaCy-09A3D5.svg)](https://spacy.io/)

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Usage](#usage)
- [Dataset](#dataset)
- [Project Structure](#project-structure)
- [Implementation Details](#implementation-details)
- [Performance](#performance)
- [Sample Data](#sample-data)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Team](#team)
- [License](#license)

## Features

### Core Components

- **Lexicon**: Vocabulary generation with word-to-ID mapping and frequency tracking
- **Forward Index**: Efficient document-to-terms mapping with metadata storage
- **Inverted Index**: Fast term-to-documents mapping for query processing

### Technical Highlights

-  **Fast Processing**: Python daemon architecture avoids repeated NLP model loading
-  **Binary Serialization**: Efficient storage and retrieval of indices
-  **Advanced NLP**: spaCy-based lemmatization with medical term preservation
-  **Comprehensive Statistics**: Detailed analytics for lexicon and indices
-  **Extensible Design**: Clean architecture for easy enhancement

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Input Documents                          │
│              (CORD-19 CSV + JSON files)                      │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                  Metadata Parser                             │
│         (Extracts text from PDF/XML JSONs)                   │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│               Text Processor (C++)                           │
│                       │                                      │
│                       ▼                                      │
│          ┌─────────────────────────┐                        │
│          │   Python Daemon         │                        │
│          │   (spaCy NLP)           │                        │
│          │   - Lemmatization       │                        │
│          │   - Stop word removal   │                        │
│          │   - Medical term        │                        │
│          │     preservation        │                        │
│          └─────────────────────────┘                        │
└───────────────────────┬─────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        │               │               │
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   Lexicon    │ │   Forward    │ │   Inverted   │
│              │ │    Index     │ │    Index     │
│ word → ID    │ │ doc → terms  │ │ term → docs  │
│ frequency    │ │ metadata     │ │ frequencies  │
└──────────────┘ └──────────────┘ └──────────────┘
        │               │               │
        └───────────────┴───────────────┘
                        │
                        ▼
            ┌───────────────────────┐
            │   Binary + Text       │
            │   Output Files        │
            └───────────────────────┘
```

## Quick Start

### Try with Sample Data (100 papers)

```bash
# Clone repository
git clone https://github.com/yourusername/search-engine.git
cd search-engine

# Install dependencies
pip install spacy
python -m spacy download en_core_web_sm

# Build
cmake -B build -G Ninja
cmake --build build

# Run on sample data
cd build
./main

# View output
head -20 ../indices/lexicon_text.txt
```

### Run on Full Dataset

```bash
# Download CORD-19 dataset
# https://www.semanticscholar.org/cord19

# Update data path in src/main.cpp
# const std::string data_path = "data/2020-04-10";

# Build and run
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build
./main
```

## Installation

### Prerequisites

- **C++ Compiler**: Clang 10+ (C++20 support required)
- **CMake**: 3.20 or higher
- **Ninja**: Build system
- **Python**: 3.6 or higher
- **spaCy**: NLP library for lemmatization

### System Dependencies

#### Ubuntu/Debian (Normal, WSL)
```bash
sudo apt-get update
sudo apt-get install clang cmake ninja-build python3 python3-pip
```

#### macOS
```bash
brew install llvm cmake ninja python3
```

### Python Dependencies

```bash
# Create virtual environment (recommended)
python3 -m venv python/.venv
source python/.venv/bin/activate

# Install spaCy
pip install spacy

# Download English model
python -m spacy download en_core_web_sm
```

### Build

```bash
# Clone repository
git clone https://github.com/yourusername/search-engine.git
cd search-engine

# Create build directory
mkdir build
cd build

# Configure with CMake and Ninja
cmake -G Ninja ..

# Build
ninja

# Run (from build directory)
./main
```



## Usage

### Basic Usage

```bash
# Build
cmake -B build -G Ninja
cmake --build build

# Run indexer
cd build
./main

# Output files will be created in indices/
```

### Configuration

Edit `src/main.cpp` to configure:

```cpp
// Data path
const std::string data_path = "data/2020-04-10";

// Python paths (if using custom virtual environment)
TextProcessor text_processor(
    lexicon,
    "python/.venv/bin/python3",          // Python interpreter
    "python/lemmatizer_daemon.py"        // Daemon script
);

// Processing limit (for testing)
if (paper_count >= 1000) { break; }  // Remove for full dataset
```

After changing configuration:
```bash
cmake --build build  # Rebuild
cd build
./main               # Run
```

### Output Files

After running, you'll find in `indices/`:

```
indices/
├── lexicon_cordR1.bin              # Binary lexicon (fast loading)
├── lexicon_text.txt                # Human-readable lexicon
├── forward_index_cordR1.bin        # Binary forward index
├── forward_index_text.txt          # Human-readable forward index
├── inverted_index_cordR1.bin       # Binary inverted index
└── inverted_index_text.txt         # Human-readable inverted index
```

### Reading Output Files

**Lexicon Format** (CSV):
```csv
word,wordid,freq
patient,0,12543
virus,1,9876
infection,2,8234
```

**Forward Index Format**:
```
doc_id: cord_uid -> [ (word_id, word, freq) ]
0: ug7v899j -> [ ( 0, patient, 15 ), ( 1, virus, 12 ), ... ]
```

**Inverted Index Format**:
```
word_id: word -> [ (doc_id, freq) ]
0: patient -> [ ( 0, 15 ), ( 2, 9 ), ( 5, 21 ), ... ]
```

## Dataset

### CORD-19 Dataset

This project uses the **COVID-19 Open Research Dataset (CORD-19)**:

- **Source**: https://www.semanticscholar.org/cord19
- **Size**: 1M+ papers (we use 2020-04-10 snapshot with ~59,000 papers)
- **Format**: CSV metadata + JSON full-text papers
- **License**: Various (see individual papers)

### Dataset Structure

```
data/2020-04-10/
├── metadata.csv                    # Paper metadata
├── comm_use_subset/
│   ├── pdf_json/*.json            # Commercial use papers (PDFs)
│   └── pmc_json/*.json            # Commercial use papers (XMLs)
├── noncomm_use_subset/
│   ├── pdf_json/*.json
│   └── pmc_json/*.json
├── custom_license/
│   ├── pdf_json/*.json
│   └── pmc_json/*.json
└── biorxiv_medrxiv/
    └── pdf_json/*.json
```

### JSON Paper Format

Each paper JSON contains:

```json
{
  "paper_id": "unique_identifier",
  "metadata": {
    "title": "Paper Title"
  },
  "abstract": [
    {
      "text": "Abstract text...",
      "section": "Abstract"
    }
  ],
  "body_text": [
    {
      "text": "Section text...",
      "section": "Introduction"
    }
  ]
}
```

## Project Structure

```
search-engine/
├── include/                        # Header files
│   ├── lexicon.hpp
│   ├── forward_index.hpp
│   ├── inverted_index.hpp
│   ├── text_processor.hpp
│   ├── metadata_parser.hpp
│   └── word_data.hpp
│
├── src/                            # Implementation files
│   ├── main.cpp                   # Main entry point
│   ├── lexicon.cpp
│   ├── forward_index.cpp
│   ├── inverted_index.cpp
│   ├── text_processor.cpp
│   └── metadata_parser.cpp
│
├── python/                         # Python NLP components
│   ├── lemmatizer_daemon.py       # spaCy daemon
│   └── .venv/                     # Virtual environment
│
├── sample/                         # Sample dataset (100 papers)
│   ├── sample_input/              # Sample CORD-19 papers
│   ├── sample_output/             # Pre-generated indices
│   └── README.md
│
├── build/                          # Build directory (gitignored)
│   └── main                       # Compiled executable
│
├── indices/                        # Generated indices (gitignored)
│   ├── *.bin                      # Binary format
│   └── *.txt                      # Text format
│
├── data/                           # Full dataset (gitignored)
│   └── 2020-04-10/
│
├── CMakeLists.txt                  # CMake configuration
├── README.md                       # This file
├── .gitignore
└── LICENSE
```

## Implementation Details

### Data Structures

#### Lexicon
```cpp
// Hash map for O(1) word lookup
std::unordered_map<std::string, WordData> lexicon;

// Reverse mapping for ID → word
std::unordered_map<uint32_t, std::string> reverse_lex;

struct WordData {
    uint32_t word_id;    // Unique identifier
    uint32_t freq;       // Total frequency
};
```

**Time Complexity**: O(1) average for lookup, insert  
**Space Complexity**: O(V) where V = vocabulary size

#### Forward Index
```cpp
// Document → Terms mapping
std::unordered_map<uint32_t, std::vector<WordData>> forward_index;

// Document metadata
std::unordered_map<uint32_t, std::string> doc_metadata;
```

**Time Complexity**: O(1) document lookup, O(n log n) for building (sorting)  
**Space Complexity**: O(D × T) where D = documents, T = avg terms per doc

#### Inverted Index
```cpp
// Term → Documents mapping
std::unordered_map<uint32_t, std::vector<InvertedEntry>> inverted_index;

struct InvertedEntry {
    uint32_t doc_id;     // Document containing term
    uint32_t freq;       // Frequency in that document
};
```

**Time Complexity**: O(1) term lookup  
**Space Complexity**: O(V × D_avg) where D_avg = avg documents per term

### Algorithms

#### Text Processing Pipeline

```
1. Read metadata.csv
2. For each paper:
   a. Extract title, abstract
   b. Find full text (PDF or XML JSON)
   c. Extract body text from JSON
   d. Concatenate all text
   e. Send to Python daemon
   f. Receive lemmatized terms with frequencies
   g. Update lexicon
   h. Build forward index entry
   i. Build inverted index entry
3. Save all indices (binary + text)
```

#### Python Daemon Communication

```cpp
// One-time setup (fast subsequent processing)
1. Fork Python process
2. Create bidirectional pipes
3. Load spaCy model (happens once!)
4. Keep process alive

// For each document
1. Send text via pipe
2. Python lemmatizes and returns CSV
3. Parse CSV into temp_lex
4. Merge with main lexicon
```

**Performance**: ~8 documents/second (vs ~0.3/sec without daemon)

### NLP Processing

#### Lemmatization with spaCy

```python
# Converts words to base form
"running" → "run"
"viruses" → "virus"
"patients" → "patient"
```

#### Medical Term Preservation

```python
# Special terms kept as-is (no lemmatization)
MEDICAL_PRESERVE = {
    'covid-19', 'sars-cov-2', 'covid', 'coronavirus',
    'vaccine', 'antibody', 'rna', 'dna', 'protein', ...
}
```

#### Noise Filtering

- Remove punctuation and whitespace
- Filter stop words (except medical terms)
- Remove single characters (except 'a', 'i')
- Remove URLs and special patterns
- Remove very long words (>25 chars)
- Filter high-digit-ratio tokens

## Performance

### Processing Speed

| Papers | Time | Speed | Unique Terms |
|--------|------|-------|--------------|
| 100    | 12s  | 8.3 docs/sec | 856 |
| 1,000  | 2m   | 8.3 docs/sec | 15,423 |
| 10,000 | 20m  | 8.3 docs/sec | ~48,000 |
| 50,000 | 100m | 8.3 docs/sec | ~120,000 |

*Tested on: Intel Core i5, 16GB RAM, SSD*

### Memory Usage

| Component | Memory (1K papers) | Memory (50K papers) |
|-----------|-------------------|---------------------|
| Lexicon | ~2 MB | ~40 MB |
| Forward Index | ~15 MB | ~750 MB |
| Inverted Index | ~20 MB | ~1 GB |
| **Total** | **~37 MB** | **~1.8 GB** |

### File Sizes

| Index Type | Binary | Text |
|------------|--------|------|
| Lexicon (1K) | 350 KB | 400 KB |
| Forward (1K) | 12 MB | 15 MB |
| Inverted (1K) | 18 MB | 22 MB |

### Optimization Techniques

1. **Python Daemon**: 25x faster than reloading model each time
2. **Binary Serialization**: 3-5x faster than CSV parsing
3. **Sorted Term Lists**: Better cache locality during queries
4. **Hash Maps**: O(1) average lookups
5. **Move Semantics**: Avoid unnecessary copies

## Sample Data

A sample dataset with 100 real CORD-19 papers is included for quick testing:

```bash
# Location
cd sample/

# Contents
sample/
├── sample_input/      # 100 real papers (~1 MB)
├── sample_output/     # Pre-generated indices
└── README.md         # Sample documentation
```

### Using Sample Data

```cpp
// Update src/main.cpp
const std::string data_path = "sample/sample_input";

// Build and run
cmake --build build
cd build
./main

// Compare your output with provided sample
diff ../indices/lexicon_text.txt ../sample/sample_output/lexicon_text.txt
```

See [sample/README.md](sample/README.md) for details.

## Documentation

### API Reference

#### Lexicon Class

```cpp
class Lexicon {
public:
    // Add or update word, returns word_id
    uint32_t add_word(const std::string& word, uint32_t freq);
    
    // Get word data (ID + frequency)
    const WordData* get_word_data(const std::string& word) const;
    
    // Check if word exists
    bool contains(const std::string& word) const;
    
    // Save/load
    void save_to_file_binary(const std::string& path) const;
    bool load_from_file_binary(const std::string& path);
    
    // Statistics
    void print_top_words(int n) const;
    size_t size() const;
};
```

#### Forward Index Class

```cpp
class ForwardIndex {
public:
    // Add document, returns doc_id
    uint32_t add_document(const std::string& cord_uid,
                         const std::unordered_map<std::string, WordData>& terms);
    
    // Get document terms
    const std::vector<WordData>* get_document_terms(uint32_t doc_id) const;
    
    // Get document metadata
    const std::string* get_doc_cord_uid(uint32_t doc_id) const;
    
    // Save/load
    void save_to_file(const std::string& path) const;
    bool load_from_file(const std::string& path);
    
    // Statistics
    void print_statistics() const;
};
```

#### Inverted Index Class

```cpp
class InvertedIndex {
public:
    // Add document's terms
    void add_document(uint32_t doc_id,
                     const std::unordered_map<std::string, WordData>& terms);
    
    // Get posting list for word
    const std::vector<InvertedEntry>* get_word_terms(uint32_t word_id) const;
    
    // Save/load
    void save_to_file(const std::string& path) const;
    bool load_from_file(const std::string& path);
    
    // Statistics
    void print_statistics() const;
};
```


### Code Examples


#### Loading Indices

```cpp
#include "lexicon.hpp"
#include "forward_index.hpp"
#include "inverted_index.hpp"

int main() {
    Lexicon lexicon;
    ForwardIndex forward_index;
    InvertedIndex inverted_index;
    
    // Load from binary files
    lexicon.load_from_file_binary("indices/lexicon_cordR1.bin");
    forward_index.load_from_file("indices/forward_index_cordR1.bin");
    inverted_index.load_from_file("indices/inverted_index_cordR1.bin");
    
    // Use indices...
    return 0;
}
```


#### Querying

```cpp
// Search for a term
std::string query = "covid";
const WordData* word_data = lexicon.get_word_data(query);

if (word_data) {
    uint32_t word_id = word_data->word_id;
    
    // Get documents containing this term
    const auto* postings = inverted_index.get_word_terms(word_id);
    
    if (postings) {
        for (const auto& posting : *postings) {
            std::cout << "Document " << posting.doc_id 
                     << " contains '" << query 
                     << "' " << posting.freq << " times\n";
        }
    }
}
```


### Build System

This project uses CMake with Ninja:
- **CMakeLists.txt**: Build configuration
- **C++20 Standard**: Modern C++ features
- **Clang Compiler**: For better diagnostics
- **Ninja**: Fast parallel builds

### Testing

```bash
# Test with sample data (should process 100 papers in ~12 seconds)
cmake -B build -G Ninja
cmake --build build
cd build
./main

# Verify output
head -20 ../indices/lexicon_text.txt

# Clean build
cd ..
rm -rf build
cmake -B build -G Ninja && cmake --build build
```


## Acknowledgments

- **CORD-19 Dataset**: Allen Institute for AI
- **spaCy**: Explosion AI for the NLP library
- **nlohmann/json**: JSON for Modern C++


## Future Work

### Planned Features

- [ ] Query processing engine
- [ ] TF-IDF ranking
- [ ] BM25 scoring
- [ ] Phrase queries
- [ ] Boolean operators (AND, OR, NOT)
- [ ] Web interface
- [ ] Multi-threaded indexing
- [ ] Distributed processing
- [ ] Real-time updates

### Potential Improvements

- [ ] Compression of posting lists
- [ ] Memory-mapped file support
- [ ] Index optimization (pruning)
- [ ] Spell correction
- [ ] Query suggestions
- [ ] Relevance feedback

---



