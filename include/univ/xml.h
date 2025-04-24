#pragma once

typedef struct {
  char *name;
  char *value;
} XMLAttribute;

typedef struct XMLNode {
  char *name;
  char *textContent;
  struct XMLNode **children; /* vec */
  XMLAttribute *attrs; /* vec */
} XMLNode;

XMLNode *ParseXML(char *str);
void FreeXMLNode(XMLNode *node);
void PrintXMLNode(XMLNode *node);
