#include "univ/xml.h"
#include "univ/str.h"
#include "univ/vec.h"

#define IsNameStart(ch) (IsAlpha(ch) || (ch) == ':' || (ch) == '_')
#define IsNameChar(ch)  (IsNameStart(ch) || (ch) == '-' || (ch) == '.' || IsDigit(ch))

static char *ParseName(char **str)
{
  char *cur = *str;
  char *name;
  while (IsNameChar(*cur)) cur++;
  name = StringFrom(*str, cur - *str);
  *str = cur;
  return name;
}

#define IsCharData(ch)  ((ch) && (ch) != '&' && (ch) != '<')

static char *ParseCharData(char **str, char *data, u32 dataLen)
{
  char *cur = *str;
  while (IsCharData(*cur)) cur++;

  if (cur > *str) {
    data = realloc(data, dataLen + (cur - *str) + 1);
    Copy(*str, data+dataLen, cur - *str);
    dataLen += cur - *str;
    data[dataLen] = 0;
  }

  if (Match("&amp;", cur)) {
    data = realloc(data, dataLen + 2);
    data[dataLen++] = '&';
    data[dataLen] = 0;
    cur += 5;
  } else if (Match("&lt;", cur)) {
    data = realloc(data, dataLen + 2);
    data[dataLen++] = '<';
    data[dataLen] = 0;
    cur += 4;
  } else if (Match("&quot;", cur)) {
    data = realloc(data, dataLen + 2);
    data[dataLen++] = '"';
    data[dataLen] = 0;
    cur += 6;
  } else if (Match("&apos;", cur)) {
    data = realloc(data, dataLen + 2);
    data[dataLen++] = '\'';
    data[dataLen] = 0;
    cur += 6;
  }
  if (cur == *str) return data;

  *str = cur;
  return ParseCharData(str, data, dataLen);
}

static void ParseProlog(char **str)
{
  if (!Match("<?xml", *str)) return;
  *str += 5;
  while (**str && **str != '>') (*str)++;
  if (**str == '>') (*str)++;
  SkipSpace(*str);
  if (!Match("<!DOCTYPE", *str)) return;
  *str += 9;
  while (**str && **str != '>') (*str)++;
  if (**str) (*str)++;
  SkipSpace(*str);
}

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

static bool ParseAttribute(char **str, XMLAttribute *attr)
{
  char *cur;
  attr->name = ParseName(str);
  if (!Match("=\"", *str)) {
    free(attr->name);
    return false;
  }
  *str += 2;
  cur = *str;
  while (*cur && *cur != '"') cur++;
  if (*cur != '"') {
    free(attr->name);
    return false;
  }

  attr->value = StringFrom(*str, cur - *str);
  *str = cur + 1;
  return true;
}

static XMLNode *ParseStartTag(char **str)
{
  XMLNode *node;
  if (!Match("<", *str)) return 0;
  (*str)++;
  node = NewNode();
  node->name = ParseName(str);
  SkipSpace(*str);
  while (**str != '>') {
    XMLAttribute attr;

    if (Match("/>", *str)) return node;

    if (!ParseAttribute(str, &attr)) {
      FreeXMLNode(node);
      return 0;
    }
    VecPush(node->attrs, attr);
    SkipSpace(*str);
  }

  if (**str != '>') {
    FreeXMLNode(node);
    return 0;
  }
  (*str)++;
  return node;
}

XMLNode *ParseXMLNode(char **str)
{
  XMLNode *node = ParseStartTag(str);
  if (!node) return 0;

  if (Match("/>", *str)) {
    *str += 2;
    return node;
  }

  SkipSpace(*str);
  while (!Match("</", *str)) {
    if (!*str) {
      FreeXMLNode(node);
      return 0;
    }

    if (Match("<", *str)) {
      XMLNode *child = ParseXMLNode(str);
      if (!child) {
        FreeXMLNode(node);
        return 0;
      }
      VecPush(node->children, child);
    } else {
      XMLNode *child = NewNode();
      child->textContent = ParseCharData(str, child->textContent, 0);
      VecPush(node->children, child);
    }

    SkipSpace(*str);
  }
  *str += 2;

  if (!Match(node->name, *str)) {
    FreeXMLNode(node);
    return 0;
  }

  *str += StrLen(node->name);

  if (!Match(">", *str)) {
    FreeXMLNode(node);
    return 0;
  }
  *str += 1;

  return node;
}

XMLNode *ParseXML(char *str)
{
  SkipSpace(str);
  ParseProlog(&str);
  return ParseXMLNode(&str);
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
