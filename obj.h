/* I3DG&A Object loader
 * 
 * Alex Holkner 2006-2008
 */

/* $Id: obj.h 98 2008-04-01 13:46:13Z jarcher $ */

#ifndef OBJ_H
#define OBJ_H

#include "mtl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type for a 3D vertex or point. */
typedef struct _vertex {
    float x;
    float y;
    float z;
} vertex_t;

/* Type for a triangle, consisting of three points (p1, p2, p3) which index
 * into an array of vertices, three texture coordinates (t1, t2, t3) and
 * three normals (n1, n2, n3). */
typedef struct _triangle {
    int p1, t1, n1;
    int p2, t2, n2;
    int p3, t3, n3;
    const material_t *material;
} triangle_t;

typedef struct _vertex_array {
    int max_vertices;
    int n_vertices;
    vertex_t *vertices;
} vertex_array_t;


/* Type for a complete mesh.  
 *
 * The vertices are stored as a separate array to the triangles for efficiency
 * (often a vertex will be shared between several triangles).  
 *
 * The arrays may need to grow while the file is being loaded, so we keep
 * track (max_vertices, max_triangles) of the current allocation size so
 * it can be realloc'd when necessary.  n_vertices and n_triangles give
 * the actual number of vertices and triangles that are valid within the
 * array (rather than allocating just a single element at a time).
 */
typedef struct _mesh {
    vertex_array_t vertices;
    vertex_array_t tex_coords;
    vertex_array_t normals;

    int max_triangles;
    int n_triangles;
    triangle_t *triangles;

    material_t *material;
} mesh_t;

/* Public functions to load, destroy (free) and draw a mesh. */
mesh_t *mesh_load(const char *obj_filename);
void mesh_destroy(mesh_t *mesh);
void mesh_draw(mesh_t *mesh);
void mesh_draw_normals(mesh_t *mesh);

#ifdef __cplusplus
}
#endif

#endif
