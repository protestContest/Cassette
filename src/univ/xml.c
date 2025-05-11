#include "univ/xml.h"
#include "univ/peg.h"
#include "univ/str.h"
#include "univ/vec.h"

static XMLAttribute *NewAttr(void)
{
  XMLAttribute *attr = malloc(sizeof(XMLAttribute));
  attr->name = 0;
  attr->value = 0;
  return attr;
}

static void DestroyAttr(XMLAttribute *attr)
{
  if (attr->name) free(attr->name);
  if (attr->value) free(attr->value);
}

static XMLNode *NewNode(void)
{
  XMLNode *node = malloc(sizeof(XMLNode));
  node->type = xmlNode;
  node->name = 0;
  node->textContent = 0;
  node->children = 0;
  node->attrs = 0;
  return node;
}

void FreeXMLNode(XMLNode *node)
{
  u32 i;
  if (node->name) free(node->name);
  if (node->textContent) free(node->textContent);
  for (i = 0; i < VecCount(node->attrs); i++) {
    DestroyAttr(&node->attrs[i]);
  }
  FreeVec(node->attrs);
  for (i = 0; i < VecCount(node->children); i++) {
    FreeXMLNode(node->children[i]);
  }
  FreeVec(node->children);
  free(node);
}

PNode *DocumentRule(PNode *node)
{
  PNode *element = node->elements[0];
  node->value.data = element->value.data;
  return node;
}

PNode *TagRule(PNode *node)
{
  XMLNode *xmlNode = NewNode();
  u32 i;
  xmlNode->name = node->elements[0]->value.data;
  for (i = 1; i < VecCount(node->elements); i++) {
    XMLAttribute attr;
    PNode *element = node->elements[i];
    attr.name = element->elements[0]->value.data;
    attr.value = element->elements[1]->value.data;
    VecPush(xmlNode->attrs, attr);
  }
  node->value.data = xmlNode;
  return node;
}

PNode *ContentRule(PNode *node)
{
  return node;
}

PNode *ElementRule(PNode *node)
{
  PNode *tag = node->elements[0];
  PNode *content;
  XMLNode *xmlNode = tag->value.data;
  u32 i;

  if (VecCount(node->elements) > 1) {
    content = node->elements[1];
    for (i = 0; i < VecCount(content->elements); i++) {
      XMLNode *child = content->elements[i]->value.data;
      if (child) VecPush(xmlNode->children, child);
    }
  }

  node->value.data = xmlNode;
  return node;
}

PNode *AttValueRule(PNode *node)
{
  char *text = malloc(node->length - 1);
  Copy(node->lexeme + 1, text, node->length - 2);
  text[node->length - 2] = 0;
  node->value.data = text;
  return node;
}

PNode *CharDataRule(PNode *node)
{
  u32 length = node->length;
  char *lexeme = node->lexeme;
  while (length > 0 && IsWhitespace(*lexeme)) {
    length--;
    lexeme++;
  }
  while (length > 0 && IsWhitespace(*(lexeme+length-1))) length--;

  if (length > 0) {
    XMLNode *xmlNode = NewNode();
    char *text = malloc(length+1);
    Copy(lexeme, text, length);
    text[length] = 0;
    xmlNode->textContent = text;
    xmlNode->type = xmlText;
    node->value.data = xmlNode;
  }
  return node;
}

Grammar *XMLGrammar(void)
{
  Grammar *g = NewGrammar();
  AddRule("document", "prolog element Misc*", DocumentRule, g);
  AddRule("Char", "[\\t\\n\\r\\20-\\FF]", DiscardNode, g);
  AddRule("S", "[ \\n\\t\\r]+", DiscardNode, g);
  AddRule("NameStartChar", "[:A-Za-z_\\C0-\\D6\\D8-\\F6\\F8-\\FF]", 0, g);
  AddRule("NameChar", "NameStartChar / [-.0-9\\B7]", 0, g);
  AddRule("Name", "NameStartChar NameChar*", TextNode, g);
  AddRule("Names", "Name (' ' Name)*", DiscardNode, g);
  AddRule("Nmtoken", "NameChar+", DiscardNode, g);
  AddRule("Nmtokens", "Nmtoken (' ' Nmtoken)*", DiscardNode, g);
  AddRule("EntityValue", "'\"' ((![%&\"] .) / PEReference / Reference)* '\"' / \"'\" ((![%&'] .) / PEReference / Reference)* \"'\"", DiscardNode, g);
  AddRule("AttValue", "'\"' ((![<&\"] .) / Reference)* '\"' / \"'\" ((![<&\"] .) / Reference)* \"'\"", AttValueRule, g);
  AddRule("SystemLiteral", "('\"' (!'\"' .)* '\"') / (\"'\" (!\"'\" . )* \"'\")", DiscardNode, g);
  AddRule("PubidLiteral", "'\"' PubidChar* '\"' / \"'\" (!\"'\" PubidChar)* \"'\"", DiscardNode, g);
  AddRule("PubidChar", "[- \\n\\r\\ta-zA-Z0-9'()+,./:=?;!*#@$_%]", DiscardNode, g);
  AddRule("CharData", "!((![<&] .)* ']]>' (![<&] .)*) (![<&] .)*", CharDataRule, g);
  AddRule("Comment", "'<!--' ((!'-' Char) / ('-' !'-' Char))* '-->'", DiscardNode, g);
  AddRule("PI", "'<?' PITarget (S (!(Char* '?>' Char*) Char*))? '?>'", DiscardNode, g);
  AddRule("PITarget", "!([Xx] [Mm] [Ll]) Name", DiscardNode, g);
  AddRule("CDSect", "'<![CDATA[' CData ']]>'", DiscardNode, g);
  AddRule("CData", "!(Char* ']]>' Char*) Char*", DiscardNode, g);
  AddRule("prolog", "XMLDecl? Misc* (doctypedecl Misc*)?", DiscardNode, g);
  AddRule("XMLDecl", "'<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>'", DiscardNode, g);
  AddRule("VersionInfo", "S 'version' Eq (\"'\" VersionNum \"'\" / '\"' VersionNum '\"')", DiscardNode, g);
  AddRule("Eq", "S? '=' S?", DiscardNode, g);
  AddRule("VersionNum", "'1.' [0-9]+", DiscardNode, g);
  AddRule("Misc", "Comment / PI / S", DiscardNode, g);
  AddRule("doctypedecl", "'<!DOCTYPE' S Name (S ExternalID)? S? ('[' intSubset ']' S?)? '>'", DiscardNode, g);
  AddRule("DeclSep", "PEReference / S", DiscardNode, g);
  AddRule("intSubset", "(markupdecl / DeclSep)*", DiscardNode, g);
  AddRule("markupdecl", "elementdecl / AttlistDecl / EntityDecl / NotationDecl / PI / Comment", DiscardNode, g);
  AddRule("extSubset", "TextDecl? extSubsetDecl", DiscardNode, g);
  AddRule("extSubsetDecl", "(markupdecl / conditionalSect / DeclSep)*", DiscardNode, g);
  AddRule("SDDecl", "S 'standalone' Eq ((\"'\" ('yes' / 'no') \"'\") / ('\"' ('yes' / 'no') '\"'))", DiscardNode, g);
  AddRule("element", "EmptyElemTag / STag content ETag", ElementRule, g);
  AddRule("STag", "'<' Name (S Attribute)* S? '>'", TagRule, g);
  AddRule("Attribute", "Name Eq AttValue", 0, g);
  AddRule("ETag", "'</' Name S? '>'", DiscardNode, g);
  AddRule("content", "CharData? ((element / Reference / CDSect / PI / Comment) CharData?)*", 0, g);
  AddRule("EmptyElemTag", "'<' Name (S Attribute)* S? '/>'", TagRule, g);
  AddRule("elementdecl", "'<!ELEMENT' S Name S contentspec S? '>'", DiscardNode, g);
  AddRule("contentspec", "'EMPTY' / 'ANY' / Mixed / children", DiscardNode, g);
  AddRule("children", "(choice / seq) [?*+]?", DiscardNode, g);
  AddRule("cp", "(Name / choice / seq) [?*+]?", DiscardNode, g);
  AddRule("choice", "'(' S? cp (S? '|' S? cp)+ S? ')'", DiscardNode, g);
  AddRule("seq", "'(' S? cp (S? ',' S? cp)+ S? ')'", DiscardNode, g);
  AddRule("Mixed", "'(' S? '#PCDATA' (S? '|' S? Name)* S? ')*' / '(' S? '#PCDATA' S? ')'", DiscardNode, g);
  AddRule("AttlistDecl", "'<!ATTLIST' S Name AttDef* S? '>'", DiscardNode, g);
  AddRule("AttDef", "S Name S AttType S DefaultDecl", DiscardNode, g);
  AddRule("AttType", "'CDATA' / TokenizedType / EnumeratedType", DiscardNode, g);
  AddRule("TokenizedType", "'ID' / 'IDREF' / 'IDREFS' / 'ENTITY' / 'ENTITIES' / 'NMTOKEN' / 'NMTOKENS'", DiscardNode, g);
  AddRule("EnumeratedType", "NotationType / Enumeration", DiscardNode, g);
  AddRule("NotationType", "'NOTATION' S '(' S? Name (S? '|' S? Name)* S? ')'", DiscardNode, g);
  AddRule("Enumeration", "'(' S? Nmtoken (S? '|' S? Nmtoken)* S? ')'", DiscardNode, g);
  AddRule("DefaultDecl", "'#REQUIRED' / '#IMPLIED' / (('#FIXED' S)? AttValue)", DiscardNode, g);
  AddRule("conditionalSect", "includeSect / ignoreSect", DiscardNode, g);
  AddRule("includeSect", "'<![' S? 'INCLUDE' S? '[' extSubsetDecl ']]>'", DiscardNode, g);
  AddRule("ignoreSect", "'<![' S? 'IGNORE' S? '[' ignoreSectContents* ']]>'", DiscardNode, g);
  AddRule("ignoreSectContents", "Ignore ('<![' ignoreSectContents ']]>' Ignore)*", DiscardNode, g);
  AddRule("Ignore", "!(Char* ('<![' / ']]>') Char*) Char*", DiscardNode, g);
  AddRule("CharRef", "'&#' [0-9]+ ';' / '&#x' [0-9a-fA-F]+ ';'", DiscardNode, g);
  AddRule("Reference", "EntityRef / CharRef", DiscardNode, g);
  AddRule("EntityRef", "'&' Name ';'", DiscardNode, g);
  AddRule("PEReference", "'%' Name ';'", DiscardNode, g);
  AddRule("EntityDecl", "GEDecl / PEDecl", DiscardNode, g);
  AddRule("GEDecl", "'<!ENTITY' S Name S EntityDef S? '>'", DiscardNode, g);
  AddRule("PEDecl", "'<!ENTITY' S '%' S Name S PEDef S? '>'", DiscardNode, g);
  AddRule("EntityDef", "EntityValue / (ExternalID NDataDecl?)", DiscardNode, g);
  AddRule("PEDef", "EntityValue / ExternalID", DiscardNode, g);
  AddRule("ExternalID", "'SYSTEM' S SystemLiteral / 'PUBLIC' S PubidLiteral S SystemLiteral", DiscardNode, g);
  AddRule("NDataDecl", "S 'NDATA' S Name", DiscardNode, g);
  AddRule("TextDecl", "'<?xml' VersionInfo? EncodingDecl S? '?>'", DiscardNode, g);
  AddRule("extParsedEnt", "TextDecl? content", DiscardNode, g);
  AddRule("EncodingDecl", "S 'encoding' Eq (('\"' EncName '\"') / (\"'\" EncName \"'\"))", DiscardNode, g);
  AddRule("EncName", "[A-Za-z][-A-Za-z0-9._]*", DiscardNode, g);
  AddRule("NotationDecl", "'<!NOTATION' S Name S (ExternalID / PublicID) S? '>'", DiscardNode, g);
  AddRule("PublicID", "'PUBLIC' S PubidLiteral", DiscardNode, g);
  return g;
}

XMLNode *ParseXML(char *str)
{
  XMLNode *result;
  u32 index = 0;
  PNode *node;
  Grammar *g;

  if (!str) return 0;

  g = XMLGrammar();

  node = ParseRule("document", str, &index, StrLen(str), g);
  if (!node) {
    fprintf(stderr, "Parse error at %d\n", index);
    return 0;
  }
  result = node->value.data;
  FreePNode(node);
  return result;
}

static void PrintXMLNodeLevel(XMLNode *node, u32 level)
{
  u32 i;
  for (i = 0; i < level; i++) printf("  ");

  if (node->textContent != 0) {
    printf("%s", node->textContent);
    return;
  }

  printf("<%s", node->name);
  for (i = 0; i < VecCount(node->attrs); i++) {
    printf(" %s=\"%s\"", node->attrs[i].name, node->attrs[i].value);
  }
  if (VecCount(node->children) == 0) {
    printf(" />");
    return;
  }
  printf(">");

  if (VecCount(node->children) == 1 && node->children[0]->textContent != 0) {
    PrintXMLNodeLevel(node->children[0], 0);
  } else {
    printf("\n");
    for (i = 0; i < VecCount(node->children); i++) {
      PrintXMLNodeLevel(node->children[i], level+1);
      printf("\n");
    }
    for (i = 0; i < level; i++) printf("  ");
  }

  printf("</%s>", node->name);
}

void PrintXMLNode(XMLNode *node)
{
  PrintXMLNodeLevel(node, 0);
}
