#include <utility>
 

#include "document.h"

using namespace std;

Document::Document(const int _id, const double _relevance, const int _rating):
    id{_id},
    relevance{_relevance},
    rating{_rating}
{        
}


ostream& operator<<(ostream& os, const Document& doc)
{
    os<<"{";
    os<<" document_id = "<<doc.id;
    os<<", relevance = "<<doc.relevance;
    os<<", rating = "<<doc.rating;
    os<<" }";
    return os;
}

void PrintDocument(const Document& document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

void PrintMatchDocumentResult(
        int document_id, 
        const vector<std::string_view>& words,
        DocumentStatus status)
{
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string_view& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}
