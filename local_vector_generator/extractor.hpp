#ifndef __EXTRACTOR_HPP__
#define __EXTRACTOR_HPP__

#include "gumbo.h"
#include "logger.hpp"
#include "lexicon.hpp"
#include "doc_map.hpp"
#include "doc_collection.hpp"
#include "tuple_sorter.hpp"
#include <memory>
#include <stdio.h>
#include <string>
#include <vector>

namespace InvertedIndex {

class Extractor {
  std::shared_ptr<ILogger> logger_;
  std::shared_ptr<ILexicon> lexicon_;
  std::shared_ptr<DocMap> doc_map_;
  std::shared_ptr<ITupleSorter> tuple_sorter_;
  std::shared_ptr<IDocCollection> doc_collection_;
  std::vector<std::string> lexemes_;

  FILE* input_file_ = NULL;
  FILE* output_file_ = NULL;
  unsigned int current_doc_id_ = 0;
  Tuple tuple_block_[MAX_TUPLES];
  std::string doc_text_;

  std::string GetCleanText(GumboNode* node);
  void Parse(GumboNode* node, bool get_links = false);
  void ExtractFromDoc(int, const std::string&);
  void ExtractLexemes(std::string);
  char GetValidCharacter(int);
  std::string TruncateUrl(std::string);
  std::string GetRootUrl(std::string);
  std::string NormalizeHyperlink(unsigned int, std::string&);
  bool IsStopWord(const std::string&);

 public:
  Extractor(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<ILexicon> lexicon,
    std::shared_ptr<DocMap> doc_map,
    std::shared_ptr<ITupleSorter> tuple_sorter,
    std::shared_ptr<IDocCollection> doc_collection
  );
 
  void ExtractLinks(RawDocument&);
  void ExtractFromDoc(RawDocument&);
  void Extract(const std::string&, const std::string&);
  void PrintLexemes(RawDocument&);
  void Parse(const std::string&, bool get_links = false);
  void ReadDoc(unsigned int);
  std::string GetTitle(unsigned int);
  std::string GetShortTextAt(unsigned int, unsigned int);
  std::string GetRawText(unsigned int);
};

} // End of namespace.

#endif
