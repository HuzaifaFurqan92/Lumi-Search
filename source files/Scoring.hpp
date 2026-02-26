#pragma once

#include <cmath>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <iostream>

// --- Type Definitions ---
// DocID -> Score (double)
using ScoreMap = std::unordered_map<int, double>;

// LexID -> Document Frequency (int)
using DFMap = std::unordered_map<int, int>; 

struct SearchResult {
    int docID;
    double score;
};
// ------------------------

/**
 * @brief Calculates the Inverse Document Frequency (IDF) for a term.
 * IDF = log(N / DF_t)
 * @param N Total number of documents in the dataset.
 * @param df Document Frequency (number of documents containing the term).
 * @return The IDF score.
 */
double calculateIDF(int N, int df) {
    if (df == 0) return 0.0;
    // Adding 1 to N and DF is a common smoothing technique (IDF = log((N+1)/(DF_t+1)))
    // but for simplicity here we use the basic form:
    return std::log(static_cast<double>(N) / df);
}

/**
 * @brief Scores the merged posting list using a simple TF-IDF-based approach.
 * Since the current postings map only holds the *sum* of term frequencies (Total TF) 
 * for all query terms, we use a simplified scoring: Score(D) = Total_TF * (Max IDF of query terms).
 * This provides frequency-based ranking while giving more weight to rare terms.
 * * @param postings The intersection (merged) posting list (DocID -> Total Term Frequency).
 * @param lexIDMap Map of word in query -> its LexID.
 * @param queryWords Vector of the original tokenized query words.
 * @param dfMap Global map of LexID -> Document Frequency (DF).
 * @param N Total number of documents in the collection.
 * @return A map of DocID to its calculated score.
 */
ScoreMap scoreResults(
    const std::unordered_map<int, int>& postings,
    const std::unordered_map<std::string, int>& lexIDMap,
    const std::vector<std::string>& queryWords,
    const DFMap& dfMap,
    int N)
{
    ScoreMap scores;
    
    // 1. Calculate the Max IDF for the current query
    double max_idf = 0.0;
    for (const std::string& word : queryWords) {
        auto lex_it = lexIDMap.find(word);
        if (lex_it == lexIDMap.end()) continue; 
        int lexID = lex_it->second;

        auto df_it = dfMap.find(lexID);
        int df = (df_it != dfMap.end()) ? df_it->second : 0;
        
        double current_idf = calculateIDF(N, df);
        if (current_idf > max_idf) {
            max_idf = current_idf;
        }
    }
    
    // 2. Score each document
    // Score = Total_TF * Max_IDF (as a basic frequency and rareness multiplier)
    for (const auto& pair : postings) {
        int docID = pair.first;
        int total_tf = pair.second;
        
        double score = static_cast<double>(total_tf) * max_idf; 
        
        // --- CUSTOM RANKING CRITERIA LAYER (Project Requirement #8) ---
        // Placeholder for a metadata/positional boost.
        // If DocID is less than 100, assume it's a high-priority document (e.g., matching a title field)
        if (docID < 100) { 
             score *= 1.25; // 25% Boost
        }
        // 
        
        scores[docID] = score;
    }
    
    return scores;
}

/**
 * @brief Sorts the documents based on their scores (descending) to generate the final rank.
 */
std::vector<SearchResult> rankResults(const ScoreMap& scores) {
    std::vector<SearchResult> ranked;
    for (const auto& pair : scores) {
        // Only include documents that have a score greater than zero (should be handled by intersection, but good practice)
        if (pair.second > 0) {
            ranked.push_back({pair.first, pair.second});
        }
    }

    // Sort by score (descending)
    std::sort(ranked.begin(), ranked.end(), 
        [](const SearchResult& a, const SearchResult& b) {
            return a.score > b.score; 
        });
        
    return ranked;
}