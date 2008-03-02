/* QuesoGLC
 * A free implementation of the OpenGL Character Renderer (GLC)
 * Copyright (c) 2002, 2004-2008, Bertrand Coconnier
 *
 * UTF-8 subroutines
 * Copyright (c) 2007-2008, Giel van Schijndel
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

/* This file defines miscellaneous utility routines used for Unicode management
 */

/** \file
 *  defines the routines used to manipulate Unicode strings and characters
 */

#include <fribidi/fribidi.h>

/* The maximum amount of octets this UTF-8 encoder/decoder can handle in a
 * single UTF-8 sequence.
 */
#define GLC_UTF8_MAX_LEN 7

#include "internal.h"



/** Check that non-starting bytes are of the form 10xxxxxx */
static inline int __glcIsNonStartByte(const GLCchar8 byte)
{
  return ((byte & 0xC0) == 0x80);
}



/** Check that starting bytes are either of the form 0xxxxxxx (ASCII) or
 *  11xxxxxx
 */
static inline int __glcIsStartByte(const GLCchar8 byte)
{
  return ((byte & 0x80) == 0x00      /* 0xxxxxxx */
	  || (byte & 0xC0) == 0xC0);    /* 11xxxxxx */
}



/* Determines the amount of unicode codepoints in a UTF-8 encoded string
 * \param utf8_string the UTF-8 encoded, and nul-terminated, string to count
 * \param[out] len the amount of codepoints found in the UTF-8 string
 * \return non-zero on success, zero when the given string isn't a valid UTF-8,
 *         string.
 */
static int __glcUtf8CharacterCount(const GLCchar8* inUtf8String, size_t* len)
{
  const GLCchar8* curChar = inUtf8String;

  size_t length = 0;
  while (*curChar != '\0') {
    if (!__glcIsStartByte(*curChar))
      return 0;

    /* first octect: 0xxxxxxx: 7 bit (ASCII) */
    if ((*curChar & 0x80) == 0x00) {
      /* 1 byte long encoding */
      curChar += 1;
    }
    /* first octect: 110xxxxx */
    else if ((*curChar & 0xe0) == 0xc0)	{
      /* 2 byte long encoding */
      if (!__glcIsNonStartByte(curChar[1]))
	return 0;

      curChar += 2;
    }
    /* first octect: 1110xxxx */
    else if ((*curChar & 0xf0) == 0xe0)	{
      /* 3 byte long encoding */
      if (!__glcIsNonStartByte(curChar[1])
	  || !__glcIsNonStartByte(curChar[2]))
	return 0;

      curChar += 3;
    }
    /* first octect: 11110xxx */
    else if ((*curChar & 0xf8) == 0xf0)	{
      /* 4 byte long encoding */
      if (!__glcIsNonStartByte(curChar[1])
	  || !__glcIsNonStartByte(curChar[2])
	  || !__glcIsNonStartByte(curChar[3]))
	return 0;

      curChar += 4;
    }
    /* first octect: 111110xx */
    else if ((*curChar & 0xfc) == 0xf8)	{
      /* 5 byte long encoding */
      if (!__glcIsNonStartByte(curChar[1])
	  || !__glcIsNonStartByte(curChar[2])
	  || !__glcIsNonStartByte(curChar[3])
	  || !__glcIsNonStartByte(curChar[4]))
	return 0;

      curChar += 5;
    }
    /* first octect: 1111110x */
    else if ((*curChar & 0xfe) == 0xfc)	{
      /* 6 byte long encoding */
      if (!__glcIsNonStartByte(curChar[1])
	  || !__glcIsNonStartByte(curChar[2])
	  || !__glcIsNonStartByte(curChar[3])
	  || !__glcIsNonStartByte(curChar[4])
	  || !__glcIsNonStartByte(curChar[5]))
	return 0;

      curChar += 6;
    }
    /* first octect: 11111110 */
    else if ((*curChar & 0xff) == 0xfe)	{
      /* 7 byte long encoding */
      if (!__glcIsNonStartByte(curChar[1])
	  || !__glcIsNonStartByte(curChar[2])
	  || !__glcIsNonStartByte(curChar[3])
	  || !__glcIsNonStartByte(curChar[4])
	  || !__glcIsNonStartByte(curChar[5])
	  || !__glcIsNonStartByte(curChar[6]))
	return 0;

      curChar += 7;
    }
    /* first byte: 11111111 */
    else {
      /* apparently this character uses more than 36 bit this decoder is not
       * developed to cope with those characters so error out
       */
      return 0;
    }

    ++length;
  }

  /* return non-zero to indicate success */
  *len = length;
  return 1;
}



/* \return the amount of octets/bytes required to store this Unicode codepoint
 *         when encoding it as UTF-8.
 */
static size_t __glcUtf8Length(GLCchar32 inCode)
{
  /* an ASCII character, which uses 7 bit at most, which is one byte in UTF-8 */
  if (inCode < 0x00000080)
    return 1; /* stores 7 bits */
  else if (inCode < 0x00000800)
    return 2; /* stores 11 bits */
  else if (inCode < 0x00010000)
    return 3; /* stores 16 bits */
  else if (inCode < 0x00200000)
    return 4; /* stores 21 bits */
  else if (inCode < 0x04000000)
    return 5; /* stores 26 bits */
  else if (inCode < 0x80000000)
    return 6; /* stores 31 bits */
  else /* if (inCode < 0x1000000000) */
    return 7; /* stores 36 bits */

  /* The program is not supposed to reach the end of the function.
   * The following return is there to prevent the compiler to issue
   * a warning about 'control reaching the end of a non-void function'.
   */
  return 0xdeadbeef;
}



/* Determines the amount of octets required to store this string if it's
 * encoded as UTF-8
 * \param ucs4_string the string to determine the UTF-8 buffer length of
 * \return the size required to hold \c ucs4_string if encoded in UTF-8
 */
static size_t __glcUtf8BufferLength(const GLCchar32* inUcs4String)
{
  /* Determine length of string (in bytes) when encoded in UTF-8 */
  size_t length = 0;
  const GLCchar32* curChar;

  for (curChar = inUcs4String; *curChar != 0; ++curChar)
    length += __glcUtf8Length(*curChar);

  return length;
}



/* Encodes a single UCS4 encoded Unicode codepoint (\c src) into a UTF-8
 * encoded, octet sequence.
 * \param src the UCS4 Unicode codepoint
 * \param[out] dst an output buffer to store the encoded UTF-8 sequence in.
 * \param len The buffer size of \c dst, it is advised to make this buffer at
 *            least UTF8_MAX_LEN bytes/octets large.
 * \return The number of octets used to encode the codepoint in UTF-8, this is
 *         the amount of valid octets contained in \c dst. This will be zero if
 *         encoding failed (should only happen if the output buffer is too
 *         small).
 */
static size_t __glcUtf8EncodeSingle(GLCchar32 inCode, GLCchar8* dst, size_t len)
{
  GLCchar8* curOutPos = dst;

  /* 7 bits */
  if (inCode < 0x00000080)	{
    /* 1 byte long encoding */
    if (len < 1)
      return 0;

    *(curOutPos++) = inCode;
  }
  /* 11 bits */
  else if (inCode < 0x00000800) {
    /* 2 byte long encoding */
    if (len < 2)
      return 0;

    /* 0xc0 provides the counting bits: 110
     * then append the 5 most significant bits
     */
    *(curOutPos++) = 0xc0 | (inCode >> 6);
    /* Put the next 6 bits in a byte of their own */
    *(curOutPos++) = 0x80 | (inCode & 0x3f);
  }
  /* 16 bits */
  else if (inCode < 0x00010000) {
    /* 3 byte long encoding */
    if (len < 3)
      return 0;

    /* 0xe0 provides the counting bits: 1110
     * then append the 4 most significant bits
     */
    *(curOutPos++) = 0xe0 | (inCode >> 12);
    /* Put the next 12 bits in two bytes of their own */
    *(curOutPos++) = 0x80 | ((inCode >> 6) & 0x3f);
    *(curOutPos++) = 0x80 | (inCode & 0x3f);
  }
  /* 21 bits */
  else if (inCode < 0x00200000) {
    /* 4 byte long encoding */
    if (len < 4)
      return 0;

    /* 0xf0 provides the counting bits: 11110
     * then append the 3 most significant bits
     */
    *(curOutPos++) = 0xf0 | (inCode >> 18);
    /* Put the next 18 bits in three bytes of their own */
    *(curOutPos++) = 0x80 | ((inCode >> 12) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 6) & 0x3f);
    *(curOutPos++) = 0x80 | (inCode & 0x3f);
  }
  /* 26 bits */
  else if (inCode < 0x04000000) {
    /* 5 byte long encoding */
    if (len < 5)
      return 0;

    /* 0xf8 provides the counting bits: 111110
     * then append the 2 most significant bits
     */
    *(curOutPos++) = 0xf8 | (inCode >> 24 );
    /* Put the next 24 bits in four bytes of their own */
    *(curOutPos++) = 0x80 | ((inCode >> 18) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 12) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 6) & 0x3f);
    *(curOutPos++) = 0x80 | (inCode & 0x3f);
  }
  /* 31 bits */
  else if (inCode < 0x80000000) {
    /* 6 byte long encoding */
    if (len < 6)
      return 0;

    /* 0xfc provides the counting bits: 1111110
     * then append the 1 most significant bit
     */
    *(curOutPos++) = 0xfc | (inCode >> 30);
    /* Put the next 30 bits in five bytes of their own */
    *(curOutPos++) = 0x80 | ((inCode >> 24) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 18) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 12) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 6) & 0x3f);
    *(curOutPos++) = 0x80 | (inCode & 0x3f);
  }
  /* 36 bits */
  else {
    /* 7 byte long encoding */
    if (len < 7)
      return 0;

    /* 0xfe provides the counting bits: 11111110 */
    *(curOutPos++) = 0xfe;
    /* Put the next 36 bits in six bytes of their own */
    *(curOutPos++) = 0x80 | ((inCode >> 30) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 24) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 18) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 12) & 0x3f);
    *(curOutPos++) = 0x80 | ((inCode >> 6) & 0x3f);
    *(curOutPos++) = 0x80 | (inCode & 0x3f);
  }

  return curOutPos - dst;
}



/* Encodes a UCS4 encoded unicode string to a UTF-8 encoded string
 * \param ucs4_string the UCS4 encoded unicode string to encode into UTF-8
 * \return a UTF-8 encoded unicode nul terminated string (use free() to
 *         deallocate it)
 */
static GLCchar8* __glcUtf8Encode(const GLCchar32* inUcs4String)
{
  const GLCchar32* curChar;
  size_t buf_length = __glcUtf8BufferLength(inUcs4String);
  /* Allocate memory to hold the UTF-8 encoded string (plus a terminating nul
   * GLCchar8)
   */
  GLCchar8* utf8_string = __glcMalloc(buf_length + 1);
  GLCchar8* curOutPos = utf8_string;

  if (utf8_string == NULL) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  for (curChar = inUcs4String; *curChar != 0; ++curChar)	{
    size_t stored = __glcUtf8EncodeSingle(*curChar, curOutPos, buf_length);

    assert(stored != 0);

    curOutPos += stored;
    buf_length -= stored;
  }

  /* Terminate the string with a nul character */
  *curOutPos = '\0';

  return utf8_string;
}



/* Converts a single Unicode codepoint from the UTF-8 string to a UCS4
 * representation.
 * \param utf8_string a pointer to the beginning of a valid UTF-8 octect
 *        sequence. (I.e. this function will decode the first Unicode codepoint
 *        from this string).
 * \param[out] dst the decoded UCS4 Unicode codepoint will be written in *dst.
 * \param len \c utf8_string must be at least \c len octects long.
 * \return the amount of octets consumed for decoding this UCS4 code point.
 *         This can never be larger than \c len and will be zero if decoding
 *         failed.
 * \note The user code must check for NUL ('\0') termination octets itself (if
 *       required), this function will decode a '\0' octet into Unicode
 *       codepoint 0x0.
 */
static size_t __glcUtf8DecodeSingle(const GLCchar8* inUtf8String,
				    GLCchar32* dst, size_t len)
{
  const GLCchar8* curChar = inUtf8String;

  /* Verify that this is a valid starting byte */
  if (!__glcIsStartByte(*curChar))
    return 0;

  /* first octect: 0xxxxxxx: 7 bit (ASCII) */
  if ((*curChar & 0x80) == 0x00){
    /* 1 byte long encoding */
    if (len < 1)
      return 0;

    *dst = *(curChar++);
  }
  /* first octect: 110xxxxx: 11 bit */
  else if ((*curChar & 0xe0) == 0xc0) {
    /* 2 byte long encoding */
    if (len < 2)
      return 0;

    if (!__glcIsNonStartByte(curChar[1]))
      return 0;

    *dst  = (*(curChar++) & 0x1f) << 6;
    *dst |= (*(curChar++) & 0x3f) << 0;
  }
  /* first octect: 1110xxxx: 16 bit */
  else if ((*curChar & 0xf0) == 0xe0) {
    /* 3 byte long encoding */
    if (len < 3)
      return 0;

    if (!__glcIsNonStartByte(curChar[1])
	|| !__glcIsNonStartByte(curChar[2]))
      return 0;

    *dst  = (*(curChar++) & 0x0f) << 12;
    *dst |= (*(curChar++) & 0x3f) << 6;
    *dst |= (*(curChar++) & 0x3f) << 0;
  }
  /* first octect: 11110xxx: 21 bit */
  else if ((*curChar & 0xf8) == 0xf0) {
    /* 4 byte long encoding */
    if (len < 4)
      return 0;

    if (!__glcIsNonStartByte(curChar[1])
	|| !__glcIsNonStartByte(curChar[2])
	|| !__glcIsNonStartByte(curChar[3]))
      return 0;

    *dst  = (*(curChar++) & 0x07) << 18;
    *dst |= (*(curChar++) & 0x3f) << 12;
    *dst |= (*(curChar++) & 0x3f) << 6;
    *dst |= (*(curChar++) & 0x3f) << 0;
  }
  /* first octect: 111110xx: 26 bit */
  else if ((*curChar & 0xfc) == 0xf8) {
    /* 5 byte long encoding */
    if (len < 5)
      return 0;

    if (!__glcIsNonStartByte(curChar[1])
	|| !__glcIsNonStartByte(curChar[2])
	|| !__glcIsNonStartByte(curChar[3])
	|| !__glcIsNonStartByte(curChar[4]))
      return 0;

    *dst  = (*(curChar++) & 0x03) << 24;
    *dst |= (*(curChar++) & 0x3f) << 18;
    *dst |= (*(curChar++) & 0x3f) << 12;
    *dst |= (*(curChar++) & 0x3f) << 6;
    *dst |= (*(curChar++) & 0x3f) << 0;
  }
  /* first octect: 1111110x: 31 bit */
  else if ((*curChar & 0xfe) == 0xfc) {
    /* 6 byte long encoding */
    if (len < 6)
      return 0;

    if (!__glcIsNonStartByte(curChar[1])
	|| !__glcIsNonStartByte(curChar[2])
	|| !__glcIsNonStartByte(curChar[3])
	|| !__glcIsNonStartByte(curChar[4])
	|| !__glcIsNonStartByte(curChar[5]))
      return 0;

    *dst  = (*(curChar++) & 0x01) << 30;
    *dst |= (*(curChar++) & 0x3f) << 24;
    *dst |= (*(curChar++) & 0x3f) << 18;
    *dst |= (*(curChar++) & 0x3f) << 12;
    *dst |= (*(curChar++) & 0x3f) << 6;
    *dst |= (*(curChar++) & 0x3f) << 0;
  }
  /* first octect: 11111110: 36 bit (we'll only use 32bit though) */
  else if ((*curChar & 0xff) == 0xfe) {
    /* 7 byte long encoding */
    if (len < 7)
      return 0;

    if (!__glcIsNonStartByte(curChar[1])
	|| !__glcIsNonStartByte(curChar[2])
	|| !__glcIsNonStartByte(curChar[3])
	|| !__glcIsNonStartByte(curChar[4])
	|| !__glcIsNonStartByte(curChar[5])
	|| !__glcIsNonStartByte(curChar[6]))
      return 0;

    /* original: *dst  = (*(curChar++) & 0x00) << 36; */
    /* The first octect contains no data bits */
    *dst = 0; ++curChar;

    /* original: *dst |= (*(curChar++) & 0x3f) << 30; */
    /* Use only the 2 least significant bits of this byte
     * to make sure we use 32bit at maximum
     */
    *dst |= (*(curChar++) & 0x03) << 30;

    *dst |= (*(curChar++) & 0x3f) << 24;
    *dst |= (*(curChar++) & 0x3f) << 18;
    *dst |= (*(curChar++) & 0x3f) << 12;
    *dst |= (*(curChar++) & 0x3f) << 6;
    *dst |= (*(curChar++) & 0x3f) << 0;
  }
  /* first byte: 11111111: 41 bit or more */
  else {
    /* apparently this character uses more than 36 bit this decoder is not
     * developed to cope with those characters so error out
     */
    return 0;
  }

  return curChar - inUtf8String;
}



#if 0 /* Comment out this function in order to prevent warning messages about
       * this function being defined but not used.
       */

/* Decodes a UTF-8 encode string to a UCS4 encoded string (native endianess)
 * \param utf8_string a UTF-8 encoded nul terminated string
 * \return a UCS4 encoded unicode nul terminated string (use free() to
 *         deallocate it)
 */
static GLCchar32* __glcUtf8Decode(const GLCchar8* inUtf8String)
{
  const GLCchar8* curChar = inUtf8String;
  size_t unicode_length;
  size_t utf8_len = strlen((const char*)inUtf8String);
  GLCchar32 *ucs4_string, *curOutPos;

  /* Determine the string's length (i.e. the amount of Unicode codepoints) */
  if (!__glcUtf8CharacterCount(inUtf8String, &unicode_length)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return NULL;
  }

  /* Allocate memory to hold the UCS4 encoded string (plus a terminating nul) */
  ucs4_string = __glcMalloc(sizeof(GLCchar32) * (unicode_length + 1));
  if (!ucs4_string) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  curOutPos = ucs4_string;
  while (*curChar != '\0') {
    size_t consumedBytes = __glcUtf8DecodeSingle(curChar, curOutPos, utf8_len);

    assert(consumedBytes != 0);

    curChar += consumedBytes;
    utf8_len -= consumedBytes;
    ++curOutPos;
  }

  /* Terminate the string with a nul */
  *curOutPos = '\0';

  return ucs4_string;
}
#endif



/* Find a Unicode name from its code */
GLCchar* __glcNameFromCode(GLint code)
{
  GLint position = -1;

  if ((code < 0) || (code > __glcMaxCode)) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  position = __glcNameFromCodeArray[code];
  if (position == -1) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return GLC_NONE;
  }

  return __glcCodeFromNameArray[position].name;
}



/* Find a Unicode code from its name */
GLint __glcCodeFromName(GLCchar* name)
{
  int start = 0;
  int end = __glcCodeFromNameSize;
  int middle = (end + start) / 2;
  int res = 0;

  while (end - start > 1) {
    res = strcmp(name, __glcCodeFromNameArray[middle].name);
    if (res > 0)
      start = middle;
    else if (res < 0)
      end = middle;
    else
      return __glcCodeFromNameArray[middle].code;
    middle = (end + start) / 2;
  }
  if (strcmp(name, __glcCodeFromNameArray[start].name) == 0)
    return __glcCodeFromNameArray[start].code;
  if (strcmp(name, __glcCodeFromNameArray[end].name) == 0)
    return __glcCodeFromNameArray[end].code;

  __glcRaiseError(GLC_PARAMETER_ERROR);
  return -1;
}

/* Convert a character from UTF-8 to UCS1 and return the number of bytes
 * needed to encode the character.
 * According to the GLC specs, when the value of a character code exceed the
 * range of the character encoding, the returned character is converted
 * to a character sequence \<hexcode> where 'hexcode' is the original
 * character code represented as a sequence of hexadecimal digits
 */
static int __glcUtf8ToUcs1(const GLCchar8* src_orig,
			   GLCchar8 dst[GLC_OUT_OF_RANGE_LEN], int len,
			   int* dstlen)
{
  GLCchar32 result = 0;
  size_t src_shift = __glcUtf8DecodeSingle(src_orig, &result, len);

  if (src_shift > 0) {
    /* src_orig is a well-formed UTF-8 character */
    if (result < 0x100) {
      *dst = result;
      *dstlen = 1;
    }
    else {
      /* Convert to the string '\<xxx>' */
#ifdef _MSC_VER
      *dstlen = sprintf_s((char*)dst, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* sprintf_s returns -1 on any error, and the number of characters
       * written to the string not including the terminating null otherwise.
       * Insufficient length of the destination buffer is an error and the
       * buffer is set to an empty string. */
      if (*dstlen < 0)
        *dstlen = 0;
#else
      *dstlen = snprintf((char*)dst, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
      /* Standard ISO/IEC 9899:1999 (ISO C99) snprintf, which it appears
       * Microsoft has not implemented for their operating systems. Return
       * value is length of the string that would have been written into
       * the destination buffer not including the terminating null had their
       * been enough space. Truncation has occurred if return value is >= 
       * destination buffer size. */
      if (*dstlen >= GLC_OUT_OF_RANGE_LEN)
        *dstlen = GLC_OUT_OF_RANGE_LEN - 1;
#endif
    }
  }
  return src_shift;
}



/* Convert a character from UTF-8 to UCS1 and return the number of bytes
 * needed to encode the character.
 * According to the GLC specs, when the value of a character code exceed the
 * range of the character encoding, the returned character is converted
 * to a character sequence \<hexcode> where 'hexcode' is the original
 * character code represented as a sequence of hexadecimal digits
 */
static int __glcUtf8ToUcs2(const GLCchar8* src_orig,
			   GLCchar16 dst[GLC_OUT_OF_RANGE_LEN], int len,
			   int* dstlen)
{
  GLCchar32 result = 0;
  size_t src_shift = __glcUtf8DecodeSingle(src_orig, &result, len);

  if (src_shift > 0) {
    /* src_orig is a well-formed UTF-8 character */
    if (result < 0x10000) {
      *dst = result;
      *dstlen = 1;
    }
    else {
      /* Convert to the string '\<xxx>' */
      int count;
      char* src = NULL;
      char buffer[GLC_OUT_OF_RANGE_LEN];

#ifdef _MSC_VER
      sprintf_s(buffer, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
#else
      snprintf(buffer, GLC_OUT_OF_RANGE_LEN, "\\<%X>", result);
#endif
      for (count = 0, src = buffer; *src && count < GLC_OUT_OF_RANGE_LEN;
	   count++, *dst++ = *src++);
      *dst = 0; /* Terminating '\0' character */
      *dstlen = count;
    }
  }
  return src_shift;
}



/* Convert 'inString' in the UTF-8 format and return a copy of the converted
 * string.
 */
GLCchar8* __glcConvertToUtf8(const GLCchar* inString, const GLint inStringType)
{
  GLCchar8 buffer[GLC_UTF8_MAX_LEN];
  GLCchar8* string = NULL;
  GLCchar8* ptr = NULL;
  int len;

  switch(inStringType) {
  case GLC_UCS1:
    {
      GLCchar8* ucs1 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs1 = (GLCchar8*)inString; *ucs1;
	   len += __glcUtf8EncodeSingle(*ucs1++, buffer, sizeof(buffer)));
      /* Allocate the room to store the final string */
      string = (GLCchar8*)__glcMalloc((len+1)*sizeof(GLCchar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs1 = (GLCchar8*)inString, ptr = string; *ucs1;
	   ptr += __glcUtf8EncodeSingle(*ucs1++, ptr, len - (ptr - string)));
      *ptr = 0;
    }
    break;
  case GLC_UCS2:
    {
      GLCchar16* ucs2 = NULL;

      /* Determine the length of the final string */
      for (len = 0, ucs2 = (GLCchar16*)inString; *ucs2;
	     len += __glcUtf8EncodeSingle(*ucs2++, buffer, sizeof(buffer)));
      /* Allocate the room to store the final string */
      string = (GLCchar8*)__glcMalloc((len+1)*sizeof(GLCchar8));
      if (!string) {
	__glcRaiseError(GLC_RESOURCE_ERROR);
	return NULL;
      }

      /* Perform the conversion */
      for (ucs2 = (GLCchar16*)inString, ptr = string; *ucs2;
	     ptr += __glcUtf8EncodeSingle(*ucs2++, ptr, len - (ptr - string)));
      *ptr = 0;
    }
    break;
  case GLC_UCS4:
    {
      /* Convert the string (memory allocation is performed automatically) */
      string = __glcUtf8Encode((GLCchar32*)inString);
    }
    break;
  case GLC_UTF8_QSO:
    /* If the string is already encoded in UTF-8 format then all we need to do
     * is to make a copy of it.
     */
#ifdef __WIN32__
    string = (GLCchar8*)_strdup((const char*)inString);
#else
    string = (GLCchar8*)strdup((const char*)inString);
#endif
    break;
  default:
    return NULL;
  }

  return string;
}



/* Convert 'inString' from the UTF-8 format and return a copy of the
 * converted string in the context buffer.
 */
GLCchar* __glcConvertFromUtf8ToBuffer(__GLCcontext* This,
				      const GLCchar8* inString,
				      const GLint inStringType)
{
  GLCchar* string = NULL;
  const GLCchar8* utf8 = NULL;
  int len_buffer = 0;
  int len = 0;
  int shift = 0;

  assert(inString);

  switch(inStringType) {
  case GLC_UCS1:
    {
      GLCchar8 buffer[GLC_OUT_OF_RANGE_LEN];
      GLCchar8* ucs1 = NULL;

      /* Determine the length of the final string */
      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs1(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift == 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcContextQueryBuffer(This,
						 (len+1)*sizeof(GLCchar8));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      ucs1 = (GLCchar8*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs1(utf8, ucs1, strlen((const char*)utf8),
				&len_buffer);
	ucs1 += len_buffer;
      }

      *ucs1 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      GLCchar16 buffer[GLC_OUT_OF_RANGE_LEN];
      GLCchar16* ucs2 = NULL;

      /* Determine the length of the final string */
      utf8 = inString;
      while(*utf8) {
	shift = __glcUtf8ToUcs2(utf8, buffer, strlen((const char*)utf8),
				&len_buffer);
	if (shift == 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  return NULL;
	}
	utf8 += shift;
	len += len_buffer;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcContextQueryBuffer(This,
						 (len+1)*sizeof(GLCchar16));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      ucs2 = (GLCchar16*)string;
      utf8 = inString;
      while(*utf8) {
	utf8 += __glcUtf8ToUcs2(utf8, ucs2, strlen((const char*)utf8),
				&len_buffer);
	ucs2 += len_buffer;
      }
      *ucs2 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      GLCchar32* ucs4 = NULL;
      size_t len;

      /* Determine the length of the final string */
      if (!__glcUtf8CharacterCount(inString, &len)) {
        __glcRaiseError(GLC_PARAMETER_ERROR);
        return NULL;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar*)__glcContextQueryBuffer(This,
						 (len+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */


      /* Perform the conversion */
      utf8 = inString;
      ucs4 = (GLCchar32*)string;

      while(*utf8) {
        size_t consumedOctets = __glcUtf8DecodeSingle(utf8, ucs4++, len);

        assert(consumedOctets != 0);

        utf8 += consumedOctets;
        len -= consumedOctets;
      }

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    /* If the string is already encoded in UTF-8 format then all we need to do
     * is to make a copy of it.
     */
    string = (GLCchar*)__glcContextQueryBuffer(This,
				 	       strlen((const char*)inString)+1);
    if (!string)
      return NULL; /* GLC_RESOURCE_ERROR has been raised */
    strcpy(string, (const char*)inString);
    break;
  default:
    return NULL;
  }
  return string;
}



/* Convert a UCS-4 character code into the current string type. The result is
 * stored in a GLint. If the code can not be converted to the current string
 * type a GLC_PARAMETER_ERROR is issued.
 */
GLint __glcConvertUcs4ToGLint(__GLCcontext *inContext, GLint inCode)
{
  switch(inContext->stringState.stringType) {
  case GLC_UCS2:
    /* Check that inCode can be stored in UCS-2 format */
    if (inCode <= 65535)
      break;
  case GLC_UCS1:
    /* Check that inCode can be stored in UCS-1 format */
    if (inCode <= 255)
      break;
  case GLC_UTF8_QSO:
    /* A Unicode codepoint can be no higher than 0x10ffff
     * (see Unicode specs)
     */
    if (inCode > 0x10ffff) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
    else {
      /* Codepoints lower or equal to 0x10ffff can be encoded on 4 bytes in
       * UTF-8 format
       */
      GLCchar8 buffer[GLC_UTF8_MAX_LEN];
      assert(__glcUtf8Length((GLCchar32)inCode) <= sizeof(GLint));
      __glcUtf8EncodeSingle((GLCchar32)inCode, buffer, sizeof(buffer));

      return *((GLint*)buffer);
    }
  }

  return inCode;
}



/* Convert a character encoded in the current string type to the UCS-4 format.
 * This function is needed since the GLC specs store individual character codes
 * in GLint which may cause problems for the UTF-8 format.
 */
GLint __glcConvertGLintToUcs4(__GLCcontext *inContext, GLint inCode)
{
  GLint code = inCode;

  if (inCode < 0) {
    __glcRaiseError(GLC_PARAMETER_ERROR);
    return -1;
  }

  /* If the current string type is GLC_UTF8_QSO then converts to GLC_UCS4 */
  if (inContext->stringState.stringType == GLC_UTF8_QSO) {
    /* Convert the codepoint in UCS4 format and check if it is ill-formed or
     * not
     */
    if (!__glcUtf8DecodeSingle((GLCchar8*)&inCode, (GLCchar32*)&code,
		     sizeof(GLint))) {
      __glcRaiseError(GLC_PARAMETER_ERROR);
      return -1;
    }
  }

  return code;
}



/* Convert 'inString' (stored in logical order) to UCS4 format and return a
 * copy of the converted string in visual order.
 */
GLCchar32* __glcConvertToVisualUcs4(__GLCcontext* inContext,
				    GLboolean *outIsRTL, GLint *outLength,
				    const GLCchar* inString)
{
  GLCchar32* string = NULL;
  int length = 0;
  FriBidiCharType base = FRIBIDI_TYPE_ON;
  GLCchar32* visualString = NULL;

  assert(inString);

  switch(inContext->stringState.stringType) {
  case GLC_UCS1:
    {
      GLCchar8* ucs1 = (GLCchar8*)inString;
      GLCchar32* ucs4 = NULL;

      length = strlen((const char*)ucs1);

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      for (ucs4 = string; *ucs1; ucs1++, ucs4++)
	*ucs4 = (GLCchar32)(*ucs1);

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      GLCchar16* ucs2 = NULL;
      GLCchar32* ucs4 = NULL;

      for (ucs2 = (GLCchar16*)inString; *ucs2; ucs2++, length++);

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      for (ucs2 = (GLCchar16*)inString, ucs4 = string; *ucs2; ucs2++, ucs4++)
	*ucs4 = (GLCchar32)(*ucs2);

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      GLCchar32* ucs4 = NULL;
      int length = 0;

      for (ucs4 = (GLCchar32*)inString; *ucs4; ucs4++, length++);

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(int));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      memcpy(string, inString, length*sizeof(int));

      ((int*)string)[length] = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    {
      GLCchar32* ucs4 = NULL;
      GLCchar8* utf8 = NULL;
      GLCchar32 buffer = 0;
      int shift = 0;

      /* Determine the length of the final string */
      utf8 = (GLCchar8*)inString;
      while(*utf8) {
	shift = __glcUtf8DecodeSingle(utf8, &buffer, strlen((const char*)utf8));
	if (shift == 0) {
	  /* There is an ill-formed character in the UTF-8 string, abort */
	  return NULL;
	}
	utf8 += shift;
	length++;
      }

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(length+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      utf8 = (GLCchar8*)inString;
      ucs4 = (GLCchar32*)string;
      while(*utf8)
	utf8 += __glcUtf8DecodeSingle(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  }

  if (length) {
    visualString = string + length + 1;
    if (!fribidi_log2vis(string, length, &base, visualString, NULL, NULL,
                         NULL)) {
      __glcRaiseError(GLC_RESOURCE_ERROR);
      return NULL;
    }

    *outIsRTL = FRIBIDI_IS_RTL(base) ? GL_TRUE : GL_FALSE;
  }
  else
    visualString = string;

  *outLength = length;

  return visualString;
}



/* Convert 'inCount' characters of 'inString' (stored in logical order) to UCS4
 * format and return a copy of the converted string in visual order.
 */
GLCchar32* __glcConvertCountedStringToVisualUcs4(__GLCcontext* inContext,
						GLboolean *outIsRTL,
						const GLCchar* inString,
						const GLint inCount)
{
  GLCchar32* string = NULL;
  FriBidiCharType base = FRIBIDI_TYPE_ON;
  GLCchar32* visualString = NULL;

  assert(inString);

  switch(inContext->stringState.stringType) {
  case GLC_UCS1:
    {
      GLCchar8* ucs1 = (GLCchar8*)inString;
      GLCchar32* ucs4 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      ucs4 = string;

      for (i = 0; i < inCount; i++)
	*(ucs4++) = (GLCchar32)(*(ucs1++));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS2:
    {
      GLCchar16* ucs2 = NULL;
      GLCchar32* ucs4 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      ucs2 = (GLCchar16*)inString;
      ucs4 = string;

      for (i = 0 ; i < inCount; i++)
	*(ucs4++) = (GLCchar32)(*(ucs2++));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UCS4:
    {
      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(int));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      memcpy(string, inString, inCount*sizeof(int));

      ((int*)string)[inCount] = 0; /* Add the '\0' termination of the string */
    }
    break;
  case GLC_UTF8_QSO:
    {
      GLCchar32* ucs4 = NULL;
      GLCchar8* utf8 = NULL;
      GLint i = 0;

      /* Allocate the room to store the final string */
      string = (GLCchar32*)__glcContextQueryBuffer(inContext,
					      2*(inCount+1)*sizeof(GLCchar32));
      if (!string)
	return NULL; /* GLC_RESOURCE_ERROR has been raised */

      /* Perform the conversion */
      utf8 = (GLCchar8*)inString;
      ucs4 = (GLCchar32*)string;

      for (i = 0; i < inCount; i++)
	utf8 += __glcUtf8DecodeSingle(utf8, ucs4++, strlen((const char*)utf8));

      *ucs4 = 0; /* Add the '\0' termination of the string */
    }
    break;
  }

  visualString = string + inCount;
  if (!fribidi_log2vis(string, inCount, &base, visualString, NULL, NULL,
		       NULL)) {
    __glcRaiseError(GLC_RESOURCE_ERROR);
    return NULL;
  }

  *outIsRTL = FRIBIDI_IS_RTL(base) ? GL_TRUE : GL_FALSE;

  return visualString;
}
