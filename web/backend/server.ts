const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const readline = require('readline');

const app = express();
const PORT = process.env.PORT || 3001;

// Middleware
app.use(cors());
app.use(express.json());

// C++ search engine process
let searchEngine = null;
let engineReady = false;
let rl = null;

// Start C++ search engine
function startSearchEngine() {
    console.log('Starting C++ search engine...');
    
    searchEngine = spawn('./search_server', ['indices/'], {
        stdio: ['pipe', 'pipe', 'pipe']
    });
    
    // Setup readline for stdout
    rl = readline.createInterface({
        input: searchEngine.stdout,
        crlfDelay: Infinity
    });
    
    // Handle stderr (logs from C++)
    searchEngine.stderr.on('data', (data) => {
        console.log(`[C++ Engine]: ${data.toString().trim()}`);
        
        // Check if engine is ready
        if (data.toString().includes('Search Engine Ready')) {
            engineReady = true;
            console.log('✓ Search engine initialized successfully');
        }
    });
    
    searchEngine.on('error', (error) => {
        console.error('Failed to start search engine:', error);
        engineReady = false;
    });
    
    searchEngine.on('close', (code) => {
        console.log(`Search engine process exited with code ${code}`);
        engineReady = false;
        searchEngine = null;
    });
}

// Send command to C++ engine and get response
function sendCommand(command, data) {
    return new Promise((resolve, reject) => {
        if (!searchEngine || !engineReady) {
            reject(new Error('Search engine not ready'));
            return;
        }
        
        const request = {
            command: command,
            data: JSON.stringify(data)
        };
        
        // Setup one-time listener for response
        const responseHandler = (line) => {
            try {
                const response = JSON.parse(line);
                resolve(response);
            } catch (error) {
                reject(new Error('Invalid JSON response from engine'));
            }
        };
        
        // Listen for next line (the response)
        rl.once('line', responseHandler);
        
        // Send request
        searchEngine.stdin.write(JSON.stringify(request) + '\n');
        
        // Timeout after 30 seconds
        setTimeout(() => {
            rl.removeListener('line', responseHandler);
            reject(new Error('Search engine timeout'));
        }, 30000);
    });
}

// Routes

// Health check
app.get('/api/health', async (req, res) => {
    if (!engineReady) {
        return res.status(503).json({
            status: 'unavailable',
            message: 'Search engine not ready'
        });
    }
    
    try {
        const result = await sendCommand('health', {});
        res.json(result);
    } catch (error) {
        res.status(500).json({
            status: 'error',
            message: error.message
        });
    }
});

// Search endpoint
app.post('/api/search', async (req, res) => {
    const { query, top_k = 10 } = req.body;
    
    if (!query || query.trim() === '') {
        return res.status(400).json({
            status: 'error',
            message: 'Query parameter is required'
        });
    }
    
    try {
        const result = await sendCommand('search', { query, top_k });
        res.json(result);
    } catch (error) {
        res.status(500).json({
            status: 'error',
            message: error.message
        });
    }
});

// Get document details
app.get('/api/document/:doc_id', async (req, res) => {
    const doc_id = parseInt(req.params.doc_id);
    
    if (isNaN(doc_id)) {
        return res.status(400).json({
            status: 'error',
            message: 'Invalid document ID'
        });
    }
    
    try {
        const result = await sendCommand('get_document', { doc_id });
        res.json(result);
    } catch (error) {
        res.status(500).json({
            status: 'error',
            message: error.message
        });
    }
});

// Get statistics
app.get('/api/stats', async (req, res) => {
    try {
        const result = await sendCommand('stats', {});
        res.json(result);
    } catch (error) {
        res.status(500).json({
            status: 'error',
            message: error.message
        });
    }
});

// Start server
startSearchEngine();

app.listen(PORT, () => {
    console.log(`\n🚀 Server running on http://localhost:${PORT}`);
    console.log(`   API endpoints:`);
    console.log(`   POST http://localhost:${PORT}/api/search`);
    console.log(`   GET  http://localhost:${PORT}/api/document/:id`);
    console.log(`   GET  http://localhost:${PORT}/api/stats`);
    console.log(`   GET  http://localhost:${PORT}/api/health\n`);
});

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\nShutting down...');
    
    if (searchEngine) {
        sendCommand('shutdown', {}).catch(() => {});
        setTimeout(() => {
            searchEngine.kill();
            process.exit(0);
        }, 1000);
    } else {
        process.exit(0);
    }
});


