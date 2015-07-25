/* OBJ Material file loader, for use with obj.c */

#ifndef MTL_H
#define MTL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _color {
    float r;
    float g;
    float b;
    float a;
} color_t;

typedef struct _material {
    char *name;
    color_t ambient;
    color_t diffuse;
    color_t specular;
    float opacity;
    float shininess;
    struct _material *next;
} material_t;

extern material_t default_material;

material_t *material_load(const char *filename);
void material_destroy(material_t *material);

const material_t *material_find(const material_t *material, const char *name);
void material_apply(const material_t *material);

#ifdef __cplusplus
}
#endif

#endif
