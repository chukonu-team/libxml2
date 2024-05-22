#include "libxml/HTMLparser.h"
#include "libxml/HTMLtree.h"
#include "libxml/tree.h"
#include "libxml.h"
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/globals.h>
#include <stdio.h>

const char* readFromFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 1);
    fread(buffer, 1, length, file);
    fclose(file);
    buffer[length] = '\0';
    return buffer;
}



int main(int argc, char* argv[]) {
    htmlDocPtr doc;
    htmlNodePtr cur;

    doc = htmlReadFile("CC-MAIN-20230606214755-20230607004755-00000_300697_340.html", "utf-8", HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == NULL) {
        fprintf(stderr, "Document not parsed successfully. \n");
        return 0;
    }
    // htmlDocDump(stdout, doc);
    xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
	ctxt->node = xmlDocGetRootElement(doc);

    const char* xpath = readFromFile("a.txt");
    printf("%s\n", xpath);
    xmlXPathCompExprPtr comp = xmlXPathCompile(BAD_CAST xpath);
    xmlXPathObjectPtr res = xmlXPathCompiledEval(comp, ctxt);
    xmlXPathDebugDumpObject(stdout, res, 0);
    return 0;
}
