/*
 * Copyright 2011 Vincent Sanders <vince@kyllikki.org>
 *
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 *
 * This file is part of png23d.
 *
 * Routines to output in scad polyhedron format
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "option.h"
#include "bitmap.h"
#include "mesh.h"
#include "mesh_index.h"
#include "mesh_gen.h"
#include "mesh_simplify.h"
#include "out_pscad.h"


/* ascii stl outout */
bool output_flat_scad_polyhedron(bitmap *bm, int fd, options *options)
{
    struct mesh *mesh;
    unsigned int ploop;
    unsigned int tloop; /* triangle loop */
    int xoff; /* x offset so 3d model is centered */
    int yoff; /* y offset so 3d model is centered */
    FILE *outf;
    uint32_t start_vcount;
    struct vertex *vertex;

    outf = fdopen(dup(fd), "w");

    mesh = new_mesh();
    if (mesh == NULL) {
        fprintf(stderr,"unable to create mesh\n");
        return false;
    }

    debug_mesh_init(mesh, options->meshdebug);

    if (mesh_from_bitmap(mesh, bm, options) == false) {
        fprintf(stderr,"unable to convert bitmap to mesh\n");
        return false;
    }

    start_vcount = mesh->fcount * 3; /* each facet has 3 vertex */

    INFO("Indexing %d vertices\n", start_vcount);
    index_mesh(mesh, options->bloom_complexity, options->vertex_complexity);

    INFO("Bloom filter prevented %d (%d%%) lookups\n",
         start_vcount - mesh->find_count,
         ((start_vcount - mesh->find_count) * 100) / start_vcount);
    INFO("Bloom filter had %d (%d%%) false positives\n",
         mesh->bloom_miss,
         (mesh->bloom_miss * 100) / (mesh->find_count));
    INFO("Indexing required %d lookups with mean search cost " D64F " comparisons\n",
         mesh->find_count,
         mesh->find_cost / mesh->find_count);

    if (options->optimise > 0) {
        INFO("Simplification of mesh with %d facets using %d unique verticies\n",
             mesh->fcount, mesh->vcount);

        simplify_mesh(mesh);

        INFO("Result mesh has %d facets using %d unique verticies\n",
             mesh->fcount, mesh->vcount);
    }

    xoff = (bm->width / 2);
    yoff = (bm->height / 2);

    fprintf(outf, "// Generated by png23d\n\n");

    fprintf(outf, "target_width = %f;\n", options->width);
    fprintf(outf, "target_depth = %f;\n\n", options->depth);

    fprintf(outf, "module image(sx,sy,sz) {\n scale([sx, sy, sz]) polyhedron(points = [\n");

    for (ploop = 0; ploop < mesh->vcount; ploop++) {
        vertex = vertex_from_index(mesh, ploop);
        fprintf(outf, "[%f,%f,%f],\n",
                vertex->pnt.x - xoff,
                vertex->pnt.y + yoff,
                vertex->pnt.z);
    }

    fprintf(outf, "], triangles = [\n");

    for (tloop = 0; tloop < mesh->fcount; tloop++) {
        fprintf(outf, "[%u,%u,%u],\n",
                mesh->f[tloop].i[0],
                mesh->f[tloop].i[1],
                mesh->f[tloop].i[2] );
    }


    fprintf(outf, "]); }\n\n");

    fprintf(outf, "image_width = %d;\n", bm->width);
    fprintf(outf, "image_height = %d;\n\n", bm->height);

    fprintf(outf, "image(target_width / image_width, target_width / image_width, target_depth);\n");


    free_mesh(mesh);

    fclose(outf);

    return true;
}
