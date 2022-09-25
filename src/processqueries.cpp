#include <algorithm>
#include <execution>

#include "processqueries.h"


using namespace std;

std::vector<std::vector<Document> > ProcessQueries(
        const SearchServer& search_server, 
        const std::vector<std::string>& queries)
{
    vector<vector<Document> > out;
    out.resize(queries.size());
    
    transform (execution::par,
                queries.begin(), queries.end(), out.begin(), 
               [&search_server](const string& querie){
        
        vector<Document> docs{search_server.FindTopDocuments(querie)};
        return docs;  
    });    
    
    return out;
}

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, 
                                         const std::vector<std::string>& queries)
{    
    return transform_reduce(
                execution::par,
                queries.begin(), queries.end(), 
                list<Document>{}, 
                [](const list<Document>& accum, const list<Document>& data){
                    list<Document> out{accum.begin(), accum.end()};
                    out.insert(out.end(), data.begin(), data.end());
                    return out;
                    },
                [&search_server](const string& querie){ 
                    vector<Document> docs{search_server.FindTopDocuments(querie)};
                        return list<Document>{docs.begin(), docs.end()}; 
                    } 
            );
}
