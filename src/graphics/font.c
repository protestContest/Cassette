#include "graphics/font.h"
#include "univ/hashmap.h"
#include "univ/math.h"
#include "univ/res.h"
#include "univ/vec.h"

#define propFont  (i16)0x9000
#define fixedFont (i16)0xB000
#define fontWid   (i16)0xACB0

typedef struct {
  i16 type;
  i16 first_char;
  i16 last_char;
  i16 wid_max;
  i16 kern_max;
  i16 neg_descent;
  i16 rect_width;
  i16 rect_height;
  i16 owt_offset;
  i16 ascent;
  i16 descent;
  i16 leading;
  i16 row_words;
} FontRec;

typedef struct {
  i16 flags;
  i16 famID;
  i16 first_char;
  i16 last_char;
  i16 ascent;
  i16 descent;
  i16 leading;
  i16 wid_max;
  i32 widths_offset;
  i32 kerns_offset;
  i32 styles_offset;
  i16 property[9];
  i16 intl[2];
  i16 version;
} FamRec;

typedef struct {
  i16 size;
  i16 style;
  u16 resID;
} FontAssoc;

static HashMap font_map = EmptyHashMap;
static FontRec **fonts = 0;

/* Swap all the bytes to reverse endianness */
static void FixFont(FontRec *rec)
{
  i32 i, table_size;
  i16 *bit_image, *loc_table, *ow_table;

  for (i = 0; i < 13; i++) {
    ((i16*)rec)[i] = ByteSwapShort(((i16*)rec)[i]);
  }

  table_size = rec->last_char - rec->first_char + 2;
  bit_image = (i16*)(rec + 1);
  loc_table = bit_image + rec->row_words*rec->rect_height;
  ow_table = &rec->owt_offset + rec->owt_offset;

  for (i = 0; i < rec->row_words*rec->rect_height; i++) {
    bit_image[i] = ByteSwapShort(bit_image[i]);
  }
  for (i = 0; i < table_size+1; i++) {
    loc_table[i] = ByteSwapShort(loc_table[i]);
  }
  for (i = 0; i < table_size; i++) {
    ow_table[i] = ByteSwapShort(ow_table[i]);
  }
}

static FontRec *LoadFont(i16 font, i16 size)
{
  i16 resID = 128*font + size;
  u32 hash = Hash(&resID, sizeof(resID));
  u32 index;
  FontRec *rec;

  if (HashMapFetch(&font_map, hash, &index)) {
    return fonts[index];
  } else {
    rec = GetResource('FONT', resID);
    if (!rec) return 0;
    HashMapSet(&font_map, hash, VecCount(fonts));
    VecPush(fonts, rec);
    FixFont(rec);
    return rec;
  }
}

void DrawChar(char ch, Canvas *canvas)
{
  char str[2];
  str[0] = ch;
  str[1] = 0;
  DrawString(str, canvas);
}

static i32 DoString(char *str, FontRec *rec, Canvas *canvas)
{
  i16 *bit_image;
  i16 *loc_table;
  i16 *ow_table;
  i16 table_size;
  i16 ow;
  i16 font_type = rec->type & 0xFFFC;
  i16 x = canvas ? canvas->pen.h : 0;
  i16 y = canvas ? canvas->pen.v : 0;

  table_size = rec->last_char - rec->first_char + 2;
  bit_image = (i16*)(rec + 1);
  loc_table = bit_image + rec->row_words*rec->rect_height;
  ow_table = &rec->owt_offset + rec->owt_offset;

  while (*str) {
    u8 offset, width;
    i16 loc, img_width, index, first_word, last_word, leading_bits, trailing_bits;

    /* find the table index for this char; default to the missing char */
    index = table_size - 1;
    if (*str >= rec->first_char && *str <= rec->last_char) {
      index = *str - rec->first_char;
    }

    ow = ow_table[index];
    if (ow == -1) {
      index = table_size - 1;
      ow = ow_table[index];
    }

    width = ow & 0xFF;
    offset = ow >> 8;
    loc = loc_table[index];
    img_width = loc_table[index+1] - loc;
    offset += rec->kern_max;

    first_word = loc / 16;
    last_word = (loc+img_width-1) / 16;
    leading_bits = loc % 16;
    trailing_bits = 15 - (loc+img_width-1) % 16;

    if (rec->type != fontWid && canvas) {
      i32 bx, by, w;
      for (by = 0; by < rec->rect_height; by++) {
        i16 px = x - leading_bits + offset;
        i16 py = y - rec->ascent + by;
        for (w = first_word; w <= last_word; w++) {
          u16 word = bit_image[by*rec->row_words + w];
          if (w == first_word) word &= (0xFFFF >> leading_bits);
          if (w == last_word) word &= (0xFFFF << trailing_bits);
          if (!word) {
            px += 16;
            continue;
          }
          for (bx = 0; bx < 16; bx++) {
            if ((word & (0x8000 >> bx)) && InCanvas(canvas, px, py)) {
              WritePixel(px, py, canvas);
            }
            px++;
          }
        }
      }
    }

    if (font_type == propFont) {
      x += width;
    } else if (font_type == fixedFont) {
      x += rec->wid_max;
    }

    str++;
  }

  return x;
}

void DrawString(char *str, Canvas *canvas)
{
  FontRec *rec = LoadFont(canvas->text.font, canvas->text.size);
  if (!rec) return;
  canvas->pen.h = DoString(str, rec, canvas);
}

i32 StringWidth(char *str, Canvas *canvas)
{
  FontRec *rec = LoadFont(canvas->text.font, canvas->text.size);
  if (!rec) return 0;
  return DoString(str, rec, 0);
}

i16 GetFNum(char *name)
{
  FamRec *data = GetNamedResource('FOND', name);
  if (!data) return -1;
  return ByteSwapShort(data->famID);
}

void GetFontInfo(FontInfo *info, Canvas *canvas)
{
  FontRec *font = LoadFont(canvas->text.font, canvas->text.size);
  if (!font) return;
  info->ascent = font->ascent;
  info->descent = font->descent;
  info->widMax = font->wid_max;
  info->leading = font->leading;
}

static char *styles[] = {
  "bold",
  "italic",
  "uline",
  "outline",
  "shadow",
  "condense",
  "extend"
};

/* Prints the available fonts */
void ListFonts(void)
{
  i32 num_families = CountResources('FOND');
  i32 i, j;

  for (i = 0; i < num_families; i++) {
    char *name;
    i16 *assoc_table;
    i16 num_entries;
    FontAssoc *entries;
    FamRec *data = GetIndResource('FOND', i);
    if (!data) continue;

    GetResInfo(data, 0, 0, &name);
    if (name) printf("\"%s\" ", name);
    printf("(#%d):\n", ByteSwapShort(data->famID));

    assoc_table = (i16*)(data+1);
    num_entries = ByteSwapShort(assoc_table[0]) + 1;
    entries = (FontAssoc*)(assoc_table+1);

    for (j = 0; j < num_entries; j++) {
      i16 style = ByteSwapShort(entries[j].style);
      printf("  %s %d", name, ByteSwapShort(entries[j].size));
      if (style) {
        u32 b;
        for (b = 0; b < ArrayCount(styles); b++) {
          if (style & (1 << b)) printf(" %s", styles[b]);
        }
      }
      printf("\n");
    }
  }
}

/* dumps the font bitmap data into a canvas */
void DumpFont(Canvas *canvas)
{
  FontRec *rec = LoadFont(canvas->text.font, canvas->text.size);
  u16 *bit_image;
  i32 w, y, b;
  if (!rec) return;

  bit_image = (u16*)(rec + 1);

  for (y = 0; y < rec->rect_height; y++) {
    for (w = 0; w < rec->row_words; w++) {
      u16 word = bit_image[y*rec->row_words + w];
      if (!word) continue;
      for (b = 0; b < 16; b++) {
        if (word & (0x8000 >> b)) {
          i32 px = (w*16+b) % canvas->width;
          i32 py = y + (w*16+b) / canvas->width * rec->rect_height;
          WritePixel(px, py, canvas);
        }
      }
    }
  }
}
