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
