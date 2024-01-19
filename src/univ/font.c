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

#elif PLATFORM == Linux

#include <fontconfig/fontconfig.h>

ObjVec *ListFonts(void)
{
  ObjVec *fonts = Alloc(sizeof(ObjVec));

  i32 i;
  FcConfig *config = FcInitLoadConfigAndFonts();
  FcPattern *pattern = FcPatternCreate();
  FcObjectSet *set = FcObjectSetBuild(FC_FAMILY, FC_FILE, (char*)0);
  FcFontSet *font_set = FcFontList(config, pattern, set);

  InitVec(fonts, sizeof(char*), 0);

  for (i = 0; font_set && i < font_set->nfont; i++) {
    FcPattern *font = font_set->fonts[i];
    FcChar8 *path, *name;
    if (FcPatternGetString(font, FC_FILE, 0, &path) == FcResultMatch &&
        FcPatternGetString(font, FC_FAMILY, 0, &name) == FcResultMatch) {

      FontInfo *info = Alloc(sizeof(FontInfo));
      info->name = (char*)name;
      info->path = (char*)path;
    }
  }
  if (font_set) FcFontSetDestroy(font_set);

  return fonts;
}


char *DefaultFont(void)
{
  FcConfig *config = FcInitLoadConfigAndFonts();
  FcPattern *pattern = FcPatternCreate();
  FcPattern *font;
  FcResult result;
  FcChar8 *name;

  FcConfigSubstitute(config, pattern, FcMatchPattern);
  FcDefaultSubstitute(pattern);
  font = FcFontMatch(config, pattern, &result);

  if (FcPatternGetString(font, FC_FAMILY, 0, &name) != FcResultMatch) {
    return 0;
  }

  return (char*)name;
}

#else

ObjVec *ListFonts(void)
{
  ObjVec *fonts = Alloc(sizeof(ObjVec));
  InitVec(fonts, sizeof(char*), 0);
  return fonts;
}

char *DefaultFont(void)
{
  return 0;
}

#endif
