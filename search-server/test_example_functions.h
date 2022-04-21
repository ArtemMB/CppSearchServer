#pragma once

#include "request_queue.h"
#include "processqueries.h"


void AddDocument(SearchServer& search_server, int document_id,
                 const std::string& document, DocumentStatus status,
                 const std::vector<int>& ratings); 

void FindTopDocuments(const SearchServer& search_server, 
                      const std::string& raw_query);

void MatchDocuments(const SearchServer& search_server, 
                    const std::string& query); 

void TestRemoveDuplication();

void TestParalelQuery();

void TestProcessQueriesJoined();

void TestRemoveDocument();

void TestMatchDocument();


void TestFindTopDocuments();
