#include "lexicon.hpp"
#include "utf8cpp/utf8.h"
#include <cstring>

using namespace std;

namespace InvertedIndex {

std::vector<size_t> Lexicon::lexeme_offsets = std::vector<size_t>();

Lexicon::Lexicon(std::shared_ptr<ILogger> logger) 
  : logger_(logger), id_counter_(0) {
}

char Lexicon::GetValidCharacter(int c) {
  if (c >= 0x41 && c <= 0x5A) { // A-Z to a-z.
    return c + 0x20;
  } else if (c >= 0x61 && c <= 0x7A) { // a-z remains a-z.
    return c;
  } else if (c >= 0xC0 && c <= 0xFF) { // Accented characters to unaccented.
    static const char*
    //   "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ"
    tr = "aaaaaaeceeeeiiiidnoooooxouuuuypsaaaaaaeceeeeiiiionooooooouuuuypy";
    return tr[c - 0xC0];
  } else { // Non alphanumeric characters to null.
    return '\0';
  }
}

std::vector<std::string> Lexicon::ExtractLexemes(std::string text) {
  vector<std::string> result;
  
  std::string lexeme;
  std::string::iterator current_it = text.begin();
  lexeme_offsets.clear();
  
  int utf8char;
  while (current_it != text.end()) {
    utf8char = utf8::next(current_it, text.end());
    char c = GetValidCharacter(utf8char);
    if (c != '\0') {
      if (lexeme.size() == 0) lexeme_offsets.push_back(current_it - text.begin() - 1);
      lexeme += c;
    } else if (lexeme.length() > 0 && lexeme.length() < MAX_LEXEME_LENGTH) {
      result.push_back(lexeme);
      lexeme.clear();
    } else {
      lexeme.clear();
    }
  }
  
  if (lexeme.length() > 0 && lexeme.length() < MAX_LEXEME_LENGTH)
    result.push_back(lexeme);
  return result;
}

/**
 * Inserts a new lexeme in the lexicon or returns its id if it has already been inserted.
 *
 */
unsigned int Lexicon::AddLexeme(const string& lexeme) {
  unsigned int id;
  if ((id = GetLexemeId(lexeme)) == 0) {
    ++id_counter_;
    lexeme_map_.insert(pair<string, unsigned int>(lexeme, id_counter_));
    id_map_.insert(pair<unsigned int, Lexeme>(id_counter_, lexeme));
    return id_counter_;
  } else
    return id;
}

/**
 * Gets a lexeme id or -1 if it does not exist.
 *
 */
unsigned int Lexicon::GetLexemeId(const string &lexeme) {
  map<string, unsigned int>::iterator it;
  if ((it = lexeme_map_.find(lexeme)) == lexeme_map_.end())
    return 0;
  else
    return it->second;
}

Lexeme Lexicon::GetLexemeById(unsigned int id) {
  map<unsigned int, Lexeme>::iterator it;
  if ((it = id_map_.find(id)) == id_map_.end())
    return Lexeme();
  else
    return it->second;
}


size_t Lexicon::GetNumLexemes() {
  return lexeme_map_.size();
}

void Lexicon::Write(FILE* file, off_t offset) {
  fseeko(file, offset, SEEK_SET);
  for (size_t i = 1; i <= id_map_.size(); ++i) {
    Lexeme& lexeme = id_map_.find(i)->second;

    static char c = '\0';
    static char buffer[10000];
    size_t size = (lexeme.lexeme.size() <= 10000) ? lexeme.lexeme.size() : 0;
    std::strncpy(buffer, lexeme.lexeme.c_str(), size);
    fwrite(buffer, sizeof(char), size, file);
    fwrite(&c, sizeof(char), 1, file);

    size_t anchor_refs = lexeme.links.size();
    fwrite(&lexeme.offset, sizeof(off_t), 1, file);
    fwrite(&lexeme.anchor_offset, sizeof(off_t), 1, file);
    fwrite(&lexeme.doc_frequency, sizeof(size_t), 1, file);
    fwrite(&anchor_refs, sizeof(size_t), 1, file);
  }
}

void Lexicon::Load(FILE* file, off_t offset, size_t num_lexemes) {
  fseeko(file, offset, SEEK_SET);
  for (size_t i = 0; i < num_lexemes; ++i) {
    std::string lexeme;
    char c;
    while (fread(&c, sizeof(char), 1, file) > 0) {
      if (c == '\0') break;
      lexeme += c; 
    }
    size_t id = AddLexeme(lexeme); 

    size_t ret = 0;
    ret += fread(&id_map_[id].offset, sizeof(off_t), 1, file);
    ret += fread(&id_map_[id].anchor_offset, sizeof(off_t), 1, file);
    ret += fread(&id_map_[id].doc_frequency, sizeof(size_t), 1, file);
    ret += fread(&id_map_[id].anchor_refs, sizeof(size_t), 1, file);

    if (ret != 4) {
      throw new std::runtime_error("Error loading lexicon.");
    }
  }
}

void Lexicon::Print() {
  std::map<unsigned int, Lexeme>::iterator it = id_map_.begin();
  for (; it != id_map_.end(); ++it)
    std::cout << "Lexeme " << it->first << ": " << it->second.lexeme << std::endl;
}

} // End of namespace.
