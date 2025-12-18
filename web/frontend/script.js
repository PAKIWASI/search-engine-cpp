const searchInput = document.getElementById('searchInput');
const suggestionsDiv = document.getElementById('suggestions');
const resultsContainer = document.getElementById('resultsContainer');
const queryTimeDiv = document.getElementById('queryTime');

let debounceTimer;

// Handle search input with debouncing
searchInput.addEventListener('input', () => {
  clearTimeout(debounceTimer);
  const query = searchInput.value.trim();
  
  if (query.length < 2) {
    hideSuggestions();
    return;
  }
  
  debounceTimer = setTimeout(() => {
    fetchSuggestions(query);
  }, 300);
});

// Handle Enter key
searchInput.addEventListener('keypress', (e) => {
  if (e.key === 'Enter') {
    performSearch();
  }
});

// Fetch search suggestions from API
async function fetchSuggestions(query) {
  try {
    const response = await fetch(`/api/suggest?q=${encodeURIComponent(query)}`);
    const data = await response.json();
    
    if (data.success && data.data && data.data.length > 0) {
      showSuggestions(data.data);
    } else {
      hideSuggestions();
    }
  } catch (error) {
    console.error('Error fetching suggestions:', error);
    hideSuggestions();
  }
}

// Show suggestions dropdown
function showSuggestions(suggestions) {
  suggestionsDiv.innerHTML = '';
  
  suggestions.forEach(suggestion => {
    const div = document.createElement('div');
    div.className = 'suggestion-item';
    div.textContent = suggestion;
    div.onclick = () => {
      searchInput.value = suggestion;
      hideSuggestions();
      performSearch();
    };
    suggestionsDiv.appendChild(div);
  });
  
  suggestionsDiv.style.display = 'block';
}

// Hide suggestions dropdown
function hideSuggestions() {
  suggestionsDiv.style.display = 'none';
  suggestionsDiv.innerHTML = '';
}

// Perform search
async function performSearch() {
  const query = searchInput.value.trim();
  if (!query) {
    resultsContainer.innerHTML = '<div class="no-results">Please enter a search query</div>';
    queryTimeDiv.textContent = '';
    return;
  }
  
  hideSuggestions();
  showLoading(query);
  
  try {
    const startTime = Date.now();
    const response = await fetch(`/api/search?q=${encodeURIComponent(query)}`);
    const data = await response.json();
    const timeMs = Date.now() - startTime;
    
    if (data.success) {
      displayResults(data.results, query, timeMs);
    } else {
      showError(data.error || 'Search failed');
    }
  } catch (error) {
    console.error('Search error:', error);
    showError('Network error. Please check if the server is running.');
  }
}

// Show loading state
function showLoading(query) {
  resultsContainer.innerHTML = `
    <div class="loading">
      <p>Searching for "${query}"...</p>
      <p>Analyzing COVID-19 research papers...</p>
    </div>
  `;
  queryTimeDiv.textContent = '';
}

// Display search results - UPDATED WITH CLICKABLE LINKS
function displayResults(results, query, timeMs) {
  if (!results || results.length === 0) {
    resultsContainer.innerHTML = `
      <div class="no-results">
        <p>No results found for "${query}"</p>
        <p>Try different keywords or check your spelling.</p>
        <p style="margin-top: 15px; font-size: 13px;">
          Suggestions: coronavirus, vaccine, treatment, pandemic, clinical trial
        </p>
      </div>
    `;
    queryTimeDiv.textContent = `Search completed in ${timeMs}ms`;
    return;
  }
  
  let html = '';
  
  results.forEach(result => {
    // Build PMC URL from pmcid
    let pmcLink = '#';
    let linkTitle = result.title || `Document ${result.docId}`;
    
    if (result.pmcid && result.pmcid.trim() !== '') {
      // Clean up pmcid - remove any whitespace, ensure it starts with PMC
      let cleanPmcid = result.pmcid.trim();
      if (!cleanPmcid.toUpperCase().startsWith('PMC')) {
        cleanPmcid = 'PMC' + cleanPmcid;
      }
      pmcLink = `https://pmc.ncbi.nlm.nih.gov/articles/${cleanPmcid}/`;
    } else if (result.cord_uid) {
      // Fallback: Use CORD UID to search on CORD-19 site
      pmcLink = `https://www.semanticscholar.org/cord19/search?q=${encodeURIComponent(result.cord_uid)}`;
      linkTitle += ' (View on CORD-19)';
    }
    
    html += `
      <div class="result-item">
        <a href="${pmcLink}" class="result-title" target="_blank" rel="noopener noreferrer">
          ${linkTitle}
        </a>
        <div class="result-meta">
          ${result.score ? `<span class="result-score">Score: ${result.score.toFixed(4)}</span>` : ''}
          <span class="result-id">Document ID: ${result.docId}</span>
          ${result.pmcid ? `<span class="result-pmcid">PMCID: ${result.pmcid}</span>` : ''}
          ${result.cord_uid ? `<span class="result-cord">CORD: ${result.cord_uid}</span>` : ''}
        </div>
        ${result.abstract ? `<div class="result-abstract">${result.abstract}</div>` : ''}
        ${result.snippet ? `<div class="result-snippet">${result.snippet}</div>` : ''}
        <div class="result-actions">
          <a href="${pmcLink}" class="view-article-btn" target="_blank" rel="noopener noreferrer">
            View Full Article
          </a>
          ${result.pmcid ? `<span class="pmc-badge">Available on PMC</span>` : ''}
        </div>
      </div>
    `;
  });
  
  resultsContainer.innerHTML = html;
  queryTimeDiv.textContent = `Found ${results.length} results in ${timeMs}ms`;
}

// Show error message
function showError(message) {
  resultsContainer.innerHTML = `
    <div class="error">
      <p><strong>Error:</strong> ${message}</p>
      <p style="margin-top: 10px; font-size: 14px;">
        Make sure the backend server is running at <code>http://localhost:3000</code>
      </p>
    </div>
  `;
  queryTimeDiv.textContent = '';
}

// Hide suggestions when clicking outside
document.addEventListener('click', (e) => {
  if (!searchInput.contains(e.target) && !suggestionsDiv.contains(e.target)) {
    hideSuggestions();
  }
});

// Test connection on page load
window.addEventListener('load', async () => {
  try {
    const response = await fetch('/api/health');
    const data = await response.json();
    console.log('Backend connected:', data.status);
    
    // Show welcome message
    if (!searchInput.value) {
      resultsContainer.innerHTML = `
        <div class="no-results" style="background: #e8f0fe; border-color: #1a73e8;">
          <p style="color: #1a73e8; font-weight: bold;">CORD-19 Search Engine Ready</p>
          <p>Search 1,000+ COVID-19 research papers</p>
          <p style="margin-top: 15px; font-size: 13px;">
            Try: <span style="color: #1a73e8; cursor: pointer;" onclick="searchExample('coronavirus')">coronavirus</span>, 
            <span style="color: #1a73e8; cursor: pointer;" onclick="searchExample('vaccine')">vaccine</span>, 
            <span style="color: #1a73e8; cursor: pointer;" onclick="searchExample('treatment')">treatment</span>
          </p>
        </div>
      `;
    }
  } catch (error) {
    console.warn('Backend not responding:', error);
    showError('Backend server not connected. Please start the server.');
  }
});

// Search example helper
function searchExample(term) {
  searchInput.value = term;
  performSearch();
}


