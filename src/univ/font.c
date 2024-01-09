#include "font.h"
#include "system.h"

#if PLATFORM == Apple

#include <CoreText/CoreText.h>

static char *StringFromCFString(CFStringRef cf_str)
{
  char *name;
  CFIndex length = CFStringGetLength(cf_str);
  name = Alloc(length + 1);
  CFStringGetCString(cf_str, name, length + 1, kCFStringEncodingUTF8);
  return name;
}

static char *FontPath(CFStringRef cf_name)
{
  CTFontRef font;
  CFURLRef url;
  CFStringRef cf_path;

  font = CTFontCreateWithName(cf_name, 0, 0);
  url = CTFontCopyAttribute(font, kCTFontURLAttribute);
  cf_path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
  return StringFromCFString(cf_path);
}

ObjVec *ListFonts(void)
{
  CFArrayRef ct_fonts;
  CFIndex i;
  ObjVec *fonts = Alloc(sizeof(ObjVec));
  InitVec(fonts, sizeof(char*), 8);

  ct_fonts = CTFontManagerCopyAvailableFontFamilyNames();

  for (i = 0; i < CFArrayGetCount(ct_fonts); i++) {
    FontInfo *info = Alloc(sizeof(FontInfo));

    CFStringRef name = CFArrayGetValueAtIndex(ct_fonts, i);
    info->name = StringFromCFString(name);
    info->path = FontPath(name);

    ObjVecPush(fonts, info);
  }

  return fonts;
}

char *DefaultFont(void)
{
  CTFontRef font = CTFontCreateUIFontForLanguage(kCTFontUIFontUser, 0, 0);
  CFStringRef name = CTFontCopyAttribute(font, kCTFontFamilyNameAttribute);
  return StringFromCFString(name);
}

#endif
