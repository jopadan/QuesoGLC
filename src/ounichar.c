/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002-2004, Bertrand Coconnier
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$ */

/* Defines the methods of an object that is intended to managed Unicode
 * characters. GLC is designed to support large coded character set defined
 * by the standards ISO/IEC 10646-1:1993 and Unicode 1.1
 */

#include <stdio.h>
#include "internal.h"
#include "ounichar.h"


typedef union {
  unsigned char *ucs1;
  unsigned short *ucs2;
  unsigned int *ucs4;
} uniChar;

GLCchar* __glcUniDup(__glcUniChar *This, GLCchar* dest, size_t n)
{ 
  return memmove(dest, This->ptr, 
		 __glcUniLenBytes(This) > n ? n : __glcUniLenBytes(This));
}

/* Length of the string in characters */
size_t __glcUniLen(__glcUniChar *This)
{
  uniChar c;
  size_t length = 0;

  if (!This->ptr)
    return 0;

  c.ucs1 = (unsigned char*)This->ptr;

  switch(This->type) {
  case GLC_UCS1:
    while (*(c.ucs1)) {
      c.ucs1++;
      length++;
    }
    return length;
  case GLC_UCS2:
    while (*(c.ucs2)) {
      c.ucs2++;
      length++;
    }
    return length;
  case GLC_UCS4:
    while (*(c.ucs4)) {
      c.ucs4++;
      length++;
    }
    return length;
  default:
    return 0;
  }

  return length;
}

/* Length of the string in bytes (including the NULL termination) */
size_t __glcUniLenBytes(__glcUniChar *This)
{
  if (!This->ptr)
    return 0;

  switch(This->type) {
  case GLC_UCS1:
    return __glcUniLen(This) + 1;
  case GLC_UCS2:
    return 2 * (__glcUniLen(This) + 1);
  case GLC_UCS4:
    return 4 * (__glcUniLen(This) + 1);
  default:
    return 0;
  }
}

static int lenType(int inType)
{
  switch(inType) {
  case GLC_UCS1:
    return sizeof(unsigned char);
  case GLC_UCS2:
    return sizeof(unsigned short);
  case GLC_UCS4:
    return sizeof(unsigned int);
  default:
    return 0;
  }
}

/* Compares two unicode strings. The API is based upon strcmp
 * Internally it converts both strings to GLC_UCS4 then compare
 */
int __glcUniCompare(__glcUniChar *This, __glcUniChar* inString)
{
  GLCchar *s1 = NULL, *s2 = NULL;
  uniChar c1 , c2;
  int l1 = 0, l2 = 0;
  int ret = 0;

  /* Reserve room to store the converted strings */
  l1 = __glcUniLenBytes(This) * 4 / lenType(This->type);
  s1 = (GLCchar*)__glcMalloc(l1);
  if (!s1)
    return 0; /* 0 means that the strings are equal : may be we should use another value ?? */
  l2 = __glcUniLenBytes(inString) * 4 / lenType(inString->type);
  s2 = (GLCchar*)__glcMalloc(l2);
  if (!s2) {
    __glcFree(s1);
    return 0; /* 0 means that strings are equal : may be we should use another value ?? */
  }

  /* Process the conversion */
  __glcUniConvert(This, s1, GLC_UCS4, l1);
  __glcUniConvert(inString, s2, GLC_UCS4, l2);

  /* Compare */
  c1.ucs1 = (unsigned char*) s1;
  c2.ucs1 = (unsigned char*) s2;

  while(*(c1.ucs4)) {
    if (*(c1.ucs4) != *(c2.ucs4))
      break;
    c1.ucs4++;
    c2.ucs4++;
  }
  if (*(c1.ucs4) < *(c2.ucs4))
    ret = -1;
  if (*(c1.ucs4) == *(c2.ucs4))
    ret = 0;
  if (*(c1.ucs4) > *(c2.ucs4))
    ret = 1;

  __glcFree(s1);
  __glcFree(s2);
  return ret;
}

#define CONVERT_UP(LOW, TYPE_LOW, UP, TYPE_UP) \
do { \
   c1.ucs##LOW = (TYPE_LOW*)This->ptr; \
   c2.ucs##UP = (TYPE_UP*)dest; \
   for(i = 0; i < __glcUniLen(This) + 1; i++) { \
     *(c2.ucs##UP) = *(c1.ucs##LOW); \
     c1.ucs##LOW++; \
     c2.ucs##UP++; \
     length++; \
     if (length * charSize >= n) \
       break; \
   } \
} while (0)

#define CONVERT_DOWN(LOW, TYPE_LOW, SIZE_LOW, UP, TYPE_UP) \
do { \
  c1.ucs##UP = (TYPE_UP*)This->ptr; \
  c2.ucs##LOW = (TYPE_LOW*) dest; \
  for (i = 0; i < __glcUniLen(This) + 1; i++) { \
    if (*(c1.ucs##UP) < SIZE_LOW) { \
      *(c2.ucs##LOW) = *(c1.ucs##LOW); \
      c2.ucs##LOW++; \
      length++; \
      if (length * charSize >= n) \
	return dest; \
    } \
    else { \
      sprintf(buffer, "\\<%X>", *(c1.ucs##UP)); \
      for (j = 0; j < strlen(buffer); j++) { \
	*(c2.ucs##LOW) = buffer[j]; \
	c2.ucs##LOW++; \
	length++; \
	if (length * charSize >= n) \
	  return dest; \
      } \
    } \
    c1.ucs##UP++; \
  } \
} while(0)

/* Converts a Unicode String from one format to another */
GLCchar* __glcUniConvert(__glcUniChar *This, GLCchar* dest, int inType,
			 size_t n)
{
  uniChar c1, c2;
  size_t length = 0;
  int charSize = lenType(inType);
  char buffer[GLC_STRING_CHUNK];
  unsigned int i = 0, j = 0;

  if (inType == This->type)
    return __glcUniDup(This, dest, n);

  if (inType > This->type) {
    switch(This->type) {
    case GLC_UCS1:
      if (inType == GLC_UCS2)
	CONVERT_UP(1, unsigned char, 2, unsigned short);
      else
	CONVERT_UP(1, unsigned char, 4, unsigned int);
      return dest;
    case GLC_UCS2:
      CONVERT_UP(2, unsigned short, 4, unsigned int);
      return dest;
    default:
      return NULL;
    }
  }
  else {
    switch(This->type) {
    case GLC_UCS2:
      CONVERT_DOWN(1, unsigned char, 256, 2, unsigned short);
      break;
    case GLC_UCS4:
      if (inType == GLC_UCS1)
	CONVERT_DOWN(1, unsigned char, 256, 4, unsigned int);
      else
	CONVERT_DOWN(2, unsigned short, 65536, 4, unsigned int);
      break;
    default:
      return NULL;
    }
    return dest;
  }
}

/* Estimates the length (in bytes including the NULL termination) of the
 * unicode string after conversion.
 */
int __glcUniEstimate(__glcUniChar *This, int inType)
{
  int length = 0;
  size_t charSize = (size_t) lenType(inType);
  unsigned int maxSize = 1;
  unsigned int i = 0;
  uniChar c;
  char buffer[256];

  if (inType >= This->type)
    return __glcUniLenBytes(This) * charSize / lenType(This->type);

  c.ucs1 = (unsigned char *)This->ptr;

  for (i = 0; i < charSize * 8; i++)
    maxSize *= 2;

  for (i = 0; i < __glcUniLen(This) + 1; i++)
    switch(This->type) {
    case GLC_UCS2:
      if (*(c.ucs2) >= maxSize) {
	sprintf(buffer, "\\<%X>", *(c.ucs2));
	length += strlen(buffer) * charSize;
      }
      else
	length += charSize;
      c.ucs2++;
      break;
    case GLC_UCS4:
      if (*(c.ucs4) >= maxSize) {
	sprintf(buffer, "\\<%X>", *(c.ucs4));
	length += strlen(buffer) * charSize;
      }
      else
	length += charSize;
      c.ucs4++;
      break;
    default:
      return 0;
    }
  return length;
}

void __glcUniDestroy(__glcUniChar *This)
{
  if (!This->ptr)
    __glcFree(This->ptr);
  This->ptr = NULL;
}

GLuint __glcUniIndex(__glcUniChar *This, int inPos)
{
  uniChar c;

  c.ucs1 = (unsigned char*)This->ptr;

  switch(This->type) {
  case GLC_UCS1:
    c.ucs1 += inPos;
    return (GLuint) (*(c.ucs1));
  case GLC_UCS2:
    c.ucs2 += inPos;
    return (GLuint) (*(c.ucs2));
  case GLC_UCS4:
    c.ucs4 += inPos;
    return (GLuint) (*(c.ucs4));
  default:
    return 0;
  }
}
