# COVID-19 Research Search Engine

A full-stack search engine for COVID-19 research papers using C++ backend with BM25 ranking, Node.js API server, and React frontend.

## Architecture

```
Frontend (React) → Backend (Node.js/Express) → Search Engine (C++)
     :3000              :3001                   stdin/stdout IPC
```

## Prerequisites

- C++17 compiler (g++)
- Node.js (v14+)
- Python 3 with spaCy
- Make

## Setup Instructions

### 1. Build C++ Search Engine

```bash
# Compile the index builder
make builder

# Compile the search server
make server
```

### 2. Create Indices (First Time Only)

If you haven't built indices yet:

```bash
# Edit main.cpp to point to your data path
# Then run:
./index_builder

# This will create files in indices/:
# - lexicon_cordR1.bin
# - forward_index_cordR1.bin
# - 0.bin, 1.bin, 2.bin, 3.bin (barrel files)
# - barrel_metadata.bin
```

### 3. Setup Backend (Node.js)

```bash
# Create backend directory
mkdir backend
cd backend

# Copy server.js and package.json to this directory

# Install dependencies
npm install

# Start the API server
npm start
```

The backend will start on `http://localhost:3001`

### 4. Setup Frontend (React)

```bash
# Create React app
npx create-react-app frontend
cd frontend

# Copy App.jsx and App.css to src/

# Install dependencies (if needed)
npm install

# Start the React development server
npm start
```

The frontend will start on `http://localhost:3000`

## Running the Application

You need 3 terminals:

**Terminal 1: C++ Search Engine**
```bash
./search_server indices/
```

**Terminal 2: Node.js Backend**
```bash
cd backend
npm start
```

**Terminal 3: React Frontend**
```bash
cd frontend
npm start
```

Then open your browser to `http://localhost:3000`

## API Endpoints

- `POST /api/search` - Search for documents
  ```json
  {
    "query": "covid vaccine efficacy",
    "top_k": 10
  }
  ```

- `GET /api/document/:id` - Get document details
- `GET /api/stats` - Get collection statistics
- `GET /api/health` - Health check

## Project Structure

```
.
├── cpp_source/
│   ├── common_includes.hpp
│   ├── lexicon.hpp/cpp
│   ├── forward_index.hpp/cpp
│   ├── inverted_index.hpp/cpp
│   ├── text_processor.hpp/cpp
│   ├── query_processor.hpp/cpp      # NEW
│   ├── search_api.hpp/cpp           # NEW
│   ├── metadata_parser.hpp/cpp
│   ├── main.cpp                     # Index builder
│   └── search_server.cpp            # Search API
├── python/
│   └── lemmatizer_daemon.py
├── backend/
│   ├── server.js
│   └── package.json
├── frontend/
│   ├── src/
│   │   ├── App.jsx
│   │   └── App.css
│   └── package.json
├── indices/
│   └── (generated binary files)
└── Makefile
```

## Features

### C++ Engine
- BM25 ranking algorithm
- Barrel-based inverted index (memory efficient)
- Lemmatization via Python daemon
- Fast binary serialization

### Backend API
- RESTful JSON API
- IPC communication with C++ engine
- CORS enabled for local development
- Graceful shutdown handling

### Frontend
- Clean, modern UI
- Real-time search
- Document preview
- Full document viewer modal
- Responsive design
- Collection statistics display

## Customization

### BM25 Parameters
Edit `query_processor.hpp`:
```cpp
const double k1 = 1.2;    // Term frequency saturation
const double b = 0.75;    // Length normalization
```

### Number of Barrels
Edit `inverted_index.hpp`:
```cpp
static const u32 num_barrel = 4;  // Adjust based on RAM
```

### Search Results
Default is 10 results, configurable in frontend or API request.

## Performance Tips

1. **RAM Usage**: Each barrel loads ~250MB. With 4 barrels, expect ~1GB RAM usage.

2. **Barrel Strategy**: Distribute word_ids evenly across barrels. Current implementation divides by range.

3. **Caching**: Consider adding LRU cache for frequently accessed posting lists.

4. **Concurrent Requests**: Current implementation handles one search at a time. For production, consider process pool.

## Troubleshooting

### "Search engine not ready"
- Ensure `./search_server` is running
- Check indices/ folder has all required files
- Look for errors in Terminal 1

### "Failed to load barrel"
- Verify all barrel files (0.bin, 1.bin, etc.) exist
- Check barrel_metadata.bin exists
- Ensure indices were built successfully

### "Python daemon failed"
- Check Python virtual environment is activated
- Verify spaCy model is installed: `python -m spacy download en_core_web_sm`
- Check python/.venv/bin/python3 path is correct

### Frontend can't connect
- Ensure backend is running on port 3001
- Check CORS is enabled in server.js
- Verify API_URL in App.jsx matches backend URL

## Future Enhancements

- [ ] Query suggestions/autocomplete
- [ ] Advanced filters (date, document type)
- [ ] Phrase queries
- [ ] Boolean operators (AND, OR, NOT)
- [ ] User authentication
- [ ] Save search history
- [ ] Export results
- [ ] Distributed indexing
- [ ] Caching layer (Redis)
- [ ] Docker containerization

## License

MIT

## Contributors

Built for COVID-19 research paper retrieval using the CORD-19 dataset.
