/*
    Utility routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include "util.h"

wchar_t *wstrchr (wchar_t *s, wchar_t c)
{
  while (*s && *s != c)
    s++;
  return s;
}

int wstrlen (wchar_t *s)
{
  wchar_t *os = s;
  while (*s++)
    ;
  return s - os;
}

static wchar_t SourcePath[200];

void
preparePath(HINSTANCE hInst)
{
    // Locate the directory containing the haret executable.
    GetModuleFileName(hInst, SourcePath
                      , sizeof (SourcePath) / sizeof (wchar_t));
    wchar_t *x = wstrchr(SourcePath, 0);
    while ((x > SourcePath) && (x [-1] != L'\\'))
        x--;
    *x = 0;
}

void fnprepare (const char *ifn, char *ofn, int ofn_max)
{
  char *out = ofn;

  // Don't translate absolute file names
  if (ifn [0] != '\\')
  {
    BOOL flag;
    wchar_t *x = wstrchr (SourcePath, 0);
    int sl = x - SourcePath;
    out = ofn + WideCharToMultiByte (CP_ACP, 0, SourcePath, sl, ofn, ofn_max,
      " ", &flag);
  }

  strncpy (out, ifn, ofn_max - (out - ofn));
}

char *strnew (const char *s)
{
  if (!s)
    return NULL;

  size_t sl = strlen (s) + 1;
  char *ns = new char [sl];
  memcpy (ns, s, sl);
  return ns;
}

GarbageCollector::GarbageCollector ()
{
  Pointers = (void **)malloc ((Max = 16) * sizeof (void *));
  Count = 0;
}

GarbageCollector::~GarbageCollector ()
{
  FreeAll ();
  free (Pointers);
}

void GarbageCollector::Collect (void *p)
{
  if (Count > Max)
  {
    Max += 16;
    Pointers = (void **)realloc (Pointers, Max * sizeof (void *));
  }
  Pointers [Count++] = p;
}

void GarbageCollector::FreeAll ()
{
  for (uint i = 0; i < Count; i++)
    free (Pointers [i]);
  Count = 0;
}
