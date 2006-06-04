#include "GL/glc.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Check that glcGetCharMetric() and glcGetStringCharMetric() return consistant
 * values.
 */

#define EPSILON 1E-5
static char* string = "Hello";
#define CheckError() \
  err = glcGetError(); \
  if (err) { \
    printf("Unexpected error : 0x%X\n", err); \
    return -1; \
  }

int main(void) {
  GLint ctx;
  GLCenum err;
  GLint length = 0;
  GLint i, j;
  GLfloat baseline1[4], baseline2[4];
  GLfloat boundingBox1[8], boundingBox2[8];
  GLfloat v1, v2, norm, area;

  ctx = glcGenContext();
  CheckError();

  glcContext(ctx);
  CheckError();

  length = glcMeasureString(GL_TRUE, string);
  if ((!length) || (length != strlen(string))) {
    printf("glcMeasureString() failed to measure %d characters"
	   " (%d measured instead)\n", (int)strlen(string), length);
    return -1;
  }

  CheckError();

  for (i = 0; i < strlen(string); i++) {
    if (!glcGetCharMetric(string[i], GLC_BASELINE, baseline1)) {
      printf("Failed to measure the baseline of the character %c\n",
	     string[i]);
      return -1;
    }

    CheckError();

    if (!glcGetStringCharMetric(i, GLC_BASELINE, baseline2)) {
      printf("Failed to get the baseline of the %dth character of the string"
	     " %s\n", i, string);
      return -1;
    }

    CheckError();

    for (j = 0; j < 4; j++) {
      v1 = fabs(baseline1[j]);
      v2 = fabs(baseline2[j]);
      norm = v1 > v2 ? v1 : v2;

      if (fabs(v1 - v2) > EPSILON * norm) {
	printf("Baseline values differ [rank %d char %d] %f (char),"
	       " %f (string)\n", j, i, v1, v2);
	return -1;
      }
    }

    if (!glcGetCharMetric(string[i], GLC_BOUNDS, boundingBox1)) {
      printf("Failed to measure the bounding box of the character %c\n",
	     string[i]);
      return -1;
    }

    CheckError();

    if (!glcGetStringCharMetric(i, GLC_BOUNDS, boundingBox2)) {
      printf("Failed to get the bounding box of the %dth character of the"
	     " string %s\n", i, string);
      return -1;
    }

    CheckError();

    for (j = 0; j < 8; j++) {
      v1 = fabs(boundingBox1[j]);
      v2 = fabs(boundingBox2[j]);
      norm = v1 > v2 ? v1 : v2;

      if (fabs(v1 - v2) > EPSILON * norm) {
	printf("Bounding Box values differ [rank %d char %d] %f (char),"
	       " %f (string)", j, i, v1, v2);
	return -1;
      }
    }
  }

  if (!glcGeti(GLC_FONT_COUNT)) {
    printf("GLC_FONT_LIST is empty\n");
    return -1;
  }

  CheckError();

  if (!glcGeti(GLC_CURRENT_FONT_COUNT)) {
    printf("GLC_CURRENT_FONT_LIST is empty\n");
    return -1;
  }

  CheckError();

  if (!glcGetMaxCharMetric(GLC_BASELINE, baseline1)) {
    printf("Failed to get the max baseline of the current fonts\n");
    return -1;
  }

  CheckError();

  v1 = fabs(baseline1[1]);
  v2 = fabs(baseline1[3]);
  norm = v1 > v2 ? v1 : v2;

  if (fabs(v1 - v2) <= EPSILON * norm) {
    v1 = fabs(baseline1[0]);
    v2 = fabs(baseline1[2]);
    norm = v1 > v2 ? v1 : v2;
    if ((fabs(v1 - v2) <= EPSILON * norm) || (baseline1[2] < baseline1[0])) {
      printf("Right and left side of the baseline are swapped\n");
      printf("%f %f %f %f\n", baseline1[0], baseline1[1], baseline1[2],
	     baseline1[3]);
      return -1;
    }
  }

  if (!glcGetMaxCharMetric(GLC_BOUNDS, boundingBox1)) {
    printf("Failed to get the max bounding box of the current fonts\n");
    return -1;
  }

  CheckError();

  area = 0.;
  for (i = 0; i < 3; i++) {
    area += boundingBox1[2*i] * boundingBox1[2*(i+1)+1] 
      - boundingBox1[2*(i+1)] * boundingBox1[2*i+1];
  }

  if (fabs(area * .5) < EPSILON) {
    printf("Max area of the characters is null\n");
    return -1;
  }

  printf("Tests successful\n");
  return 0;
}