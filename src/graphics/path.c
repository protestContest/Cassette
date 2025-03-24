#include "graphics/path.h"
#include "univ/str.h"

static void SetOrigin(Point pt, Canvas *canvas)
{
  canvas->origin.h = pt.h;
  canvas->origin.v = pt.v;
}

static void SetControl(Canvas *canvas, Point pt)
{
  canvas->control.h = canvas->pen.h*2 - pt.h;
  canvas->control.v = canvas->pen.v*2 - pt.v;
}

static void SkipSpace(char **path)
{
  while (IsWhitespace(**path)) (*path)++;
}

static bool ParseCoord(char **path, i32 *coord)
{
  float num;

  if (!ParseFloat(path, &num)) return false;
  SkipSpace(path);
  *coord = (i32)num;
  return true;
}

static bool ParsePoint(char **path, Point *pt)
{
  i32 coord;

  if (!ParseCoord(path, &coord)) return false;
  pt->h = coord;
  SkipSpace(path);

  if (**path == ',') {
    (*path)++;
    SkipSpace(path);
  }

  if (!ParseCoord(path, &coord)) return false;
  pt->v = coord;
  SkipSpace(path);

  if (**path == ',') {
    (*path)++;
    SkipSpace(path);
  }

  return true;
}

static bool ParsePoints(char **path, Point *params, u32 num)
{
  u32 i;
  for (i = 0; i < num; i++) {
    if (!ParsePoint(path, &params[i])) return false;
  }
  return true;
}

static void AdjustPoints(Canvas *canvas, u32 num)
{
  u32 i;
  for (i = 0; i < num; i++) {
    canvas->params[i].h += canvas->pen.h;
    canvas->params[i].v += canvas->pen.v;
  }
}

typedef struct {
  Point r;
  i32 angle;
  bool large_arc;
  bool sweep;
  Point dst;
} ArcParams;

static bool ParseFlag(char **path, bool *flag)
{
  if (**path != '0' && **path != '1') return false;
  if (IsDigit((*path)[1])) return false;
  *flag = **path - '0';
  (*path)++;
  return true;
}

static bool ParseArcParams(char **path, ArcParams *params)
{
  if (!ParsePoint(path, &params->r)) return false;
  if (!ParseCoord(path, &params->angle)) return false;
  if (!ParseFlag(path, &params->large_arc)) return false;
  if (!ParseFlag(path, &params->sweep)) return false;
  if (!ParsePoint(path, &params->dst)) return false;
  return true;
}

char *DrawPath(char *path, Canvas *canvas)
{
  Point pt;
  i32 coord;
  char cmd;
  ArcParams arc;

  SkipSpace(&path);
  cmd = *path++;

  while (cmd) {
    SkipSpace(&path);

    switch (cmd) {
    case 'M':
      if (!ParsePoint(&path, &pt)) return path;
      SetOrigin(pt, canvas);
      MoveTo(pt.h, pt.v, canvas);
      while (ParsePoint(&path, &pt)) {
        LineTo(pt.h, pt.v, canvas);
      }
      SetControl(canvas, canvas->pen);
      break;

    case 'm':
      if (!ParsePoint(&path, &pt)) return path;
      pt.h += canvas->pen.h;
      pt.v += canvas->pen.v;
      SetOrigin(pt, canvas);
      MoveTo(pt.h, pt.v, canvas);
      while (ParsePoint(&path, &pt)) {
        Line(pt.h, pt.v, canvas);
      }
      SetControl(canvas, canvas->pen);
      break;

    case 'L':
      if (!ParsePoint(&path, &pt)) return path;
      do {
        LineTo(pt.h, pt.v, canvas);
      } while (ParsePoint(&path, &pt));
      SetControl(canvas, canvas->pen);
      break;

    case 'l':
      if (!ParsePoint(&path, &pt)) return path;
      do {
        Line(pt.h, pt.v, canvas);
      } while (ParsePoint(&path, &pt));
      SetControl(canvas, canvas->pen);
      break;

    case 'H':
      if (!ParseCoord(&path, &coord)) return path;
      do {
        LineTo(coord, canvas->pen.v, canvas);
      } while (ParseCoord(&path, &coord));
      SetControl(canvas, canvas->pen);
      break;

    case 'h':
      if (!ParseCoord(&path, &coord)) return path;
      do {
        Line(coord, 0, canvas);
      } while (ParseCoord(&path, &coord));
      SetControl(canvas, canvas->pen);
      break;

    case 'V':
      if (!ParseCoord(&path, &coord)) return path;
      do {
        LineTo(canvas->pen.h, coord, canvas);
      } while (ParseCoord(&path, &coord));
      SetControl(canvas, canvas->pen);
      break;

    case 'v':
      if (!ParseCoord(&path, &coord)) return path;
      do {
        Line(0, coord, canvas);
      } while (ParseCoord(&path, &coord));
      SetControl(canvas, canvas->pen);
      break;

    case 'C':
      if (!ParsePoints(&path, canvas->params, 3)) return path;
      do {
        CubicBezier(canvas->params[0].h, canvas->params[0].v,
                    canvas->params[1].h, canvas->params[1].v,
                    canvas->params[2].h, canvas->params[2].v,
                    canvas);
        SetControl(canvas, canvas->params[1]);
      } while (ParsePoints(&path, canvas->params, 3));
      break;

    case 'c':
      if (!ParsePoints(&path, canvas->params, 3)) return path;
      do {
        AdjustPoints(canvas, 3);
        CubicBezier(canvas->pen.h + canvas->params[0].h, canvas->pen.v + canvas->params[0].v,
                    canvas->pen.h + canvas->params[1].h, canvas->pen.v + canvas->params[1].v,
                    canvas->pen.h + canvas->params[2].h, canvas->pen.v + canvas->params[2].v,
                    canvas);
        SetControl(canvas, canvas->params[1]);
      } while (ParsePoints(&path, canvas->params, 3));
      break;

    case 'S':
      if (!ParsePoints(&path, canvas->params, 2)) return path;
      do {
        CubicBezier(canvas->control.h, canvas->control.v,
                    canvas->params[0].h, canvas->params[0].v,
                    canvas->params[1].h, canvas->params[1].v,
                    canvas);
        SetControl(canvas, canvas->params[0]);
      } while (ParsePoints(&path, canvas->params, 2));
      break;

    case 's':
      if (!ParsePoints(&path, canvas->params, 2)) return path;
      do {
        AdjustPoints(canvas, 2);
        CubicBezier(canvas->control.h, canvas->control.v,
                    canvas->params[0].h, canvas->params[0].v,
                    canvas->params[1].h, canvas->params[1].v,
                    canvas);
        SetControl(canvas, canvas->params[0]);
      } while (ParsePoints(&path, canvas->params, 2));
      break;

    case 'Q':
      if (!ParsePoints(&path, canvas->params, 2)) return path;
      do {
        QuadBezier(canvas->params[0].h, canvas->params[0].v,
                   canvas->params[1].h, canvas->params[1].v,
                   canvas);
        SetControl(canvas, canvas->params[0]);
      } while (ParsePoints(&path, canvas->params, 2));
      break;

    case 'q':
      if (!ParsePoints(&path, canvas->params, 2)) return path;
      do {
        AdjustPoints(canvas, 2);
        QuadBezier(canvas->params[0].h, canvas->params[0].v,
                   canvas->params[1].h, canvas->params[1].v,
                   canvas);
        SetControl(canvas, canvas->params[0]);
      } while (ParsePoints(&path, canvas->params, 2));
      break;

    case 'T':
      if (!ParsePoint(&path, &pt)) return path;
      do {
        QuadBezier(canvas->control.h, canvas->control.v, pt.h, pt.v, canvas);
        SetControl(canvas, canvas->control);
      } while (ParsePoint(&path, &pt));
      break;

    case 't':
      if (!ParsePoint(&path, &pt)) return path;
      do {
        pt.h += canvas->pen.h;
        pt.v += canvas->pen.v;
        QuadBezier(canvas->control.h, canvas->control.v, pt.h, pt.v, canvas);
        SetControl(canvas, canvas->control);
      } while (ParsePoint(&path, &pt));
      break;

    case 'A':
      if (!ParseArcParams(&path, &arc)) return path;
      do {
        /* TODO: draw arc */
        MoveTo(arc.dst.h, arc.dst.v, canvas);
        SetControl(canvas, canvas->pen);
      } while (ParseArcParams(&path, &arc));
      break;

    case 'a':
      if (!ParseArcParams(&path, &arc)) return path;
      do {
        arc.dst.h += canvas->pen.h;
        arc.dst.v += canvas->pen.v;
        /* TODO: draw arc */
        MoveTo(arc.dst.h, arc.dst.v, canvas);
        SetControl(canvas, canvas->pen);
      } while (ParseArcParams(&path, &arc));
      break;

    case 'Z':
    case 'z':
      LineTo(canvas->origin.h, canvas->origin.v, canvas);
      SetControl(canvas, canvas->pen);
      break;
    }

    cmd = *path++;
  }
  return path;
}
