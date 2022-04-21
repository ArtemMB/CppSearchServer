#include <algorithm>
#include <cmath>
#include <utility>

#include "search_server.h"

//#include "logduration.h"

using namespace std;
    

SearchServer::SearchServer(const string_view stop_words_text):
    SearchServer{SplitIntoWords(stop_words_text)}
{    
}

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer(string_view(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, 
                               const string_view document, 
                               DocumentStatus status, 
                               const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument{"document_id is invalid"};
    }
    
    vector<string> words = SplitIntoWordsNoStop(document);   
        
    const double inv_word_count = 1.0 / words.size();
    map<string_view, double> wordFrequencies;
    
    for (const string& word : words) {
        const auto& w = unique_words_.insert(word);
        word_to_document_freqs_[*w.first][document_id] += inv_word_count;
        wordFrequencies[*w.first] = word_to_document_freqs_[*w.first][document_id];
    }
    
    documents_.emplace(document_id, 
                       DocumentData{ComputeAverageRating(ratings), 
                                    status});
    
    document_to_word_freqs_.emplace(document_id, wordFrequencies);
    
    document_ids_.insert(document_id);        
}

vector<Document> SearchServer::FindTopDocuments(
        const string_view raw_query, 
        DocumentStatus status) const {    
    return FindTopDocuments(
                raw_query,
                [status](int , 
                DocumentStatus document_status, int ) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(
        const execution::sequenced_policy& , 
        const string_view raw_query, 
        DocumentStatus status) const
{
    return FindTopDocuments(raw_query, status);
}

vector<Document> SearchServer::FindTopDocuments(
        const execution::parallel_policy& policy, 
        const string_view raw_query, 
        DocumentStatus status) const
{    
    return FindTopDocuments(policy,
                raw_query,
                [status](int , 
                DocumentStatus document_status, int ) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments(
        const execution::sequenced_policy& policy, 
        const string_view raw_query) const
{
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(
        const execution::parallel_policy& policy, 
        const string_view raw_query) const
{
    return FindTopDocuments(execution::par, raw_query, DocumentStatus::ACTUAL);
}



vector<Document> SearchServer::FindTopDocuments(
        const string_view raw_query) const {
    return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
        const string_view raw_query, int document_id) const { 
    
    if(0 == document_ids_.count(document_id))
    {
        throw out_of_range{"Document id in not exsist: " 
                                + to_string(document_id)};
    }
    
    Query query{ParseQuery(raw_query)};        
    
    const map<string_view, double>& words_freqs{
        document_to_word_freqs_.at(document_id)};
    
    for(const auto& [word, freq]: words_freqs)
    {
        if (find(query.minus_words.begin(), 
                 query.minus_words.end(), word) != query.minus_words.end()) {
            return {vector<string_view>{},
                documents_.at(document_id).status};  
        }
    }    
    
    vector<string_view> matched_words; 
    for(const auto& [word, freq]: words_freqs)
    {
        if (find(query.plus_words.begin(), 
                 query.plus_words.end(), word) != query.plus_words.end()) {
            matched_words.push_back(word);
        }
    }
    
    return {matched_words, documents_.at(document_id).status};        
}

tuple<vector<string_view>, DocumentStatus> 
    SearchServer::MatchDocument(
        const execution::parallel_policy& policy,
        const string_view raw_query, 
        int document_id) const
{    
    if(0 == document_ids_.count(document_id))
    {
        throw out_of_range{"Document id in not exsist: " 
                                + to_string(document_id)};
    }
    
    const map<string_view, double>& words_freqs{
        document_to_word_freqs_.at(document_id)};
    if(words_freqs.empty())
    {
        return {vector<string_view>{},
            documents_.at(document_id).status};
    }    
     
    QueryView query; 
    ParseQuery(policy, raw_query, query);
    
    auto checker = [&](const string_view& word)
    {
        return words_freqs.find(word) !=  words_freqs.end();
    };
    
    bool is_minus{any_of(
                    policy, 
                    query.minus_words.begin(), query.minus_words.end(), 
                    checker)};
    
    if (is_minus) {
        return {vector<string_view>{}, 
            documents_.at(document_id).status};
    }   
    
    vector<string_view> matched_words(query.plus_words.size());
    
    
    auto words_end = copy_if(policy, 
             query.plus_words.begin(), query.plus_words.end(), 
             matched_words.begin(),
             checker);        
    
    sort(policy, matched_words.begin(), words_end);
    words_end = unique(policy, matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());
    
    return {matched_words,
        documents_.at(document_id).status};
}

tuple<vector<string_view>, DocumentStatus> 
    SearchServer::MatchDocument(
        const execution::sequenced_policy& policy,
        const string_view raw_query, 
        int document_id) const
{
    return MatchDocument(raw_query, document_id);
}

void SearchServer::RemoveDocument(int document_id)
{    
    if(0 == document_ids_.count(document_id))
    {
        return;
    }
    
    const map<string_view, double>& words_freqs{
        document_to_word_freqs_.at(document_id)};
    
    for(auto&[word, freq]: words_freqs)
    {
        word_to_document_freqs_[word].erase(document_id);  
        
        if(word_to_document_freqs_[word].empty())
        {
            word_to_document_freqs_.erase(word);//удаляем, т.к. слово больше не нужно
        }
    }
    
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(
        const execution::parallel_policy& policy, 
        int document_id)
{
    const map<string_view, double>& words_freqs{
        document_to_word_freqs_.at(document_id)};
    if(!words_freqs.empty())
    {
        vector<string_view> words{words_freqs.size()};
        
        transform(policy, 
                  words_freqs.begin(), words_freqs.end(), 
                  words.begin(),
                  [](const auto& wf)
        {
            return wf.first;
        });
        
        
        auto trans = [this, document_id](const string_view& item)
        {
            word_to_document_freqs_[item].erase(document_id);             
        };    
        
        for_each(policy, words.begin(), words.end(), trans);    
    }
    
    documents_.erase(document_id);
    document_ids_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(
        const execution::sequenced_policy& policy,
        int document_id)
{
    RemoveDocument(document_id);
}

const map<string_view, double>& SearchServer::GetWordFrequencies(
        int document_id) const
{    
    static map<string_view, double> empty_word_frequencies;
    
    auto it = documents_.find(document_id);
    if(it != documents_.end())
    {        
        return document_to_word_freqs_.at(document_id);
    }
    
    return empty_word_frequencies;    
}

set<int>::iterator SearchServer::begin()
{
    return document_ids_.begin();
}

set<int>::iterator SearchServer::end()
{
    return document_ids_.end();
}

set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.cbegin();
}

set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.cend();
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string> SearchServer::SplitIntoWordsNoStop(
        const string_view text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument{"Contains invalid characters"};
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }        
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string text) const {     
    if (text.empty()) {
        throw invalid_argument{text + " is empty"};
    }
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
        throw invalid_argument{text + " is invalid"};
    }
    
    return QueryWord{text, is_minus, IsStopWord(text)};
}

SearchServer::QueryWordView SearchServer::ParseQueryWord(
        string_view text) const
{
    //LOG_DURATION(string{"ParseQueryWord"});
    if (text.empty()) {
        throw invalid_argument{string{text} + " is empty"};
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument{string{text} + " is invalid"};
    }
    
    return QueryWordView{word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(
        const string_view& text) const {     
    Query result;
    auto words{SplitIntoWordsView(text)};
    for (const string_view& word : words) {
        QueryWordView query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }    
    
    return result;
}

vector<string_view> SortUniq(const execution::parallel_policy& policy,
                             vector<string_view>& container) {

    sort(policy, container.begin(), container.end());
    auto words_end = unique(policy, container.begin(), container.end());
    container.erase(words_end, container.end());

    return container;
}

void SearchServer::ParseQuery(
        const execution::parallel_policy& policy,
        const string_view& text, QueryView& out) const
{
    auto words{SplitIntoWordsView(text)};
    
    for (const string_view& word : words) {
        QueryWordView query_word = ParseQueryWord(word);
        
        if (!query_word.is_stop) {
            if (query_word.is_minus) {                
                out.minus_words.push_back(query_word.data);                         
            } else {                
                out.plus_words.push_back(query_word.data);                
            }
        }
    }    
    out.minus_words = SortUniq(policy, out.minus_words);
    out.plus_words = SortUniq(policy, out.plus_words);
}

vector<string_view> SortUniq(const execution::sequenced_policy& policy,
                             vector<string_view>& container) {

    sort(policy, container.begin(), container.end());
    auto words_end = unique(policy, container.begin(), container.end());
    container.erase(words_end, container.end());

    return container;
}

void SearchServer::ParseQuery(
        const execution::sequenced_policy& policy, 
        const string_view& text, 
        QueryView& out) const
{
    auto words{SplitIntoWordsView(text)};    
    //код для сдачи
    for (const string_view& word : words) {
        QueryWordView query_word = ParseQueryWord(word);
        
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                out.minus_words.push_back(query_word.data);
            } else {
                out.plus_words.push_back(query_word.data);
            }
        }
    } 
    
    out.minus_words = SortUniq(policy, out.minus_words);
    out.plus_words = SortUniq(policy, out.plus_words);
}

double SearchServer::ComputeWordInverseDocumentFreq(
        const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}




