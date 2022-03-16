#pragma once

#include <vector>
#include <string>

#include "document.h"

class SearchServer;

void RemoveDuplicates(SearchServer& search_server);

void AddDocument(SearchServer& search_server, int document_id,
                 const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings); 

void FindTopDocuments(const SearchServer& search_server, 
                      const std::string& raw_query);

void MatchDocuments(const SearchServer& search_server, 
                    const std::string& query); 

void PrintDocument(const Document& document); 

void PrintMatchDocumentResult(int document_id, 
                              const std::vector<std::string>& words, 
                              DocumentStatus status);

