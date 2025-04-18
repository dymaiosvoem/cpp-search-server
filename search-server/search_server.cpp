#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) { 
        if (document_id < 0) { 
            throw std::invalid_argument("айди документа отрицательный"s); 
        } 
        if (documents_.count(document_id)) { 
            throw std::invalid_argument("повторяющийся документ"s); 
        } 
        if (!IsValidWord(document)) { 
            throw std::invalid_argument("недопустимые символы"s); 
        } 
 
        const std::vector<std::string> words = SplitIntoWordsNoStop(document); 
        const double inv_word_count = 1.0 / words.size(); 
        for (auto& word : words) { 
            word_to_document_freqs_[word][document_id] += inv_word_count; 
        } 
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status}); 
        documents_index_.push_back(document_id); 
    } 

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const { 
        return FindTopDocuments(raw_query, 
            [&status](int document_id, DocumentStatus new_status, int rating) { 
                return new_status == status; 
            }); 
    } 

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const { 
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL); 
    } 

int SearchServer::GetDocumentCount() const { 
        return static_cast<int>(documents_.size()); 
    } 
 
int SearchServer::GetDocumentId(int index) const { 
            return documents_index_.at(index); 
    } 

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const { 
        Query query = ParseQuery(raw_query); 
        std::vector<std::string> matched_words; 
        for (const std::string& word : query.plus_words) { 
            if (word_to_document_freqs_.count(word) == 0) { 
                continue; 
            } 
            if (word_to_document_freqs_.at(word).count(document_id)) { 
                matched_words.push_back(word); 
            } 
        } 
        for (const std::string& word : query.minus_words) { 
            if (word_to_document_freqs_.count(word) == 0) { 
                continue; 
            } 
            if (word_to_document_freqs_.at(word).count(document_id)) { 
                matched_words.clear(); 
                break; 
            } 
        } 
        return {matched_words, documents_.at(document_id).status}; 
    } 

bool SearchServer::IsStopWord(const std::string& word) const { 
        return stop_words_.count(word) > 0; 
    } 
 
    bool SearchServer::IsValidWord(const std::string& word) { 
        return none_of(word.begin(), word.end(), [](char c) { 
            return c >= '\0' && c < ' '; 
            }); 
    } 

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const { 
        std::vector<std::string> words; 
        for (const string& word : SplitIntoWords(text)) { 
            if (!IsStopWord(word)) { 
                words.push_back(word); 
            } 
        } 
        return words; 
    } 
 
    int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) { 
        if (ratings.empty()) { 
            return 0; 
        } 
        int rating_sum = accumulate(ratings.begin(), ratings.end(), 0); 
        return rating_sum / static_cast<int>(ratings.size()); 
    } 

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const { 
        QueryWord result; 
        if (text.empty()) { 
            throw std::invalid_argument("нет текста после минуса"s); 
        } 
        bool is_minus = false; 
        if (text[0] == '-') { 
            is_minus = true; 
            text = text.substr(1); 
        } 
        if (text.empty() || text[0] == '-' || !IsValidWord(text)) { 
            throw std::invalid_argument("один или более минусов"s); 
        } 
        return {text, is_minus, IsStopWord(text)}; 
    } 

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const { 
        Query result; 
        for (const std::string& word : SplitIntoWords(text)) { 
            QueryWord query_word = ParseQueryWord(word); 
            if (!query_word.is_stop) { 
                if (query_word.is_minus) { 
                    result.minus_words.insert(query_word.data); 
                } 
                else { 
                    result.plus_words.insert(query_word.data); 
                } 
            } 
        } 
        return result; 
    } 

void AddDocument(SearchServer& search_server, int document_id, const std::string& document,
                 DocumentStatus status, const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "Error in adding document "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "Results for request: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "Error is seaching: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "Matching for request: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "Error in matchig request "s << query << ": "s << e.what() << std::endl;
    }
}