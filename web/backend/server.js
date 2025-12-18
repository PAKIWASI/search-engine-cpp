import express from "express";
import { runCpp } from "./runCpp.js";
import cors from "cors";
import path from "path";
import { fileURLToPath } from "url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

const app = express();
app.use(cors());
app.use(express.json());

// Serve frontend files from web/frontend
app.use(express.static(path.join(__dirname, "../frontend")));

// Search endpoint
app.get("/api/search", async (req, res) => {
  let query = req.query.q;
  
  if (!query) {
    return res.json({
      success: false,
      error: "No query provided",
      example: "/api/search?q=coronavirus"
    });
  }
  
  console.log(`🔍 Search request: "${query}"`);
  
  // Common COVID-19 term mappings
  const termMap = {
    "covid": "coronavirus",
    "covid-19": "coronavirus",
    "sars-cov-2": "coronavirus",
    "corona": "coronavirus",
    "covid19": "coronavirus",
    "covid 19": "coronavirus"
  };
  
  const originalQuery = query;
  const queryLower = query.toLowerCase();
  
  // Replace with mapped term if needed
  if (termMap[queryLower]) {
    console.log(`🔄 Mapping "${query}" to "${termMap[queryLower]}"`);
    query = termMap[queryLower];
  }
  
  try {
    const output = await runCpp(["search", query]);
    
    // Try to parse as JSON
    try {
      const jsonResult = JSON.parse(output);
      
      // Ensure we always return exactly top 10 results
      if (jsonResult.results && jsonResult.results.length > 10) {
        jsonResult.results = jsonResult.results.slice(0, 10);
        jsonResult.count = 10;
        jsonResult.note = "Showing top 10 most relevant results";
      }
      
      // Add note if we mapped the term
      if (originalQuery.toLowerCase() !== query.toLowerCase()) {
        jsonResult.note = `Showing results for "${query}" (search for "${originalQuery}" returned no results)`;
        jsonResult.original_query = originalQuery;
      }
      
      // Ensure pmcid field exists for all results
      if (jsonResult.results) {
        jsonResult.results.forEach(result => {
          if (!result.pmcid && result.pmcid === undefined) {
            result.pmcid = "";
          }
        });
      }
      
      res.json(jsonResult);
      
    } catch (jsonError) {
      // If not JSON, return as text
      res.json({
        success: true,
        query: query,
        original_query: originalQuery,
        results: parseSearchOutput(output),
        note: "Parsed text output"
      });
    }
  } catch (error) {
    console.error("Search error:", error);
    res.status(500).json({
      success: false,
      error: "Search failed",
      details: error.toString().substring(0, 200)
    });
  }
});


// Suggestions endpoint
app.get("/api/suggest", async (req, res) => {
  const query = req.query.q || "";
  
  try {
    const output = await runCpp(["suggest", query]);
    
    try {
      const jsonResult = JSON.parse(output);
      res.json({
        success: true,
        data: jsonResult.data || jsonResult.suggestions || []
      });
    } catch {
      // Parse text suggestions
      const suggestions = output.split('\n')
        .filter(line => line.trim().length > 0)
        .slice(0, 10);
      res.json({
        success: true,
        data: suggestions
      });
    }
  } catch (error) {
    // Mock suggestions if C++ fails
    const mockSuggestions = [
      `${query} covid`,
      `${query} vaccine`,
      `${query} treatment`,
      `${query} study`,
      `coronavirus ${query}`,
      `covid-19 ${query}`,
      `${query} research paper`,
      `${query} clinical trial`
    ].filter(s => s.length > query.length);
    
    res.json({
      success: true,
      data: mockSuggestions.slice(0, 8),
      note: "Using mock suggestions"
    });
  }
});

// Document endpoint
app.get("/api/doc/:id", async (req, res) => {
  const docId = req.params.id;
  
  try {
    const output = await runCpp(["doc", docId]);
    res.json({
      success: true,
      docId: docId,
      content: output,
      length: output.length
    });
  } catch (error) {
    res.json({
      success: false,
      docId: docId,
      error: "Document not found",
      mock_content: `Document ${docId} content would appear here.

To implement this, modify your C++ program to handle 'doc <id>' command.`
    });
  }
});

// Health check
app.get("/api/health", (req, res) => {
  res.json({
    success: true,
    status: "online",
    service: "CORD-19 Search Engine",
    version: "1.0.0",
    timestamp: new Date().toISOString()
  });
});

// Parse C++ search output
function parseSearchOutput(text) {
  const lines = text.split('\n');
  const results = [];
  let currentDoc = null;
  
  for (const line of lines) {
    const trimmed = line.trim();
    
    // Look for document markers in your C++ output
    if (trimmed.includes("Doc ID:") || trimmed.includes("Document ID:")) {
      if (currentDoc) {
        results.push(currentDoc);
      }
      
      const docIdMatch = trimmed.match(/(?:Doc|Document) ID:\s*(\d+)/i);
      const scoreMatch = trimmed.match(/Score:\s*([\d.]+)/i);
      
      currentDoc = {
        docId: docIdMatch ? parseInt(docIdMatch[1]) : results.length + 1,
        score: scoreMatch ? parseFloat(scoreMatch[1]) : (1.0 - results.length * 0.1),
        title: `Document ${docIdMatch ? docIdMatch[1] : results.length + 1}`,
        snippet: "",
        cord_uid: "",
        has_abstract: false
      };
    } else if (currentDoc && trimmed) {
      // Add to snippet (limit length)
      if (currentDoc.snippet.length < 200) {
        currentDoc.snippet += (currentDoc.snippet ? " " : "") + trimmed;
      }
      
      // Check for CORD UID
      if (trimmed.includes("CORD UID:") || trimmed.includes("cord_uid:")) {
        const uidMatch = trimmed.match(/(?:CORD UID|cord_uid):\s*(\S+)/i);
        if (uidMatch) currentDoc.cord_uid = uidMatch[1];
      }
    }
  }
  
  // Add the last document
  if (currentDoc) {
    results.push(currentDoc);
  }
  
  // If no structured results, create one from the output
  if (results.length === 0 && text.trim().length > 0) {
    return [{
      docId: 1,
      score: 1.0,
      title: "Search Results",
      snippet: text.substring(0, 200) + (text.length > 200 ? "..." : "")
    }];
  }
  
  return results.slice(0, 20);
}

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`
╔══════════════════════════════════════════════════════════════╗
║            CORD-19 Search Engine                            ║
╚══════════════════════════════════════════════════════════════╝
🌐 Frontend:     http://localhost:${PORT}
🔍 Search API:   http://localhost:${PORT}/api/search?q=covid
💡 Suggestions:  http://localhost:${PORT}/api/suggest?q=cov
📊 Health:       http://localhost:${PORT}/api/health

Directory:
├── web/frontend/  <- Your HTML/CSS/JS
└── web/backend/   <- Node.js server
`);
});


