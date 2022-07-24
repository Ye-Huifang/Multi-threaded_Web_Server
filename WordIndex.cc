#include "./WordIndex.h"

namespace searchserver {

WordIndex::WordIndex() {
  wordMap = unordered_map<string, unordered_map<string, int>>();
}

 // Returns the number of unique words recorded in the index
size_t WordIndex::num_words() {
  return wordMap.size();
}
 
 // Record an occurrence of a document having the specified word show up in it
  // 
  // Arguments:
  //  - word: the word found in the specified document
  //  - doc_name: the name of the document the word occurrence showed up in
  //
  // Returns: None
void WordIndex::record(const string& word, const string& doc_name) {

  if(wordMap.find(word) != wordMap.end()) {
    if(wordMap[word].find(doc_name) != wordMap[word].end()) {
      wordMap[word][doc_name]++;
    } else {
      wordMap[word][doc_name] = 1;
    }
  } else {
    unordered_map<string, int> newMap;
    newMap[doc_name] = 1;
    wordMap[word] = newMap;
  }
}

 // Lookup a word in the index, getting a sorted list of all documents that contain
  // the word and a rank which is the number of occurrences of that word in the document
  //
  // Arguments:
  //  - word: a word we are looking up results for
  //
  // Returns:
  //  - A list of results. Each result contains a document name and the number
  //    of recorded occurances of the specified word in that document. The list is
  //    sorted with documents with the highest rank at the front.
list<Result> WordIndex::lookup_word(const string& word) {
  list<Result> result;

  // return empty list immediately if not found
  if (wordMap.find(word) == wordMap.end()) {
    return result;
  } 
  
  unordered_map<string, int> resultMap = wordMap.at(word);
  for(auto it = resultMap.begin(); it != resultMap.end(); ++it) {
    result.push_back(Result(it->first, it->second));
  }

  result.sort(); 
  
  return result;
}

 // Lookup a query (multiple words) in the index, getting a sorted list of all documents
  // that contain each word in the query and a rank which is the number of occurences
  // of each word in the document.
  //
  // Arguments:
  //  - word: a word we are looking up results for
  //
  // Returns:
  //  - A list of results. Each result contains a document name and the sum of the
  //    number of recorded occurences of the each query word in that document. The list is
  //    sorted with documents with the highest rank at the front.
list<Result> WordIndex::lookup_query(const vector<string>& query) {
  list<Result> results;

  if(query.empty()) {
    return results;
  }
  // Run query on first query word 
  list<Result> firstQuery = lookup_word(query[0]);

  if (firstQuery.size() == 0) {
    return results;
  }

  if(query.size() == 1){
    return firstQuery;
  }

unordered_map<string, int> map;
      // accumulate all queries into map
      for(Result r :firstQuery) {
        map[r.doc_name] = r.rank;
    }
firstQuery.clear();

 // for queries greater than 1 
    uint16_t i;

    for(i = 1; i < query.size(); i++){
      list<Result> subResult = lookup_word(query[i]);

      // return empty list if query word is not contained in any docs
      if(subResult.size() == 0){
        results.clear();
        return results;
      }

      std::unordered_map<string, int> subResultMap;
      for(Result r : subResult) {
          subResultMap[r.doc_name] = r.rank;
      }

      // remove all docs from map that don't contain string
      for(auto it = map.begin(); it != map.end(); ++it) {
        string queryWord = it->first;
        if(subResultMap.find(queryWord) == subResultMap.end()) {
          map.at(queryWord) = -1;
        } else {
          if(map.at(queryWord) != -1) {
            map.at(queryWord) += subResultMap.at(queryWord);
          }
        }
      }

      subResult.clear();
      subResultMap.clear();
    }
  
  // Convert map values into list
  for(auto it = map.begin(); it != map.end(); ++it) {
    if(it-> second != -1) {
      results.push_back(Result(it->first, it->second));
    }
  }
  map.clear();
  results.sort();
  return results;
}


}  // namespace searchserver
