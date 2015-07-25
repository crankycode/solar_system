/* $Id: mtl.c 96 2008-03-26 11:08:52Z aholkner $ */

/* OBJ Material file loader, for use with obj.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  ifdef WIN32
#    include <windows.h>
#  endif
#  include <GL/gl.h>
#endif

#include "mtl.h"

#define MAX_LINE_LENGTH 1024

material_t default_material = {
    "default",
    {0.2f, 0.2f, 0.2f, 1.0f},
    {0.8f, 0.8f, 0.8f, 1.0f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    1.0f,
    0.0f,
    NULL
};

material_t *material_add(material_t *last_material, const char *name)
{
    size_t name_len = strcspn(name, "\n\r ");

    material_t *material = malloc(sizeof(*material));

    material->name = malloc(name_len + 1);
    strncpy(material->name, name, name_len);
    material->name[name_len] = '\0';
    material->ambient = default_material.ambient;
    material->diffuse = default_material.diffuse;
    material->specular = default_material.specular;
    material->opacity = default_material.opacity;
    material->shininess = default_material.shininess;
    material->next = last_material;

    return material;
}

void material_set_color_data(color_t *color, const char *buf)
{
    const char *bufp;
    color->a = 1.;
    color->r = (float) strtod(buf, (char **) &bufp);
    if (bufp)
    {
        color->g = (float) strtod(bufp, (char **) &bufp);
        if (bufp)
        {
            color->b = (float) strtod(bufp, NULL);
        }

    }
}

void material_set_float_data(float *data, const char *buf)
{
    *data = (float) strtod(buf, NULL);
}

material_t *material_load(const char *filename)
{
    char buf[MAX_LINE_LENGTH];
    char *bufp;
    size_t token_len;
    material_t *material = NULL;

    FILE *file = fopen(filename, "rt");
    if (!file)
        return NULL;

    while (fgets(buf, sizeof(buf), file))
    {
        if (buf[0] == '#')
            continue;

        bufp = strchr(buf, ' ');
        if (!bufp)
            continue;

        token_len = bufp - buf;
        bufp++;

        if (strncmp(buf, "newmtl", token_len) == 0)
            material = material_add(material, bufp);
        else if (strncmp(buf, "Ka", token_len) == 0)
            material_set_color_data(&material->ambient, bufp);
        else if (strncmp(buf, "Kd", token_len) == 0)
            material_set_color_data(&material->diffuse, bufp);
        else if (strncmp(buf, "Ks", token_len) == 0)
            material_set_color_data(&material->specular, bufp);
        else if (strncmp(buf, "d", token_len) == 0)
            material_set_float_data(&material->opacity, bufp);
        else if (strncmp(buf, "Tr", token_len) == 0)
            material_set_float_data(&material->opacity, bufp);
        else if (strncmp(buf, "Ns", token_len) == 0)
            material_set_float_data(&material->shininess, bufp);
    }

    fclose(file);

    return material;
}

void material_destroy(material_t *material)
{
    material_t *doomed;
    while (material)
    {
        free(material->name);

        doomed = material;
        material = material->next;
        free(doomed);
    }
}

const material_t *material_find(const material_t *material, const char *name)
{
    size_t name_len = strcspn(name, "\n\r ");

    while (material)
    {
        if (strncmp(material->name, name, name_len) == 0)
            return material;
        material = material->next;
    }

    return &default_material;
}

void material_apply(const material_t *material)
{
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, &material->ambient.r);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, &material->diffuse.r);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &material->specular.r);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material->shininess);
}
