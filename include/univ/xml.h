#pragma once

typedef struct {
  char *name;
  char *value;
} XMLAttribute;

typedef struct XMLNode {
  enum {xmlNode, xmlText} type;
  char *name;
  char *textContent;
  XMLAttribute *attrs; /* vec */
  struct XMLNode **children; /* vec */
} XMLNode;

XMLNode *ParseXML(char *str);
void FreeXMLNode(XMLNode *node);
void PrintXMLNode(XMLNode *node);
