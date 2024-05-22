extern "C" {
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
}

#include <string_view>
#include <unordered_map>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <chrono>

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

void traverse_descendant_or_self(xmlNodePtr curr, const std::function<void(xmlNodePtr)>& fn) {
    fn(curr);
    for (xmlNodePtr n = curr->children; n != NULL; n = n->next) {
        traverse_descendant_or_self(n, fn);
    }
}

void traverse_attributes(xmlNodePtr curr, const std::function<void(xmlAttrPtr)>& fn) {
    for (xmlAttrPtr n = curr->properties; n != NULL; n = n->next) {
        fn(n);
    }
}

xmlXPathObjectPtr instanceOptimalXPathEval(xmlXPathCompExprPtr comp, xmlXPathContextPtr ctx) {
    auto curr = ctx->node;
    std::vector<xmlNodePtr> selected;
    traverse_descendant_or_self(curr, [&](xmlNodePtr n) {
        std::unordered_map<std::string, xmlNodePtr> attributes;
        traverse_attributes(n, [&](xmlAttrPtr a) {
            const char* key = (const char*) a->name;
            xmlNodePtr value = a->children;
            attributes.emplace(std::string(key), value);
        });
        auto class_it = attributes.find("class");
        if (class_it != attributes.end()) {
            xmlNodePtr v = class_it->second;
            std::string class_val((const char*) xmlNodeGetContent(v));
            if (class_val == "comments-title" 
                || class_val.contains("comments-title")
                || class_val.contains("nocomments")
                || class_val.contains("-reply-")
                || class_val.contains("message")
                || class_val.contains("akismet")
                || class_val.contains("suggest-links")
                || class_val.contains("-hide-")
                || class_val.contains("hide-print")
                || class_val.contains(" hidden")
                || class_val.contains("noprint")
                || class_val.contains("notloaded")
                || class_val.starts_with("reply-")
                || class_val.starts_with("hide-")) 
            {
                    selected.push_back(n);
                    return;
            }
        }
        {
            auto it = attributes.find("id");
            if (it != attributes.end()) {
                xmlNodePtr v = it->second;
                std::string val((const char*) xmlNodeGetContent(v));
                if (val.contains("akismet")
                    || val.contains("reader-comments")
                    || val.contains("hidden")
                    || val.starts_with("reply-")) 
                {
                    selected.push_back(n);
                    return;
                }
            }
        }
        {
            auto it = attributes.find("style");
            if (it != attributes.end()) {
                xmlNodePtr v = it->second;
                std::string val((const char*) xmlNodeGetContent(v));
                if (val.contains("hidden")
                    || val.contains("display:none")
                    || val.contains("display: none")) 
                {
                    selected.push_back(n);
                    return;
                }
            }
        }
        {
            auto it = attributes.find("aria-hidden");
            if (it != attributes.end()) {
                xmlNodePtr v = it->second;
                std::string val((const char*) xmlNodeGetContent(v));
                if (val == "true")
                {
                    selected.push_back(n);
                    return;
                }
            }
        }
    });
    if (selected.size() > 0) {
        xmlNodeSetPtr nodeSet = xmlXPathNodeSetCreate(selected[0]);
        for (int i=1; i<selected.size(); ++i) {
            xmlXPathNodeSetAdd(nodeSet, selected[i]);
        }
        xmlXPathNodeSetSort(nodeSet);
        xmlXPathObjectPtr res = xmlXPathNewNodeSetList(nodeSet);
        return res;
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    auto doc = htmlReadFile("CC-MAIN-20230606214755-20230607004755-00000_300697_340.html", "utf-8", HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
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
    xmlXPathDebugDumpCompExpr(stdout, comp, 0);
    xmlXPathObjectPtr res;
    std::vector<uint64_t> times_a;
    std::vector<uint64_t> times_b;
    for (int i=0; i<10; ++i) {
        uint64_t beginTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        res = xmlXPathCompiledEval(comp, ctxt);
        uint64_t endTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        times_a.push_back(endTimeUs - beginTimeUs);
    }
    xmlXPathDebugDumpObject(stdout, res, 0);
    printf("=======================\n");
    res = nullptr;
    for (int i=0; i<10; ++i) {
        uint64_t beginTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        res = instanceOptimalXPathEval(comp, ctxt);
        uint64_t endTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        times_b.push_back(endTimeUs - beginTimeUs);
    }
    xmlXPathDebugDumpObject(stdout, res, 0);
    // show times_a and times_b
    for (int i=0; i<10; ++i) {
        printf("times_a[%d]: %lu\n", i, times_a[i]);
    }
    for (int i=0; i<10; ++i) {
        printf("times_b[%d]: %lu\n", i, times_b[i]);
    }
    return 0;
}
