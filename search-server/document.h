#ifndef DOCUMENT_H
#define DOCUMENT_H

#pragma once

#include <iostream>

struct Document {
    int id{0};
    double relevance{0.0};
    int rating{0};
    
    Document() = default;
    Document(const int _id, const double _relevance, const int _rating);
};

std::ostream& operator<<(std::ostream& os, const Document& doc);

#endif // DOCUMENT_H
