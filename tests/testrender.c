/* $Id$ */
#include "GL/glc.h"
#include <stdio.h>

extern void my_fini(void);

void testQueso(void)
{
    int ctx = 0;
    int font = 0;
    int i = 0;
    int ntex = 0;
    int ndl = 0;
    
    glEnable(GL_TEXTURE_2D);
    
    ctx = glcGenContext();
    glcContext(ctx);
    
    font = glcNewFontFromFamily(glcGenFontID(), "Courier");
    glcFont(font);
    glcFontFace(font, "Italic");
    
    /*    glcFontMap(font, 0x57, "LATIN SMALL LETTER W"); */
    
    glcRenderStyle(GLC_LINE);
    glColor3f(0., 1., 0.);
    glTranslatef(100., 50., 0.);
    glRotatef(45., 0., 0., 1.);
    glScalef(120., 120., 0.);
    glcRenderChar('L');
    glcRenderString("inux");
    glLoadIdentity();
    
    glcRenderStyle(GLC_TEXTURE);
    glColor3f(0., 0., 1.);
    glTranslatef(30., 350., 0.);
    glScalef(100., 100., 0.);
    glcRenderChar('X');
    glcRenderString("-windows");
    glLoadIdentity();
    
    glcRenderStyle(GLC_BITMAP);
    glColor3f(1., 0., 0.);
    glRasterPos2f(30., 200.);
    glcScale(120., 120.);
    glcRotate(10.);
    glcRenderChar('O');
    glcRenderString("penGL");
     
    glcRenderStyle(GLC_TRIANGLE);
    glColor3f(1., 1., 0.);
    glTranslatef(30., 50., 0.);
    glScalef(120., 120., 0.);
    glcRenderChar('Q');
    glcRenderStyle(GLC_LINE);
    glcRenderChar('u');
    glcRenderStyle(GLC_TRIANGLE);
    glcRenderString("esoGLC");
    
    ntex = glcGeti(GLC_TEXTURE_OBJECT_COUNT);
    printf("Textures : %d\n", ntex);
    for (i = 0; i < ntex; i++)
      printf("%d ", glcGetListi(GLC_TEXTURE_OBJECT_LIST, i));

    ndl = glcGeti(GLC_LIST_OBJECT_COUNT);
    printf("\nDisplay Lists : %d\n", ndl);
    for (i = 0; i < ndl; i++)
      printf("%d ", glcGetListi(GLC_LIST_OBJECT_LIST, i));

    printf("\n");
    my_fini();
}
