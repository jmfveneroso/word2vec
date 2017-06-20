#include <vector>
 #include "indri/QueryEnvironment.hpp"
 #include "indri/SnippetBuilder.hpp"

 using namespace indri::api;

 int main(int argc, char *argv[]) {
   // we assume the index path is the first argument and the query is second
   char *indexPath=argv[1];
   char *query=argv[2];

   // our builder object - false in the constructor means no HTML output.
   SnippetBuilder builder(false);

   // create a query environment
   QueryEnvironment indriEnvironment;

   // open the index
   indriEnvironment.addIndex(indexPath);

   // run the query, max of 1000 results
   QueryAnnotation *results=indriEnvironment.runAnnotatedQuery(query, 1000);

   // extract the results as a vector of ScoredExtentResult items
   std::vector<indri::api::ScoredExtentResult> resultVector=results->getResults();

   // get the number of results
   int totalNumResults=resultVector.size();

   // get the parsed documents for the results
   std::vector<ParsedDocument*> parsedDocs=indriEnvironment.documents(resultVector);

   // for each result, print out the document ID and the snippet...
   for (int i=0; i < totalNumResults; i++) {
     // get the document ID
     int thisResultDocID=resultVector[i].document;

     // get this document's parsed doc representation
     ParsedDocument* parsedDoc=parsedDocs[i];

     // print the document ID and the snippet
     cout << thisResultDocID << "\t";
     cout << builder.build(thisResultDocID, parsedDoc, results) << "\n";
   }

  // note that we do not need to explicitly delete the
  // QueryEnvironment object here to close the index. It will
  // automatically be removed when it goes out of scope.

  return 0;
 }
