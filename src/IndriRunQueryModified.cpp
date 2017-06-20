#include <time.h>
#include "indri/QueryEnvironment.hpp"
#include "indri/LocalQueryServer.hpp"
#include "indri/delete_range.hpp"
#include "indri/NetworkStream.hpp"
#include "indri/NetworkMessageStream.hpp"
#include "indri/NetworkServerProxy.hpp"

#include "indri/ListIteratorNode.hpp"
#include "indri/ExtentInsideNode.hpp"
#include "indri/DocListIteratorNode.hpp"
#include "indri/FieldIteratorNode.hpp"

#include "indri/Parameters.hpp"

#include "indri/ParsedDocument.hpp"
#include "indri/Collection.hpp"
#include "indri/CompressedCollection.hpp"
#include "indri/TaggedDocumentIterator.hpp"
#include "indri/XMLNode.hpp"

#include "indri/QueryExpander.hpp"
#include "indri/RMExpander.hpp"
#include "indri/PonteExpander.hpp"
// need a QueryExpanderFactory....
#include "indri/TFIDFExpander.hpp"

#include "indri/IndriTimer.hpp"
#include "indri/UtilityThread.hpp"
#include "indri/ScopedLock.hpp"
#include "indri/delete_range.hpp"
#include "indri/SnippetBuilder.hpp"

#include <queue>






///////////////////////////////////////// My code.

#include <math.h>
#include "indri/RelevanceModel.hpp"

#define MAX_SIZE 2000
#define N 10
#define MAX_W 50

// Driver function to sort the vector elements
// by second element of pairs
bool sortbysec(const std::pair<std::string, double> &a, const std::pair<std::string, double> &b) {
  return (a.second > b.second);
}

class Vectors {
 public:
  // double centroid; // Word Vector
  FILE *f;
  char st1[MAX_SIZE];
  char *bestw[N];
  char file_name[MAX_SIZE], st[100][MAX_SIZE];
  float dist, len, bestd[N], vec[MAX_SIZE];
  long long words, size, a, b, c, d, cn, bi[100];
  char ch;
  float *M;
  char *vocab;
  std::map<std::string, double> word_vectors_;

  int LoadWord2VecBin(const char* filename) {
    strcpy(file_name, filename);
    f = fopen(file_name, "rb");
    if (f == NULL) {
      printf("Input file not found\n");
      return -1;
    }
    fscanf(f, "%lld", &words);
    fscanf(f, "%lld", &size);
    vocab = (char *)malloc((long long)words * MAX_W * sizeof(char));
    for (a = 0; a < N; a++) bestw[a] = (char *)malloc(MAX_SIZE * sizeof(char));
    M = (float *)malloc((long long)words * (long long)size * sizeof(float));
    if (M == NULL) {
      printf("Cannot allocate memory: %lld MB    %lld  %lld\n", (long long)words * size * sizeof(float) / 1048576, words, size);
      return -1;
    }
    for (b = 0; b < words; b++) {
      a = 0;
      while (1) {
        vocab[b * MAX_W + a] = fgetc(f);
        if (feof(f) || (vocab[b * MAX_W + a] == ' ')) break;
        if ((a < MAX_W) && (vocab[b * MAX_W + a] != '\n')) a++;
      }
      vocab[b * MAX_W + a] = 0;
      for (a = 0; a < size; a++) fread(&M[a + b * size], sizeof(float), 1, f);
      len = 0;
      for (a = 0; a < size; a++) len += M[a + b * size] * M[a + b * size];
      len = sqrt(len);
      for (a = 0; a < size; a++) M[a + b * size] /= len;
    }
    fclose(f);
    return 0;
  }

  void CalculateQueryCentroid(const std::string& st1) {
    word_vectors_.clear();

    cn = 0;
    b = 0;
    c = 0;
    while (1) {
      st[cn][b] = st1[c];
      b++;
      c++;
      st[cn][b] = 0;
      if (st1[c] == 0) break;
      if (st1[c] == ' ') {
        cn++;
        b = 0;
        c++;
      }
    }
    cn++;
    for (a = 0; a < cn; a++) {
      for (b = 0; b < words; b++) if (!strcmp(&vocab[b * MAX_W], st[a])) break;
      if (b == words) b = -1;
      bi[a] = b;
      // if (b == -1) break; // Out of dictionary word.
    }
    // if (b == -1) return;

    for (a = 0; a < size; a++) vec[a] = 0;
    for (b = 0; b < cn; b++) {
      if (bi[b] == -1) continue;
      for (a = 0; a < size; a++) vec[a] += M[a + bi[b] * size];
    }
    len = 0;
    for (a = 0; a < size; a++) len += vec[a] * vec[a];
    len = sqrt(len);
    for (a = 0; a < size; a++) vec[a] /= len;
    for (a = 0; a < N; a++) bestd[a] = -1;
    for (a = 0; a < N; a++) bestw[a][0] = 0;
    for (c = 0; c < words; c++) {
      a = 0;
      for (b = 0; b < cn; b++) if (bi[b] == c) a = 1;
      if (a == 1) continue;
      dist = 0;
      for (a = 0; a < size; a++) dist += vec[a] * M[a + c * size];
      for (a = 0; a < N; a++) {
        if (dist > bestd[a]) {
          for (d = N - 1; d > a; d--) {
            bestd[d] = bestd[d - 1];
            strcpy(bestw[d], bestw[d - 1]);
          }
          bestd[a] = dist;
          strcpy(bestw[a], &vocab[c * MAX_W]);
          break;
        }
      }
    }

    double sum = 0;
    for (a = 0; a < N; a++) {
      word_vectors_[bestw[a]] = exp(bestd[a]);
      sum += exp(bestd[a]);
      // printf("%50s\t\t%f\n", bestw[a], bestd[a]);
    }

    for (a = 0; a < N; a++) {
      word_vectors_[bestw[a]] /= sum;
    }
  }
};


class Word2VecExpander : public indri::query::QueryExpander  {
  indri::api::Parameters param;

public:
  Vectors v1;
  Vectors v2;

  Word2VecExpander( indri::api::QueryEnvironment * env , indri::api::Parameters& param )
    : indri::query::QueryExpander( env, param ), param(param) { }

  std::string expand( std::string originalQuery , std::vector<indri::api::ScoredExtentResult>& results ) {
    int fbDocs = _param.get( "fbDocs" , 10 );
    int fbTerms = _param.get( "fbTerms" , 10 );
    double fbOrigWt = _param.get( "fbOrigWeight", 0.5 );
    double mu = _param.get( "fbMu", 0 );
  
    std::string rmSmoothing = "";
    if (mu != 0) // specify dirichlet smoothing
      rmSmoothing = "method:dirichlet,mu:" + (std::string)_param.get( "fbMu", "0");
  
    // this should be a parameter, have to change the generation to
    // account for phrases then.
    int maxGrams = 1;
    
    indri::query::RelevanceModel rm(*_env, rmSmoothing, maxGrams, fbDocs);
    rm.generate( originalQuery, results );
  
    const std::vector<indri::query::RelevanceModel::Gram*>& grams = rm.getGrams();
    std::vector< std::pair<std::string, double> > probs;
    // sorted grams came from rm
    for( size_t j=0; j<grams.size(); j++ ) {
      double w = grams[j]->weight;
      // multi terms need to be handled if maxGrams becomes a parameter
      std::string &term = grams[j]->terms[0]; 
      probs.push_back( std::pair<std::string, double>( term, w ) );    
    }
    std::string expQuery;


    // BEGIN - Word2vec integration.
    double sum = 0;
    for (size_t i = 0; i < probs.size(); ++i) {
      sum += probs[i].second;
      if (i >= 50) break;
    }
    for (size_t i = 0; i < probs.size(); ++i) {
      probs[i].second /= sum;
    }

    double alpha = atof(param.get( "w2vec_alpha", "0.5" ).c_str());
    double beta = atof(param.get( "w2vec_beta", "0.5" ).c_str());
    size_t counter = 0;
    std::map<std::string, double>::iterator it;

    std::vector< std::pair<std::string, double> > w2vec_probs;
    if (param.get( "global_w2vec", "false" ) == "true") {
      if (param.get( "local_w2vec", "false" ) != "true") beta = 1;
      v1.CalculateQueryCentroid(originalQuery);
      it = v1.word_vectors_.begin();
      for (; it != v1.word_vectors_.end(); ++it) {
        if (it->first.find('_') != std::string::npos) continue;
        w2vec_probs.push_back(pair<string, double>(it->first, beta * it->second));
        if (++counter > 50) break;
      }
    }

    if (param.get( "local_w2vec", "false" ) == "true") {
      if (param.get( "global_w2vec", "false" ) != "true") beta = 0;
      v2.CalculateQueryCentroid(originalQuery);
      counter = 0;
      it = v2.word_vectors_.begin();
      for (; it != v2.word_vectors_.end(); ++it) {
        if (it->first.find('_') != std::string::npos) continue;
        bool found = false;
        for (size_t i = 0; i < w2vec_probs.size(); ++i) { 
          if (w2vec_probs[i].first == it->first) {
            w2vec_probs[i].second = w2vec_probs[i].second + (1 - beta) * it->second;
            found = true;
            break;
          }
        }
        if (!found)
          w2vec_probs.push_back(pair<string, double>(it->first, (1 - beta) * it->second));
        if (++counter > 50) break;
      }
    }

    std::sort(w2vec_probs.begin(), w2vec_probs.end(), sortbysec);

    for (size_t i = 0; i < w2vec_probs.size(); ++i) {
      bool found = false;
      for (size_t j = 0; j < probs.size(); ++j) {
        if (probs[i].first == w2vec_probs[i].first) {
          probs[i].second = alpha * w2vec_probs[i].second + (1 - alpha) * probs[i].second;
          found = true;
          break;
        }
      }
      if (!found) probs.push_back( std::pair<std::string, double>(w2vec_probs[i].first, alpha * w2vec_probs[i].second));    
      if (++counter > 100) break;
    }

    std::sort(probs.begin(), probs.end(), sortbysec);
    // END - Word2vec integration.
    
    // if this was an extent restricted query, move the restriction outside
    // the expansion.
    // For field restrictions, it could be left nested, FixedPassageNode
    // restrictions can not be nested in the query.
    int openBrace = originalQuery.find('[');
    if ( openBrace != std::string::npos) {
      int closeBrace = originalQuery.find(']');
      int firstParen = originalQuery.find('('); // must be one if '['
      if ( openBrace < firstParen ) {
        std::string qCopy = originalQuery;
        // get the restriction
        std::string restrict = originalQuery.substr(openBrace+1, 
                                                    closeBrace-openBrace-1);
        // remove the inner restriction
        qCopy.erase(openBrace, closeBrace-openBrace+1);
        expQuery = buildQuery( qCopy, fbOrigWt, probs, fbTerms );
        return "#combine[" + restrict + "]( " + expQuery + " )";
      }
    }
    return buildQuery( originalQuery, fbOrigWt, probs, fbTerms );
  }
};


///////////////////////////////////////// End of my code.










static bool copy_parameters_to_string_vector( std::vector<std::string>& vec, indri::api::Parameters p, const std::string& parameterName ) {
  if( !p.exists(parameterName) )
    return false;

  indri::api::Parameters slice = p[parameterName];

  for( size_t i=0; i<slice.size(); i++ ) {
    vec.push_back( slice[i] );
  }

  return true;
}

struct query_t {
  struct greater {
    bool operator() ( query_t* one, query_t* two ) {
      return one->index > two->index;
    }
  };

  query_t( int _index, std::string _number, const std::string& _text, const std::string &queryType,  std::vector<std::string> workSet,   std::vector<std::string> FBDocs) :
    index( _index ),
    number( _number ),
    text( _text ), qType(queryType), workingSet(workSet), relFBDocs(FBDocs)
  {
  }

  query_t( int _index, std::string _number, const std::string& _text ) :
    index( _index ),
    number( _number ),
    text( _text )
  {
  }

  std::string number;
  int index;
  std::string text;
  std::string qType;
  // working set to restrict retrieval
  std::vector<std::string> workingSet;
  // Rel fb docs
  std::vector<std::string> relFBDocs;
};

class QueryThread : public indri::thread::UtilityThread {
private:
  indri::thread::Lockable& _queueLock;
  indri::thread::ConditionVariable& _queueEvent;
  std::queue< query_t* >& _queries;
  std::priority_queue< query_t*, std::vector< query_t* >, query_t::greater >& _output;

  indri::api::QueryEnvironment _environment;
  indri::api::Parameters& _parameters;
  int _requested;
  int _initialRequested;

  bool _printDocuments;
  bool _printPassages;
  bool _printSnippets;
  bool _printQuery;

  std::string _runID;
  bool _trecFormat;
  bool _inexFormat;

  indri::query::QueryExpander* _expander;
  std::vector<indri::api::ScoredExtentResult> _results;
  indri::api::QueryAnnotation* _annotation;

  // Runs the query, expanding it if necessary.  Will print output as well if verbose is on.
  void _runQuery( std::stringstream& output, const std::string& query,
                  const std::string &queryType, const std::vector<std::string> &workingSet, std::vector<std::string> relFBDocs ) {
    try {
      if( _printQuery ) output << "# query: " << query << std::endl;
      std::vector<lemur::api::DOCID_T> docids;;
      if (workingSet.size() > 0) 
        docids = _environment.documentIDsFromMetadata("docno", workingSet);

      if (relFBDocs.size() == 0) {
          if( _printSnippets ) {
            if (workingSet.size() > 0) 
              _annotation = _environment.runAnnotatedQuery( query, docids, _initialRequested, queryType ); 
            else
              _annotation = _environment.runAnnotatedQuery( query, _initialRequested );
            _results = _annotation->getResults();
          } else {
            if (workingSet.size() > 0)
              _results = _environment.runQuery( query, docids, _initialRequested, queryType );
            else
              _results = _environment.runQuery( query, _initialRequested, queryType );
          }
      }
      
      if( _expander ) {
        std::vector<indri::api::ScoredExtentResult> fbDocs;
        if (relFBDocs.size() > 0) {
          docids = _environment.documentIDsFromMetadata("docno", relFBDocs);
          for (size_t i = 0; i < docids.size(); i++) {
            indri::api::ScoredExtentResult r(0.0, docids[i]);
            fbDocs.push_back(r);
          }
        }
        std::string expandedQuery;
        if (relFBDocs.size() != 0)
          expandedQuery = _expander->expand( query, fbDocs );
        else
          expandedQuery = _expander->expand( query, _results );
        if( _printQuery ) output << "# expanded: " << expandedQuery << std::endl;
        if (workingSet.size() > 0) {
          docids = _environment.documentIDsFromMetadata("docno", workingSet);
          _results = _environment.runQuery( expandedQuery, docids, _requested, queryType );
        } else {
          _results = _environment.runQuery( expandedQuery, _requested, queryType );
        }
      }
    }
    catch( lemur::api::Exception& e )
    {
      _results.clear();
      LEMUR_RETHROW(e, "QueryThread::_runQuery Exception");
    }
  }

  void _printResultRegion( std::stringstream& output, std::string queryIndex, int start, int end  ) {
    std::vector<std::string> documentNames;
    std::vector<indri::api::ParsedDocument*> documents;

    std::vector<indri::api::ScoredExtentResult> resultSubset;

    resultSubset.assign( _results.begin() + start, _results.begin() + end );


    // Fetch document data for printing
    if( _printDocuments || _printPassages || _printSnippets ) {
      // Need document text, so we'll fetch the whole document
      documents = _environment.documents( resultSubset );
      documentNames.clear();

      for( size_t i=0; i<resultSubset.size(); i++ ) {
        indri::api::ParsedDocument* doc = documents[i];
        std::string documentName;

        indri::utility::greedy_vector<indri::parse::MetadataPair>::iterator iter = std::find_if( documents[i]->metadata.begin(),
          documents[i]->metadata.end(),
          indri::parse::MetadataPair::key_equal( "docno" ) );

        if( iter != documents[i]->metadata.end() )
          documentName = (char*) iter->value;

        // store the document name in a separate vector so later code can find it
        documentNames.push_back( documentName );
      }
    } else {
      // We only want document names, so the documentMetadata call may be faster
      documentNames = _environment.documentMetadata( resultSubset, "docno" );
    }

    std::vector<std::string> pathNames;
    if ( _inexFormat ) {
      // retrieve path names
      pathNames = _environment.pathNames( resultSubset );
    }

    // Print results
    for( size_t i=0; i < resultSubset.size(); i++ ) {
      int rank = start+i+1;
      std::string queryNumber = queryIndex;

      if( _trecFormat ) {
        // TREC formatted output: queryNumber, Q0, documentName, rank, score, runID
        output << queryNumber << " "
                << "Q0 "
                << documentNames[i] << " "
                << rank << " "
                << resultSubset[ i ].score << " "
                << _runID << std::endl;
      } else if( _inexFormat ) {

  output << "    <result>" << std::endl
         << "      <file>" << documentNames[i] << "</file>" << std::endl
         << "      <path>" << pathNames[i] << "</path>" << std::endl
         << "      <rsv>" << resultSubset[i].score << "</rsv>"  << std::endl
         << "    </result>" << std::endl;
      }
      else {
        // score, documentName, firstWord, lastWord
        output << resultSubset[i].score << "\t"
                << documentNames[i] << "\t"
                << resultSubset[i].begin << "\t"
                << resultSubset[i].end << std::endl;
      }

      if( _printDocuments ) {
        output << documents[i]->text << std::endl;
      }

      if( _printPassages ) {
        int byteBegin = documents[i]->positions[ resultSubset[i].begin ].begin;
        int byteEnd = documents[i]->positions[ resultSubset[i].end-1 ].end;
        output.write( documents[i]->text + byteBegin, byteEnd - byteBegin );
        output << std::endl;
      }

      if( _printSnippets ) {
        indri::api::SnippetBuilder builder(false);
        output << builder.build( resultSubset[i].document, documents[i], _annotation ) << std::endl;
      }

      if( documents.size() )
        delete documents[i];
    }
  }

  void _printResults( std::stringstream& output, std::string queryNumber ) {
    if (_inexFormat) {
      // output topic header
      output << "  <topic topic-id=\"" << queryNumber << "\">" << std::endl
             << "    <collections>" << std::endl
             << "      <collection>ieee</collection>" << std::endl
             << "    </collections>" << std::endl;
    }
    for( size_t start = 0; start < _results.size(); start += 50 ) {
      size_t end = std::min<size_t>( start + 50, _results.size() );
      _printResultRegion( output, queryNumber, start, end );
    }
    if( _inexFormat ) {
      output << "  </topic>" << std::endl;
    }
    delete _annotation;
    _annotation = 0;
  }


public:
  QueryThread( std::queue< query_t* >& queries,
               std::priority_queue< query_t*, std::vector< query_t* >, query_t::greater >& output,
               indri::thread::Lockable& queueLock,
               indri::thread::ConditionVariable& queueEvent,
               indri::api::Parameters& params ) :
    _queries(queries),
    _output(output),
    _queueLock(queueLock),
    _queueEvent(queueEvent),
    _parameters(params),
    _expander(0),
    _annotation(0)
  {
  }

  ~QueryThread() {
  }

  UINT64 initialize() {
    try {        
    _environment.setSingleBackgroundModel( _parameters.get("singleBackgroundModel", false) );

    std::vector<std::string> stopwords;
    if( copy_parameters_to_string_vector( stopwords, _parameters, "stopper.word" ) )
      _environment.setStopwords(stopwords);

    std::vector<std::string> smoothingRules;
    if( copy_parameters_to_string_vector( smoothingRules, _parameters, "rule" ) )
      _environment.setScoringRules( smoothingRules );

   if( _parameters.exists( "index" ) ) {
      indri::api::Parameters indexes = _parameters["index"];

      for( size_t i=0; i < indexes.size(); i++ ) {
        _environment.addIndex( std::string(indexes[i]) );
      }
    }

    if( _parameters.exists( "server" ) ) {
      indri::api::Parameters servers = _parameters["server"];

      for( size_t i=0; i < servers.size(); i++ ) {
        _environment.addServer( std::string(servers[i]) );
      }
    }

    if( _parameters.exists("maxWildcardTerms") )
        _environment.setMaxWildcardTerms(_parameters.get("maxWildcardTerms", 100));

    _requested = _parameters.get( "count", 1000 );
    _initialRequested = _parameters.get( "fbDocs", _requested );
    _runID = _parameters.get( "runID", "indri" );
    _trecFormat = _parameters.get( "trecFormat" , false );
    _inexFormat = _parameters.exists( "inex" );

    _printQuery = _parameters.get( "printQuery", false );
    _printDocuments = _parameters.get( "printDocuments", false );
    _printPassages = _parameters.get( "printPassages", false );
    _printSnippets = _parameters.get( "printSnippets", false );

    if (_parameters.exists("baseline")) {
      // doing a baseline
      std::string baseline = _parameters["baseline"];
      _environment.setBaseline(baseline);
      // need a factory for this...
      if( _parameters.get( "fbDocs", 0 ) != 0 ) {
        // have to push the method in...
        std::string rule = "method:" + baseline;
        _parameters.set("rule", rule);
        _expander = new indri::query::TFIDFExpander( &_environment, _parameters );
      }
    } else {
      if( _parameters.get( "fbDocs", 0 ) != 0 ) {
        // _expander = new indri::query::RMExpander( &_environment, _parameters );

        // Alterei aqui !!!!!!!!!!!!!!!!!!!!!!!!!
        Word2VecExpander* mygod = new Word2VecExpander( &_environment, _parameters );
        if (_parameters.get( "local_w2vec", "false" ) == "true" || _parameters.get( "global_w2vec", "false" ) == "true") {
          mygod->v1.LoadWord2VecBin(_parameters.get( "wordVectorFile", "" ).c_str());
          mygod->v2.LoadWord2VecBin(_parameters.get( "wordLocalVectorFile", "" ).c_str());
        }
        _expander = mygod;
      }
    }

    if (_parameters.exists("maxWildcardTerms")) {
      _environment.setMaxWildcardTerms((int)_parameters.get("maxWildcardTerms"));
    }    
    } catch ( lemur::api::Exception& e ) {      
      while( _queries.size() ) {
        query_t *query = _queries.front();
        _queries.pop();
        _output.push( new query_t( query->index, query->number, "query: " + query->number + " QueryThread::_initialize exception\n" ) );
        _queueEvent.notifyAll();
        LEMUR_RETHROW(e, "QueryThread::_initialize");
      }
    }
    return 0;
  }

  void deinitialize() {
    delete _expander;
    _environment.close();
  }

  bool hasWork() {
    indri::thread::ScopedLock sl( &_queueLock );
    return _queries.size() > 0;
  }

  UINT64 work() {
    query_t* query;
    std::stringstream output;

    // pop a query off the queue
    {
      indri::thread::ScopedLock sl( &_queueLock );
      if( _queries.size() ) {
        query = _queries.front();
        _queries.pop();
      } else {
        return 0;
      }
    }

    // run the query
    try {
      if (_parameters.exists("baseline") && ((query->text.find("#") != std::string::npos) || (query->text.find(".") != std::string::npos)) ) {
        LEMUR_THROW( LEMUR_PARSE_ERROR, "Can't run baseline on this query: " + query->text + "\nindri query language operators are not allowed." );
      }
      _runQuery( output, query->text, query->qType, query->workingSet, query->relFBDocs );
    } catch( lemur::api::Exception& e ) {
      output << "# EXCEPTION in query " << query->number << ": " << e.what() << std::endl;
    }

    // print the results to the output stream
    _printResults( output, query->number );

    // push that data into an output queue...?
    {
      indri::thread::ScopedLock sl( &_queueLock );
      _output.push( new query_t( query->index, query->number, output.str() ) );
      _queueEvent.notifyAll();
    }

    delete query;
    return 0;
  }
};

void push_queue( std::queue< query_t* >& q, indri::api::Parameters& queries,
                 int queryOffset ) {

  for( size_t i=0; i<queries.size(); i++ ) {
    std::string queryNumber;
    std::string queryText;
    std::string queryType = "indri";
    if( queries[i].exists( "type" ) )
      queryType = (std::string) queries[i]["type"];
    if (queries[i].exists("text"))
      queryText = (std::string) queries[i]["text"];
    if( queries[i].exists( "number" ) ) {
      queryNumber = (std::string) queries[i]["number"];
    } else {
      int thisQuery=queryOffset + int(i);
      std::stringstream s;
      s << thisQuery;
      queryNumber = s.str();
    }
    if (queryText.size() == 0)
      queryText = (std::string) queries[i];

    // working set and RELFB docs go here.
    // working set to restrict retrieval
    std::vector<std::string> workingSet;
    // Rel fb docs
    std::vector<std::string> relFBDocs;
    copy_parameters_to_string_vector( workingSet, queries[i], "workingSetDocno" );
    copy_parameters_to_string_vector( relFBDocs, queries[i], "feedbackDocno" );

    q.push( new query_t( i, queryNumber, queryText, queryType, workingSet, relFBDocs ) );

  }
}

int main(int argc, char * argv[]) {
  try {
    indri::api::Parameters& param = indri::api::Parameters::instance();
    param.loadCommandLine( argc, argv );

    if( param.get( "version", 0 ) ) {
      std::cout << INDRI_DISTRIBUTION << std::endl;
    }

    if( !param.exists( "query" ) )
      LEMUR_THROW( LEMUR_MISSING_PARAMETER_ERROR, "Must specify at least one query." );

    if( !param.exists("index") && !param.exists("server") )
      LEMUR_THROW( LEMUR_MISSING_PARAMETER_ERROR, "Must specify a server or index to query against." );

    if (param.exists("baseline") && param.exists("rule"))
      LEMUR_THROW( LEMUR_BAD_PARAMETER_ERROR, "Smoothing rules may not be specified when running a baseline." );

    int threadCount = param.get( "threads", 1 );
    std::queue< query_t* > queries;
    std::priority_queue< query_t*, std::vector< query_t* >, query_t::greater > output;
    std::vector< QueryThread* > threads;
    indri::thread::Mutex queueLock;
    indri::thread::ConditionVariable queueEvent;

    // push all queries onto a queue
    indri::api::Parameters parameterQueries = param[ "query" ];
    int queryOffset = param.get( "queryOffset", 0 );
    push_queue( queries, parameterQueries, queryOffset );
    int queryCount = (int)queries.size();

    // launch threads
    for( int i=0; i<threadCount; i++ ) {
      threads.push_back( new QueryThread( queries, output, queueLock, queueEvent, param ) );
      threads.back()->start();
    }

    int query = 0;

    bool inexFormat = param.exists( "inex" );
    if( inexFormat ) {
      std::string participantID = param.get( "inex.participantID", "1");
      std::string runID = param.get( "runID", "indri" );
      std::string inexTask = param.get( "inex.task", "CO.Thorough" );
      std::string inexTopicPart = param.get( "inex.topicPart", "T" );
      std::string description = param.get( "inex.description", "" );
      std::string queryType = param.get("inex.query", "automatic");
      std::cout << "<inex-submission participant-id=\"" << participantID
    << "\" run-id=\"" << runID
    << "\" task=\"" << inexTask
    << "\" query=\"" << queryType
    << "\" topic-part=\"" << inexTopicPart
    << "\">" << std::endl
    << "  <description>" << std::endl << description
    << std::endl << "  </description>" << std::endl;
    }

    // acquire the lock.
    queueLock.lock();

    // process output as it appears on the queue
    while( query < queryCount ) {
      query_t* result = NULL;

      // wait for something to happen
      queueEvent.wait( queueLock );

      while( output.size() && output.top()->index == query ) {
        result = output.top();
        output.pop();

        queueLock.unlock();

        std::cout << result->text;
        delete result;
        query++;

        queueLock.lock();
      }
    }
    queueLock.unlock();

    if( inexFormat ) {
      std::cout << "</inex-submission>" << std::endl;
    }

    // join all the threads
    for( size_t i=0; i<threads.size(); i++ )
      threads[i]->join();

    // we've seen all the query output now, so we can quit
    indri::utility::delete_vector_contents( threads );
  } catch( lemur::api::Exception& e ) {
    LEMUR_ABORT(e);
  } catch( ... ) {
    std::cout << "Caught unhandled exception" << std::endl;
    return -1;
  }

  return 0;
}

