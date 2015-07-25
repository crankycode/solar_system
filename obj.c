/* I3DG&A Object loader
 * 
 * Alex Holkner 2006-2008
 *
 * This source file contains functions for parsing an OBJ file into a
 * mesh_t structure and then displaying that structure using OpenGL.
 */

/* $Id: obj.c 101 2008-04-07 05:01:47Z aholkner $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  ifdef WIN32
#    include <windows.h>
#    define snprintf _snprintf
#  endif
#  include <GL/gl.h>
#endif

#include "obj.h"

/* The initial default size for the arrays of a new mesh. */
#define VERTICES_INIT_SIZE 100
#define TRIANGLES_INIT_SIZE 100

/* How much to increase the size of the arrays by when they need to be
 * resized. */
#define VERTICES_INCR_SIZE 100
#define TRIANGLES_INCR_SIZE 100

/* Maximum length of a line in an OBJ file. */
#define MAX_LINE_LENGTH 1024

/* Maximum path size on the filesystem. */
#define MAX_PATH 1024

/* Add a vertex to a mesh.  The vertex is copied in, but the array is
 * resized first if it is not large enough.
 */
void add_vertex(vertex_array_t *array, const vertex_t *vertex)
{
    /* Resize vertex array if not large enough */
    if (array->n_vertices == array->max_vertices)
    {
        array->max_vertices += VERTICES_INCR_SIZE;
        array->vertices = realloc(array->vertices, 
                                 sizeof(*array->vertices) * array->max_vertices);
    }

    /* Copy in the next vertex */
    array->vertices[array->n_vertices++] = *vertex;
}

void pad_vertex_data(vertex_array_t *array, int n_vertices)
{
    if (array->max_vertices < n_vertices)
    {
        array->max_vertices = n_vertices;
        array->vertices = realloc(array->vertices, 
                                  sizeof(*array->vertices) * array->max_vertices);
    }

    while (array->n_vertices < n_vertices)
    {
        array->vertices[array->n_vertices].x = 0;
        array->vertices[array->n_vertices].y = 0;
        array->vertices[array->n_vertices++].z = 0;
    }
}

/* Add a vertex to a mesh, by first parsing the line from the OBJ file.
 * The line `buf` does not include the "v" at the start, just the
 * vertex data.
 */
void add_vertex_data(vertex_array_t *array, const char *buf)
{
    vertex_t vertex;
    const char *bufp = buf;

    vertex.x = vertex.y = vertex.z = 0.;

    /* Read up to three floats separated by spaces */
    vertex.x = (float) strtod(bufp, (char **) &bufp);
    if (bufp)
    {
        vertex.y = (float) strtod(bufp, (char **) &bufp);
        if (bufp)
            vertex.z = (float) strtod(bufp, (char **) &bufp);
    }

    add_vertex(array, &vertex);
}

/* Add a triangle to a mesh.  The triangle is copied in but the array
 * is resized first if there is not enough room.
 */
void add_triangle(mesh_t *mesh, const triangle_t *triangle)
{
    /* Resize triangle array if not large enough */
    if (mesh->n_triangles == mesh->max_triangles)
    {
        mesh->max_triangles += VERTICES_INCR_SIZE;
        mesh->triangles = realloc(mesh->triangles, 
                             sizeof(*mesh->triangles) * mesh->max_triangles);
    }

    /* Copy in the next triangle */
    mesh->triangles[mesh->n_triangles++] = *triangle;
}

/* Add a vertex to a triangle by parsing the components from the
 * OBJ file.  `pointp` points to a single component in a triangle
 * (either triangle.p1, triangle.p2, or triangle.p3).  Sim. for texp
 * and normalp.  `buf` is "1//1" or "81/65/23", etc.
 */
void add_triangle_vertex_data(int *pointp, 
                              int *texp,
                              int *normalp,
                              const char *buf)
{

    const char *bufp;
    *pointp = *texp = *normalp = 0;

    *pointp = strtol(buf, (char **) &bufp, 10) - 1;
    if (!bufp)
        return;     /* Error */
    bufp++;

    if (*bufp != '/')
    {
        *texp = strtol(bufp, (char **) &bufp, 10) - 1;
        if (!bufp)
            return;     /* Error */
    }
    bufp++;

    *normalp = strtol(bufp, (char **) &bufp, 10) - 1;
}


/* Add a face (polygon) to the mesh.  Faces may consist of more than
 * three vertices, so some very simple triangulation is implemented
 * to split up larger polygons into triangles.
 */
void add_face_data(mesh_t *mesh, const char *buf, const material_t *material)
{
    triangle_t triangle;
    const char *bufp = buf;

    triangle.material = material;

    /* Find each v/vt/vn group */
    add_triangle_vertex_data(&triangle.p1, 
                             &triangle.t1, 
                             &triangle.n1, bufp);

    bufp = strchr(bufp, ' ');
    if (!bufp)
        return;         /* Not enough points */
    bufp++;             /* Skip past space */
    add_triangle_vertex_data(&triangle.p2, 
                             &triangle.t2, 
                             &triangle.n2, bufp);

    bufp = strchr(bufp, ' ');
    if (!bufp)
        return;         /* Not enough points */
    bufp++;             /* Skip past space */
    add_triangle_vertex_data(&triangle.p3, 
                             &triangle.t3, 
                             &triangle.n3, bufp);

    /* We have a complete triangle now: add it to the mesh. */
    add_triangle(mesh, &triangle);

    /* For each additional point after the first three we are describing
     * a polygon, not a triangle.  Continue adding triangles using
     * the first point of the first triangle, the last point of the previous
     * triangle, and the new point.  This is a crude form of triangulation
     * that creates a triangle fan.  It is not correct for concave polygons,
     * and for convex polygons it does not always give the best triangulation
     * possible.  (But it was easy to implement).
     */
    while ((bufp = strchr(bufp, ' ')) != NULL)
    {
        triangle.p2 = triangle.p3;
        triangle.t2 = triangle.t3;
        triangle.n2 = triangle.n3;
        bufp++;             /* Skip past space */
        add_triangle_vertex_data(&triangle.p3, 
                                 &triangle.t3, 
                                 &triangle.n3, bufp);
        add_triangle(mesh, &triangle);
    }
}

void vertex_array_init(vertex_array_t *array)
{
    array->max_vertices = VERTICES_INIT_SIZE;
    array->n_vertices = 0;
    array->vertices = malloc(sizeof(*array->vertices) * array->max_vertices);
}

/* Allocate memory for and "zero" a new mesh. */
mesh_t *mesh_create()
{
    mesh_t *mesh = malloc(sizeof(*mesh));
    vertex_array_init(&mesh->vertices);
    vertex_array_init(&mesh->tex_coords);
    vertex_array_init(&mesh->normals);
    mesh->max_triangles = TRIANGLES_INIT_SIZE;
    mesh->n_triangles = 0;
    mesh->triangles = malloc(sizeof(*mesh->triangles) * mesh->max_triangles);
    return mesh;
}

/* Load mesh data from an OBJ file and return it.  Errors in the OBJ file
 * or unknown token types are silently ignored.
 */
mesh_t *mesh_load(const char *obj_filename)
{
    mesh_t *mesh = mesh_create();
    char buf[MAX_LINE_LENGTH];
    const char *bufp;
    size_t token_len;
    int line_number = 0;
    const material_t *current_material;
    char material_filename[MAX_LINE_LENGTH];
    char material_path[MAX_PATH];
    FILE *file = fopen(obj_filename, "r");
    int i;

    if(!file) return NULL;

    /* Get the relative path where the mtl file is (it should be in the same
       location as the obj file) */
    strcpy(material_path, obj_filename);
    for(i = strlen(obj_filename); material_path[i] != '\\' &&
        material_path[i] != '/' && i >= 0; i--)
    {
        material_path[i] = '\0';
    }

    /* Process each line from the OBJ file */
    while (fgets(buf, sizeof(buf), file))
    {
        /* Keeping track of the line number can help with debugging. */
        line_number++;

        /* Ignore comment lines */
        if (buf[0] == '#')
            continue ;

        /* Find the first space character, ignore line if not found */
        bufp = strchr(buf, ' ');
        if (!bufp)
            continue;

        token_len = bufp - buf;
        bufp++;          /* Skip past the space character */

        /* Do the appropriate action for each token type */
        if (strncmp(buf, "v", token_len) == 0)
            add_vertex_data(&mesh->vertices, bufp);
        else if (strncmp(buf, "vn", token_len) == 0)
            add_vertex_data(&mesh->normals, bufp);
        else if (strncmp(buf, "vt", token_len) == 0)
            add_vertex_data(&mesh->tex_coords, bufp);
        else if (strncmp(buf, "f", token_len) == 0)
            add_face_data(mesh, bufp, current_material);
        else if (strncmp(buf, "mtllib", token_len) == 0)
        {
            snprintf(material_filename, sizeof material_filename, "%s%.*s",
                material_path, (int) strcspn(bufp, "\r\n "), bufp);
            mesh->material = material_load(material_filename);
        }
        else if (strncmp(buf, "usemtl", token_len) == 0)
            current_material = material_find(mesh->material, bufp);
    }

    /* Pad the normals and tex coords arrays to be as large as the vertex
     * array. */
    pad_vertex_data(&mesh->normals, mesh->vertices.n_vertices);
    pad_vertex_data(&mesh->tex_coords, mesh->vertices.n_vertices);

    return mesh;
}

/* Free memory associated with a mesh. */
void mesh_destroy(mesh_t *mesh)
{
    material_destroy(mesh->material);
    free(mesh->triangles);
    free(mesh->vertices.vertices);
    free(mesh->normals.vertices);
    free(mesh->tex_coords.vertices);
    free(mesh);
}

/* Draw a mesh using immediate-mode OpenGL calls.  
 *
 * Loop through the array of triangles and call glVertex for each
 * vertex of each triangle.
 *
 * It would be more efficient to use vertex arrays or a display list, if
 * performance was an issue.
 */
void mesh_draw(mesh_t *mesh)
{
    int i;
    triangle_t *triangle;
    const material_t *material = NULL;

    if (!mesh->n_triangles)
        return;

    material = mesh->triangles[0].material;
    //material_apply(material);

    glBegin(GL_TRIANGLES);
    for (i = 0; i < mesh->n_triangles; i++)
    {
        triangle = &mesh->triangles[i];
        if (triangle->material != material)
        {
            glEnd();
            material = triangle->material;
            material_apply(material);
            glBegin(GL_TRIANGLES);
        }
        glNormal3fv((float *) &mesh->normals.vertices[triangle->p1]);
        glTexCoord3fv((float *) &mesh->tex_coords.vertices[triangle->p1]);
        glVertex3fv((float *) &mesh->vertices.vertices[triangle->p1]);
        glNormal3fv((float *) &mesh->normals.vertices[triangle->p2]);
        glTexCoord3fv((float *) &mesh->tex_coords.vertices[triangle->p2]);
        glVertex3fv((float *) &mesh->vertices.vertices[triangle->p2]);
        glNormal3fv((float *) &mesh->normals.vertices[triangle->p3]);
        glTexCoord3fv((float *) &mesh->tex_coords.vertices[triangle->p3]);
        glVertex3fv((float *) &mesh->vertices.vertices[triangle->p3]);
    }
    glEnd();
}

void draw_normal(vertex_t *vertex, vertex_t *normal)
{
    glVertex3fv((float *) vertex);
    glVertex3f(vertex->x + normal->x * 0.1f, 
               vertex->y + normal->y * 0.1f,
               vertex->z + normal->z * 0.1f);

}

void mesh_draw_normals(mesh_t *mesh)
{
    int i;

    glBegin(GL_LINES);
    for (i = 0; i < mesh->n_triangles; i++)
    {
        draw_normal(&mesh->vertices.vertices[mesh->triangles[i].p1],
                    &mesh->normals.vertices[mesh->triangles[i].n1]);
        draw_normal(&mesh->vertices.vertices[mesh->triangles[i].p2],
                    &mesh->normals.vertices[mesh->triangles[i].n2]);
        draw_normal(&mesh->vertices.vertices[mesh->triangles[i].p3],
                    &mesh->normals.vertices[mesh->triangles[i].n3]);
    }
    glEnd();
}
