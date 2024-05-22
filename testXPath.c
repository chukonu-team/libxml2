/*
 * testXPath.c : a small tester program for XPath.
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#include "libxml.h"
#include "libxml/HTMLparser.h"
#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_DEBUG_ENABLED)

#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/debugXML.h>
#include <libxml/xmlmemory.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlerror.h>
#include <libxml/globals.h>
#if defined(LIBXML_XPTR_ENABLED)
#include <libxml/xpointer.h>
static int xptr = 0;
#endif
static int debug = 0;
static int valid = 0;
static int expr = 0;
static int tree = 0;
static int nocdata = 0;
static xmlDocPtr document = NULL;

/*
 * Default document
 */
// static xmlChar buffer[] =
// "<?xml version=\"1.0\"?>\n\
// <EXAMPLE prop1=\"gnome is great\" prop2=\"&amp; linux too\">\n\
//   <head>\n\
//    <title>Welcome to Gnome</title>\n\
//   </head>\n\
//   <chapter>\n\
//    <title>The Linux adventure</title>\n\
//    <p>bla bla bla ...</p>\n\
//    <image href=\"linus.gif\"/>\n\
//    <p>...</p>\n\
//   </chapter>\n\
//   <chapter>\n\
//    <title>Chapter 2</title>\n\
//    <p>this is chapter 2 ...</p>\n\
//   </chapter>\n\
//   <chapter>\n\
//    <title>Chapter 3</title>\n\
//    <p>this is chapter 3 ...</p>\n\
//   </chapter>\n\
// </EXAMPLE>\n\
// ";

static xmlChar* buffer = NULL;

static void readFromFile(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        return;
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    buffer = (xmlChar*)malloc(length + 1);
    if (buffer == NULL) {
        fclose(file);
        return;
    }
    fread(buffer, 1, length, file);
    fclose(file);
    buffer[length] = '\0';
}

#include <signal.h>

typedef struct _xmlXPathStepOp xmlXPathStepOp;
typedef xmlXPathStepOp *xmlXPathStepOpPtr;

typedef enum {
    XPATH_OP_END=0,
    XPATH_OP_AND,
    XPATH_OP_OR,
    XPATH_OP_EQUAL,
    XPATH_OP_CMP,
    XPATH_OP_PLUS,
    XPATH_OP_MULT,
    XPATH_OP_UNION,
    XPATH_OP_ROOT,
    XPATH_OP_NODE,
    XPATH_OP_COLLECT,
    XPATH_OP_VALUE, /* 11 */
    XPATH_OP_VARIABLE,
    XPATH_OP_FUNCTION,
    XPATH_OP_ARG,
    XPATH_OP_PREDICATE,
    XPATH_OP_FILTER, /* 16 */
    XPATH_OP_SORT /* 17 */
#ifdef LIBXML_XPTR_ENABLED
    ,XPATH_OP_RANGETO
#endif
} xmlXPathOp;

struct _xmlXPathStepOp {
    xmlXPathOp op;		/* The identifier of the operation */
    int ch1;			/* First child */
    int ch2;			/* Second child */
    int value;
    int value2;
    int value3;
    void *value4;
    void *value5;
    xmlXPathFunction cache;
    void *cacheURI;
};

struct _xmlXPathCompExpr {
    int nbStep;			/* Number of steps in this expression */
    int maxStep;		/* Maximum number of steps allocated */
    xmlXPathStepOp *steps;	/* ops for computation of this expression */
    int last;			/* index of last step in expression */
    xmlChar *expr;		/* the expression being computed */
    xmlDictPtr dict;		/* the dictionary to use if any */
#ifdef DEBUG_EVAL_COUNTS
    int nb;
    xmlChar *string;
#endif
#ifdef XPATH_STREAMING
    xmlPatternPtr stream;
#endif
};

static void
testXPath(const char *str) {
    xmlXPathObjectPtr res;
    xmlXPathContextPtr ctxt;

#if defined(LIBXML_XPTR_ENABLED)
    if (xptr) {
	ctxt = xmlXPtrNewContext(document, NULL, NULL);
	res = xmlXPtrEval(BAD_CAST str, ctxt);
    } else {
#endif
	ctxt = xmlXPathNewContext(document);
	ctxt->node = xmlDocGetRootElement(document);
	if (expr) {
		printf("%s:%d\n", __func__, __LINE__);
	    res = xmlXPathEvalExpression(BAD_CAST str, ctxt);
	} else {
		printf("%s:%d\n", __func__, __LINE__);
	    /* res = xmlXPathEval(BAD_CAST str, ctxt); */
	    xmlXPathCompExprPtr comp;

	    comp = xmlXPathCompile(BAD_CAST str);
	    if (comp != NULL) {
			printf("%s:%d\n", __func__, __LINE__);
		// if (tree)
        xmlXPathDebugDumpCompExpr(stdout, comp, 0);

		res = xmlXPathCompiledEval(comp, ctxt);
		// raise(SIGTRAP);
		xmlXPathFreeCompExpr(comp);
	    } else
		res = NULL;
	}
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    xmlXPathDebugDumpObject(stdout, res, 0);
    xmlXPathFreeObject(res);
    xmlXPathFreeContext(ctxt);
}

static void
testXPathFile(const char *filename) {
    FILE *input;
    char expression[5000];
    int len;

    input = fopen(filename, "r");
    if (input == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"Cannot open %s for reading\n", filename);
	return;
    }
    while (fgets(expression, 4500, input) != NULL) {
	len = strlen(expression);
	len--;
	while ((len >= 0) &&
	       ((expression[len] == '\n') || (expression[len] == '\t') ||
		(expression[len] == '\r') || (expression[len] == ' '))) len--;
	expression[len + 1] = 0;
	if (len >= 0) {
	    printf("\n========================\nExpression: %s\n", expression) ;
	    testXPath(expression);
	}
    }

    fclose(input);
}

int main(int argc, char **argv) {
    readFromFile("/home/ybw/ghp/libxml2/CC-MAIN-20230606214755-20230607004755-00000_300697_340.html");
    printf("Doc:\n %s\n", buffer);

    int i;
    int strings = 0;
    int usefile = 0;
    char *filename = NULL;

    for (i = 1; i < argc ; i++) {
#if defined(LIBXML_XPTR_ENABLED)
	if ((!strcmp(argv[i], "-xptr")) || (!strcmp(argv[i], "--xptr")))
	    xptr++;
	else
#endif
	if ((!strcmp(argv[i], "-debug")) || (!strcmp(argv[i], "--debug")))
	    debug++;
	else if ((!strcmp(argv[i], "-valid")) || (!strcmp(argv[i], "--valid")))
	    valid++;
	else if ((!strcmp(argv[i], "-expr")) || (!strcmp(argv[i], "--expr")))
	    expr++;
	else if ((!strcmp(argv[i], "-tree")) || (!strcmp(argv[i], "--tree")))
	    tree++;
	else if ((!strcmp(argv[i], "-nocdata")) ||
		 (!strcmp(argv[i], "--nocdata")))
	    nocdata++;
	else if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "--input")))
	    filename = argv[++i];
	else if ((!strcmp(argv[i], "-f")) || (!strcmp(argv[i], "--file")))
	    usefile++;
    }
    if (valid != 0) xmlDoValidityCheckingDefaultValue = 1;
    xmlLoadExtDtdDefaultValue |= XML_DETECT_IDS;
    xmlLoadExtDtdDefaultValue |= XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefaultValue = 1;
#ifdef LIBXML_SAX1_ENABLED
    if (nocdata != 0) {
	xmlDefaultSAXHandlerInit();
	xmlDefaultSAXHandler.cdataBlock = NULL;
    }
#endif
    if (document == NULL) {
        if (filename == NULL)
	    document = xmlReadDoc(buffer,NULL,NULL,XML_PARSE_COMPACT);
	else
	    // document = xmlReadFile(filename,NULL,XML_PARSE_COMPACT);
        document = htmlReadFile(filename, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
        if (document == NULL) {
            fprintf(stderr, "Document not parsed successfully. \n");
        }
    }
    
    for (i = 1; i < argc ; i++) {
	if ((!strcmp(argv[i], "-i")) || (!strcmp(argv[i], "--input"))) {
	    i++; continue;
	}
	if (argv[i][0] != '-') {
	    if (usefile)
	        testXPathFile(argv[i]);
	    else
		testXPath(argv[i]);
	    strings ++;
	}
    }
    if (strings == 0) {
	printf("Usage : %s [--debug] [--copy] stringsorfiles ...\n",
	       argv[0]);
	printf("\tParse the XPath strings and output the result of the parsing\n");
	printf("\t--debug : dump a debug version of the result\n");
	printf("\t--valid : switch on DTD support in the parser\n");
#if defined(LIBXML_XPTR_ENABLED)
	printf("\t--xptr : expressions are XPointer expressions\n");
#endif
	printf("\t--expr : debug XPath expressions only\n");
	printf("\t--tree : show the compiled XPath tree\n");
	printf("\t--nocdata : do not generate CDATA nodes\n");
	printf("\t--input filename : or\n");
	printf("\t-i filename      : read the document from filename\n");
	printf("\t--file : or\n");
	printf("\t-f     : read queries from files, args\n");
    }
    if (document != NULL)
	xmlFreeDoc(document);
    xmlCleanupParser();
    xmlMemoryDump();

    return(0);
}
#else
#include <stdio.h>
int main(int argc ATTRIBUTE_UNUSED, char **argv ATTRIBUTE_UNUSED) {
    printf("%s : XPath/Debug support not compiled in\n", argv[0]);
    return(0);
}
#endif /* LIBXML_XPATH_ENABLED */
