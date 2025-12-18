// DOM Elements
const searchInput = document.getElementById('searchInput');
const searchButton = document.getElementById('searchButton');
const suggestionsDiv = document.getElementById('suggestions');
const resultsContainer = document.getElementById('resultsContainer');
const queryTimeDiv = document.getElementById('queryTime');
const resultCountDiv = document.getElementById('resultCount');
const statusIcon = document.getElementById('statusIcon');
const statusText = document.getElementById('statusText');
const welcomeMessage = document.getElementById('welcomeMessage');
const quickButtons = document.querySelectorAll('.quick-btn');

// API Base URL
const API_BASE = window.location.origin;

// Debounce timer for suggestions
let debounceTimer;

// Initialize
document.addEventListener('DOMContentLoaded', () => {
    checkBackendStatus();
    setupEventListeners();
    showWelcomeMessage();
});

// Setup event listeners
function setupEventListeners() {
    // Search input with debouncing
    searchInput.addEventListener('input', handleSearchInput);
    
    // Enter key search
    searchInput.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') {
            performSearch();
        }
    });
    
    // Search button
    searchButton.addEventListener('click', performSearch);
    
    // Quick search buttons
    quickButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            searchInput.value = btn.dataset.query;
            performSearch();
        });
    });
    
    // Hide suggestions when clicking outside
    document.addEventListener('click', (e) => {
        if (!searchInput.contains(e.target) && !suggestionsDiv.contains(e.target)) {
            hideSuggestions();
        }
    });
}

// Handle search input with debouncing
function handleSearchInput() {
    clearTimeout(debounceTimer);
    const query = searchInput.value.trim();
    
    if (query.length < 2) {
        hideSuggestions();
        return;
    }
    
    debounceTimer = setTimeout(() => {
        fetchSuggestions(query);
    }, 300);
}

// Check backend status
async function checkBackendStatus() {
    try {
        const response = await fetch(`${API_BASE}/api/health`);
        const data = await response.json();
        
        if (data.success) {
            statusIcon.style.color = '#2ecc71';
            statusText.textContent = 'Backend Online';
            statusIcon.className = 'fas fa-circle';
        } else {
            statusIcon.style.color = '#f39c12';
            statusText.textContent = 'Backend Warning';
            statusIcon.className = 'fas fa-exclamation-circle';
        }
    } catch (error) {
        statusIcon.style.color = '#e74c3c';
        statusText.textContent = 'Backend Offline';
        statusIcon.className = 'fas fa-times-circle';
        console.warn('Backend not responding:', error);
    }
}

// Fetch search suggestions
async function fetchSuggestions(query) {
    try {
        const response = await fetch(`${API_BASE}/api/suggest?q=${encodeURIComponent(query)}`);
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
    
    suggestions.forEach((suggestion, index) => {
        const div = document.createElement('div');
        div.className = 'suggestion-item';
        div.innerHTML = `
            <i class="fas fa-search"></i>
            <span>${suggestion}</span>
        `;
        
        div.onclick = () => {
            searchInput.value = suggestion;
            hideSuggestions();
            performSearch();
        };
        
        // Highlight matching part
        const suggestionLower = suggestion.toLowerCase();
        const queryLower = searchInput.value.toLowerCase();
        const matchIndex = suggestionLower.indexOf(queryLower);
        
        if (matchIndex !== -1) {
            const before = suggestion.substring(0, matchIndex);
            const match = suggestion.substring(matchIndex, matchIndex + queryLower.length);
            const after = suggestion.substring(matchIndex + queryLower.length);
            
            div.innerHTML = `
                <i class="fas fa-search"></i>
                <span>${before}<strong>${match}</strong>${after}</span>
            `;
        }
        
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
        showMessage('Please enter a search query', 'error');
        return;
    }
    
    hideSuggestions();
    showLoading(query);
    
    try {
        const startTime = Date.now();
        const response = await fetch(`${API_BASE}/api/search?q=${encodeURIComponent(query)}`);
        const data = await response.json();
        const timeMs = Date.now() - startTime;
        
        if (data.success) {
            displayResults(data.results, query, timeMs, data.count);
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
            <div class="loading-icon">
                <i class="fas fa-spinner"></i>
            </div>
            <h2>Searching for "${query}"</h2>
            <p>Querying CORD-19 research database...</p>
            <p style="color: #7f8c8d; margin-top: 10px;">
                <i class="fas fa-microscope"></i> Analyzing COVID-19 research papers
            </p>
        </div>
    `;
    
    queryTimeDiv.textContent = '';
    resultCountDiv.textContent = '';
}

// Display search results
function displayResults(results, query, timeMs, totalCount) {
    if (!results || results.length === 0) {
        resultsContainer.innerHTML = `
            <div class="no-results">
                <div class="no-results-icon">
                    <i class="fas fa-search"></i>
                </div>
                <h2>No results found for "${query}"</h2>
                <p>Try different keywords or broader search terms</p>
                <div style="margin-top: 30px;">
                    <button class="quick-btn" onclick="searchInput.value='coronavirus'; performSearch();">
                        Try "coronavirus"
                    </button>
                    <button class="quick-btn" onclick="searchInput.value='vaccine'; performSearch();">
                        Try "vaccine"
                    </button>
                    <button class="quick-btn" onclick="searchInput.value='treatment'; performSearch();">
                        Try "treatment"
                    </button>
                </div>
            </div>
        `;
        updateStats(timeMs, 0);
        return;
    }
    
    let html = '';
    
    // Show exactly 10 results
    const displayResults = results.slice(0, 10);
    
    displayResults.forEach((result, index) => {
        // Build PMC URL
        let pmcUrl = '#';
        let displayUrl = 'Link not available';
        
        if (result.pmc_url && result.pmc_url.trim() !== '') {
            pmcUrl = result.pmc_url;
            const urlObj = new URL(pmcUrl);
            displayUrl = urlObj.hostname + urlObj.pathname;
        } else if (result.pmcid && result.pmcid.trim() !== '') {
            let pmcid = result.pmcid.trim();
            if (!pmcid.toUpperCase().startsWith('PMC')) {
                pmcid = 'PMC' + pmcid;
            }
            pmcUrl = `https://pmc.ncbi.nlm.nih.gov/articles/${pmcid}/`;
            displayUrl = `pmc.ncbi.nlm.nih.gov/articles/${pmcid}`;
        }
        
        // Get title
        const title = result.title || `Research Document ${result.docId || index + 1}`;
        
        // Get snippet/abstract
        let snippet = result.snippet || result.abstract || 
                     'This document contains relevant research on COVID-19. Click to view full paper.';
        
        // Highlight search terms in snippet
        const queryWords = query.toLowerCase().split(' ');
        let highlightedSnippet = snippet;
        queryWords.forEach(word => {
            if (word.length > 2) {
                const regex = new RegExp(`(${word})`, 'gi');
                highlightedSnippet = highlightedSnippet.replace(regex, '<mark>$1</mark>');
            }
        });
        
        // Shorten snippet
        if (highlightedSnippet.length > 250) {
            highlightedSnippet = highlightedSnippet.substring(0, 247) + '...';
        }
        
        // Build result HTML
        html += `
            <div class="result-item" onclick="window.open('${pmcUrl}', '_blank')">
                <div class="result-meta">
                    <div class="result-number">${index + 1}</div>
                    ${result.score ? `<div class="result-score">Score: ${result.score.toFixed(4)}</div>` : ''}
                    ${result.docId ? `<div class="result-id">Doc ID: ${result.docId}</div>` : ''}
                </div>
                
                <a href="${pmcUrl}" class="result-title" target="_blank" rel="noopener noreferrer">
                    ${title}
                </a>
                
                <a href="${pmcUrl}" class="result-url" target="_blank" rel="noopener noreferrer">
                    ${displayUrl}
                </a>
                
                <div class="result-snippet">
                    ${highlightedSnippet}
                </div>
                
                <div class="result-details">
                    ${result.pmcid ? `<div class="pmc-badge">PMC ID: ${result.pmcid}</div>` : ''}
                    <button class="view-btn" onclick="event.stopPropagation(); window.open('${pmcUrl}', '_blank')">
                        <i class="fas fa-external-link-alt"></i> View Paper
                    </button>
                </div>
            </div>
        `;
    });
    
    resultsContainer.innerHTML = html;
    updateStats(timeMs, displayResults.length, totalCount);
}

// Update stats bar
function updateStats(timeMs, displayedCount, totalCount) {
    queryTimeDiv.textContent = `Search completed in ${timeMs}ms`;
    resultCountDiv.textContent = `Showing ${displayedCount} results`;
    
    if (totalCount && totalCount > displayedCount) {
        resultCountDiv.textContent += ` (of ${totalCount} total)`;
    }
}

// Show welcome message
function showWelcomeMessage() {
    welcomeMessage.style.display = 'block';
}

// Hide welcome message
function hideWelcomeMessage() {
    welcomeMessage.style.display = 'none';
}

// Show error message
function showError(message) {
    resultsContainer.innerHTML = `
        <div class="error">
            <div class="error-icon">
                <i class="fas fa-exclamation-triangle"></i>
            </div>
            <h2>Search Error</h2>
            <p>${message}</p>
            <p style="margin-top: 20px; font-size: 0.9rem; color: #7f8c8d;">
                Make sure the C++ backend is compiled and running properly.
            </p>
        </div>
    `;
    
    queryTimeDiv.textContent = '';
    resultCountDiv.textContent = '';
}

// Show custom message
function showMessage(message, type = 'info') {
    const icon = type === 'error' ? 'fa-exclamation-circle' : 
                type === 'warning' ? 'fa-exclamation-triangle' : 'fa-info-circle';
    
    resultsContainer.innerHTML = `
        <div class="${type === 'error' ? 'error' : 'no-results'}">
            <div class="${type === 'error' ? 'error-icon' : 'no-results-icon'}">
                <i class="fas ${icon}"></i>
            </div>
            <h2>${message}</h2>
        </div>
    `;
}

// Make functions globally available for onclick handlers
window.performSearch = performSearch;


