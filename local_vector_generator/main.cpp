#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include "utf8cpp/utf8.h"
#include <cstring>
#include <vector>

#include "gumbo.h"

using namespace std;

char GetValidCharacter(int c) {
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

std::vector<std::string> ExtractLexemes(std::string text) {
  vector<std::string> result;
  
  std::string lexeme;
  std::string::iterator current_it = text.begin();
  
  int utf8char;
  while (current_it != text.end()) {
    utf8char = utf8::next(current_it, text.end());
    char c = GetValidCharacter(utf8char);
    if (c != '\0') {
      lexeme += c;
    } else if (lexeme.length() > 0 && lexeme.length() < 30) {
      result.push_back(lexeme);
      lexeme.clear();
    } else {
      lexeme.clear();
    }
  }
  
  if (lexeme.length() > 0 && lexeme.length() < 30)
    result.push_back(lexeme);
  return result;
}

static std::string cleantext(GumboNode* node) {
  if (node->type == GUMBO_NODE_TEXT) {
    return std::string(node->v.text.text);
  } else if (node->type == GUMBO_NODE_ELEMENT &&
             node->v.element.tag != GUMBO_TAG_SCRIPT &&
             node->v.element.tag != GUMBO_TAG_STYLE) {
    std::string contents = "";
    GumboVector* children = &node->v.element.children;
    for (unsigned int i = 0; i < children->length; ++i) {
      const std::string text = cleantext((GumboNode*) children->data[i]);
      if (i != 0 && !text.empty()) {
        contents.append(" ");
      }
      contents.append(text);
    }
    return contents;
  } else {
    return "";
  }
}

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "Usage: clean_text <html filename>\n";
    exit(EXIT_FAILURE);
  }
  const char* filename = argv[1];

  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in) {
    std::cout << "File " << filename << " not found!\n";
    exit(EXIT_FAILURE);
  }

  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  contents = contents.substr(contents.find('<'));

  GumboOutput* output = gumbo_parse(contents.c_str());
  std::vector<string> lexemes = ExtractLexemes(cleantext(output->root));
  for (auto lexeme : lexemes) std::cout << lexeme << " ";
  gumbo_destroy_output(&kGumboDefaultOptions, output);
}
