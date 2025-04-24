#include "univ/http.h"
#include "univ/file.h"
#include "univ/str.h"

static char *FormatRequest(char *method, char *host, char *path, char *headers, char *body)
{
  u32 methodLen = StrLen(method);
  u32 pathLen = StrLen(path);
  u32 hostLen = StrLen(host);
  u32 headersLen = StrLen(headers);
  u32 bodyLen = StrLen(body);
  u32 reqLen = methodLen+pathLen+hostLen+headersLen+bodyLen+30;
  char *req = malloc(reqLen+1);
  char *cur = req;

  Copy(method, cur, methodLen);
  cur += methodLen;
  *cur = ' ';
  cur++;
  Copy(path, cur, pathLen);
  cur += pathLen;
  Copy(" HTTP/1.0\r\nHost: ", cur, 17);
  cur += 17;
  Copy(host, cur, hostLen);
  cur += hostLen;
  Copy("\r\n", cur, 2);
  cur += 2;
  Copy(headers, cur, headersLen);
  cur += headersLen;
  Copy("\r\n\r\n", cur, 4);
  cur += 4;
  Copy(body, cur, bodyLen);
  cur += bodyLen;
  *cur = 0;

  return req;
}

char *Request(char *method, char *host, char *path, char *headers, char *body)
{
  char *error = 0;
  i32 socket;
  char *request;
  char *response = 0;
  char *buf;
  u32 sizeRead;
  u32 totalSize = 0;

  socket = Connect(host, "80", &error);
  if (error) {
    printf("%s\n", error);
    return 0;
  }

  request = FormatRequest(method, host, path, headers, body);

  Write(socket, request, StrLen(request), &error);
  free(request);
  if (error) {
    printf("%s\n", error);
    return 0;
  }

  do {
    response = realloc(response, totalSize + 4096);
    buf = response + totalSize;
    sizeRead = Read(socket, buf, 4096, &error);
    totalSize += sizeRead;
  } while (!error && sizeRead > 0);

  Close(socket);

  if (error) {
    printf("%s\n", error);
    free(response);
    return 0;
  }

  response = realloc(response, totalSize + 1);
  response[totalSize] = 0;
  return response;
}
