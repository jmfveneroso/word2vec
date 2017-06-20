#include "extractor.hpp"
#include "utf8cpp/utf8.h"
#include <algorithm>
#include <sstream>

namespace InvertedIndex {

Extractor::Extractor(
  std::shared_ptr<ILogger> logger,
  std::shared_ptr<ILexicon> lexicon,
  std::shared_ptr<DocMap> doc_map,
  std::shared_ptr<ITupleSorter> tuple_sorter,
  std::shared_ptr<IDocCollection> doc_collection
) : logger_(logger), lexicon_(lexicon), 
    doc_map_(doc_map), tuple_sorter_(tuple_sorter), doc_collection_(doc_collection) {
}

std::string Extractor::GetCleanText(GumboNode* node) {
  if (node->type == GUMBO_NODE_TEXT) return std::string(node->v.text.text);

  if (node->type != GUMBO_NODE_ELEMENT || node->v.element.tag == GUMBO_TAG_SCRIPT ||
    node->v.element.tag == GUMBO_TAG_STYLE) return "";

  std::string contents = "";
  GumboVector* children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    const std::string text = GetCleanText((GumboNode*) children->data[i]);
    if (i != 0 && !text.empty()) {
      contents.append(" ");
    }
    contents.append(text);
  }
  return contents;
}

bool Extractor::IsStopWord(const std::string& lexeme){
  static std::set<std::string> stop_words = {
    "de", "a", "o", "que", "e", "do", "da", "em", "um", "para", "e", "com", 
    "nao", "uma", "os", "no", "se", "na", "por", "mais", "as", "dos", "como", 
    "mas", "foi", "ao", "ele", "das", "tem", "seu", "sua", "ou", "ser", "quando", 
    "muito", "ha", "nos", "ja", "esta", "eu", "tambem", "so", "pelo", "pela", "ate", 
    "isso", "ela", "entre", "era", "depois", "sem", "mesmo", "aos", "ter", "seus", 
    "quem", "nas", "me", "esse", "eles", "estao", "voce", "tinha", "foram", "essa", 
    "num", "nem", "suas", "meu", "às", "minha", "tem", "numa", "pelos", "elas", 
    "havia", "seja", "qual", "sera", "nos", "tenho", "lhe", "deles", "essas", "esses", 
    "pelas", "este", "fosse", "dele", "tu", "te", "voces", "vos", "lhes", "meus", "minhas", 
    "teu", "tua", "teus", "tuas", "nosso", "nossa", "nossos", "nossas", "dela", "delas", "esta", 
    "estes", "estas", "aquele", "aquela", "aqueles", "aquelas", "isto", "aquilo", "estou", 
    "esta", "estamos", "estao", "estive", "esteve", "estivemos", "estiveram", "estava", "estavamos", 
    "estavam", "estivera", "estiveramos", "esteja", "estejamos", "estejam", "estivesse", "estivessemos", 
    "estivessem", "estiver", "estivermos", "estiverem", "hei", "há", "havemos", "hao", "houve", 
    "houvemos", "houveram", "houvera", "houveramos", "haja", "hajamos", "hajam", "houvesse", 
    "houvessemos", "houvessem", "houver", "houvermos", "houverem", "houverei", "houvera", "houveremos", 
    "houverao", "houveria", "houveriamos", "houveriam", "sou", "somos", "sao", "era", "eramos", "eram", 
    "fui", "foi", "fomos", "foram", "fora", "foramos", "seja", "sejamos", "sejam", "fosse", "fossemos", 
    "fossem", "for", "formos", "forem", "serei", "sera", "seremos", "serao", "seria", "seriamos", 
    "seriam", "tenho", "tem", "temos", "tem", "tinha", "tinhamos", "tinham", "tive", "teve", "tivemos", 
    "tiveram", "tivera", "tiveramos", "tenha", "tenhamos", "tenham", "tivesse", "tivéssemos", "tivessem", 
    "tiver", "tivermos", "tiverem", "terei", "tera", "teremos", "terao", "teria", "teriamos", "teriam",

    // English.
    "about", "above", "above", "across", "after", "afterwards", "again", "against", "all", 
    "almost", "alone", "along", "already", "also","although","always","am","among", "amongst", 
    "amoungst", "amount",  "an", "and", "another", "any","anyhow","anyone","anything","anyway", 
    "anywhere", "are", "around", "as",  "at", "back","be","became", "because","become","becomes", 
    "becoming", "been", "before", "beforehand", "behind", "being", "below", "beside", "besides", 
    "between", "beyond", "bill", "both", "bottom","but", "by", "call", "can", "cannot", "cant", 
    "co", "con", "could", "couldnt", "cry", "de", "describe", "detail", "do", "done", "down", 
    "due", "during", "each", "eg", "eight", "either", "eleven","else", "elsewhere", "empty", 
    "enough", "etc", "even", "ever", "every", "everyone", "everything", "everywhere", "except", 
    "few", "fifteen", "fify", "fill", "find", "fire", "first", "five", "for", "former", "formerly", 
    "forty", "found", "four", "from", "front", "full", "further", "get", "give", "go", "had", "has", 
    "hasnt", "have", "he", "hence", "her", "here", "hereafter", "hereby", "herein", "hereupon", "hers", 
    "herself", "him", "himself", "his", "how", "however", "hundred", "ie", "if", "in", "inc", "indeed", 
    "interest", "into", "is", "it", "its", "itself", "keep", "last", "latter", "latterly", "least", 
    "less", "ltd", "made", "many", "may", "me", "meanwhile", "might", "mill", "mine", "more", 
    "moreover", "most", "mostly", "move", "much", "must", "my", "myself", "name", "namely", "neither", 
    "never", "nevertheless", "next", "nine", "no", "nobody", "none", "noone", "nor", "not", "nothing", 
    "now", "nowhere", "of", "off", "often", "on", "once", "one", "only", "onto", "or", "other", "others", 
    "otherwise", "our", "ours", "ourselves", "out", "over", "own","part", "per", "perhaps", "please", 
    "put", "rather", "re", "same", "see", "seem", "seemed", "seeming", "seems", "serious", "several", 
    "she", "should", "show", "side", "since", "sincere", "six", "sixty", "so", "some", "somehow", 
    "someone", "something", "sometime", "sometimes", "somewhere", "still", "such", "system", "take", 
    "ten", "than", "that", "the", "their", "them", "themselves", "then", "thence", "there", "thereafter", 
    "thereby", "therefore", "therein", "thereupon", "these", "they", "thickv", "thin", "third", "this", 
    "those", "though", "three", "through", "throughout", "thru", "thus", "to", "together", "too", "top", 
    "toward", "towards", "twelve", "twenty", "two", "un", "under", "until", "up", "upon", "us", "very", 
    "via", "was", "we", "well", "were", "what", "whatever", "when", "whence", "whenever", "where", 
    "whereafter", "whereas", "whereby", "wherein", "whereupon", "wherever", "whether", "which", 
    "while", "whither", "who", "whoever", "whole", "whom", "whose", "why", "will", "with", "within", 
    "without", "would", "yet", "you", "your", "yours", "yourself", "yourselves", "the"
  };

  if (lexeme.size() <= 1 || lexeme.size() >= 30) return true;
  return stop_words.find(lexeme) != stop_words.end();
}

void Extractor::Parse(GumboNode* node, bool get_links) {
  if (node->type == GUMBO_NODE_TEXT) {  
    if (get_links) return;
    doc_text_ += node->v.text.text;
    doc_text_ += " ";
    // std::vector<std::string> lexemes = Lexicon::ExtractLexemes(node->v.text.text);
    // lexemes_.insert(lexemes_.end(), lexemes.begin(), lexemes.end());
  }

  if (node->type != GUMBO_NODE_ELEMENT || node->v.element.tag == GUMBO_TAG_SCRIPT ||
    node->v.element.tag == GUMBO_TAG_STYLE) return;

  GumboVector* children = &node->v.element.children;
  for (unsigned int i = 0; i < children->length; ++i) {
    Parse((GumboNode*) children->data[i], get_links);
  }

  if (!get_links) return;

  GumboAttribute* href;
  bool is_link = node->v.element.tag == GUMBO_TAG_A &&
    (href = gumbo_get_attribute(&node->v.element.attributes, "href"));

  if (!is_link) return;

  std::string anchor_text = GetCleanText(node);
  std::vector<std::string> lexemes = Lexicon::ExtractLexemes(anchor_text);
  for (auto lexeme : lexemes) { 
    unsigned int lexeme_id = lexicon_->GetLexemeId(lexeme);
    if (lexeme_id == 0) continue;

    std::string href_str = href->value;
    std::string normalized_link = NormalizeHyperlink(current_doc_id_, href_str);
    unsigned int doc_id = doc_map_->GetDocId(normalized_link);

    // Ignore links pointing to documents outside the collection and
    // self referring pages.
    if (doc_id == 0 || current_doc_id_ == doc_id) continue; 

    Tuple tuple = Tuple(lexeme_id, doc_id, 0);
    tuple_sorter_->WriteTuple(tuple);

    // lexicon_->AddLink(lexeme_id, doc_id);
    doc_map_->AddOutboundLink(current_doc_id_, doc_id);
  }
}

void Extractor::Parse(const std::string& document, bool get_links) {
  GumboOutput* output = gumbo_parse(document.c_str());
  Parse(output->root, get_links);
  gumbo_destroy_output(&kGumboDefaultOptions, output);
}

void Extractor::ExtractFromDoc(RawDocument& doc) {
  Document new_doc(doc.url, doc.file_number, doc.offset);

  unsigned int id = doc_map_->GetDocId(doc.url);
  if (id != 0) {
    std::stringstream ss;
    ss << "Ignoring duplicate document: " << doc.url << ". File number: " 
       << doc.file_number << ", offset: " << doc.offset << ".";
    logger_->Log(ss.str());
    return; 
  }

  // Gumbo Parser does not handle XML very well. So
  // we will skip XML documents.
  size_t doc_offset = 0;
  while (
    doc.content[doc_offset] == ' ' || 
    doc.content[doc_offset] == '\n' || 
    doc.content[doc_offset] == '\t'
  ) { 
    doc_offset++;
    if (doc_offset >= doc.content.size()) break;
  }
  if (doc.content.find("<?xml") == doc_offset) return;
  if (doc.content.find("<") != doc_offset) {
    logger_->Log("Document " + doc.url + " is not html.");
    return;
  }

  unsigned int doc_id = doc_map_->AddDoc(new_doc);
  Parse(doc.content);
  unsigned int word_count = 0;
  lexemes_ = Lexicon::ExtractLexemes(doc_text_);
  for (auto lexeme : lexemes_) {
    ++word_count;
    if (IsStopWord(lexeme)) continue;
    unsigned int lexeme_id = lexicon_->AddLexeme(lexeme);
    Tuple tuple = Tuple(lexeme_id, doc_id, word_count);
    tuple_sorter_->WriteTuple(tuple);
  }
  lexemes_.clear();
  doc_text_.clear();
}

void Extractor::ExtractLinks(RawDocument& doc) {
  current_doc_id_ = doc_map_->GetDocId(doc.url);

  // We will only get links from pages that were indexed.
  if (current_doc_id_ == 0) {
    logger_->Log("Skipping links from: " + doc.url);
    return;
  }

  Parse(doc.content, true);
  lexemes_.clear();
  doc_text_.clear();
}

std::string Extractor::TruncateUrl(std::string url) {
  if (url.find("http://")    == 0)   url = url.substr(7);
  if (url.find("https://")   == 0)   url = url.substr(8);
  if (url.find("www.")       == 0)   url = url.substr(4);
  while (url[url.size() - 1] == '/') url = url.substr(0, url.size() - 1);
  return url;
}

std::string Extractor::GetRootUrl(std::string url) {
  url = TruncateUrl(url);
  size_t pos = url.find_first_of('/');
  return url.substr(0, pos - 1);
}

std::string Extractor::NormalizeHyperlink(unsigned int doc_id, std::string& url) {
  // Trim.
  url.erase(
    url.begin(), 
    std::find_if(url.begin(), url.end(), 
    std::not1(std::ptr_fun<int, int>(std::isspace)))
  );
  url.erase(
    std::find_if(url.rbegin(), url.rend(),
    std::not1(std::ptr_fun<int, int>(std::isspace))).base(), url.end()
  );

  if (url.find("/") == 0) {
    Document doc = doc_map_->GetDocById(doc_id);
    if (url.size() == 1) return GetRootUrl(doc.url);
    return GetRootUrl(doc.url) + url;
  }

  return TruncateUrl(url);
}

void Extractor::PrintLexemes(RawDocument& doc) {
  Parse(doc.content);
  unsigned int word_count = 0;
  std::vector<std::string> lexemes = Lexicon::ExtractLexemes(doc_text_);
  for (auto lexeme : lexemes) {
    std::cout << "(" << ++word_count << ") " << lexeme << " ";
  }
  std::cout << std::endl;
  lexemes_.clear();
  doc_text_.clear();
}

void Extractor::ReadDoc(unsigned int doc_id) {
  Document doc = doc_map_->GetDocById(doc_id);
  std::cout << "Reading document " << doc.url << " from file " << doc.file_num << 
               " and offset " << doc.offset << std::endl;
  RawDocument raw_doc = doc_collection_->Read(doc.file_num, doc.offset);
  PrintLexemes(raw_doc);
}

std::string Extractor::GetTitle(unsigned int doc_id) {
  Document doc = doc_map_->GetDocById(doc_id);
  RawDocument raw_doc = doc_collection_->Read(doc.file_num, doc.offset);
  GumboOutput* output = gumbo_parse(raw_doc.content.c_str());

  const GumboVector* root_children = &output->root->v.element.children;
  GumboNode* head = NULL;
  for (size_t i = 0; i < root_children->length; ++i) {
    GumboNode* child = (GumboNode*) root_children->data[i];
    if ( 
      child->type == GUMBO_NODE_ELEMENT &&
      child->v.element.tag == GUMBO_TAG_HEAD
    ) {
      head = child;
      break;
    }
  }
  if (head == NULL) return "";

  GumboVector* head_children = &head->v.element.children;
  for (size_t i = 0; i < head_children->length; ++i) {
    GumboNode* child = (GumboNode*) head_children->data[i];
    if (child->type == GUMBO_NODE_ELEMENT &&
        child->v.element.tag == GUMBO_TAG_TITLE) {
      if (child->v.element.children.length != 1) {
        gumbo_destroy_output(&kGumboDefaultOptions, output);
        return "";
      }
      GumboNode* title_text = (GumboNode*) child->v.element.children.data[0];
      if (
        title_text->type != GUMBO_NODE_TEXT && 
        title_text->type != GUMBO_NODE_WHITESPACE
      ) { 
        return "";
      }
      std::string title = title_text->v.text.text;
      gumbo_destroy_output(&kGumboDefaultOptions, output);
      return title;
    }
  }

  gumbo_destroy_output(&kGumboDefaultOptions, output);
  return "";
}

std::string Extractor::GetShortTextAt(unsigned int doc_id, unsigned int position) {
  Document doc = doc_map_->GetDocById(doc_id);
  RawDocument raw_doc = doc_collection_->Read(doc.file_num, doc.offset);

  Parse(raw_doc.content);
  std::vector<std::string> lexemes = Lexicon::ExtractLexemes(doc_text_);

  unsigned int start = ((int) position - 11 >= 0) ? position - 11 : 0;
  unsigned int end = (position + 10 < lexemes.size()) ? position + 10 : lexemes.size();

  std::string result;
  start = Lexicon::lexeme_offsets[start];
  end = Lexicon::lexeme_offsets[end];
  
  std::string short_text = doc_text_.substr(start, end - start);
  lexemes_.clear();
  doc_text_.clear();
  return short_text;
}

std::string Extractor::GetRawText(unsigned int doc_id) {
  Document doc = doc_map_->GetDocById(doc_id);
  RawDocument raw_doc = doc_collection_->Read(doc.file_num, doc.offset);

  return raw_doc.content;
}

} // End of namespace.
