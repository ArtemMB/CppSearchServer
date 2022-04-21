#pragma once

#include <iostream>
#include <vector>

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

struct Document {
    int id{0};
    double relevance{0.0};
    int rating{0};
    
    Document() = default;
    Document(const int _id, const double _relevance, const int _rating);     
};

std::ostream& operator<<(std::ostream& os, const Document& doc);

void PrintDocument(const Document& document); 

void PrintMatchDocumentResult(
        int document_id, 
        const std::vector<std::string_view>& words, 
        DocumentStatus status);
