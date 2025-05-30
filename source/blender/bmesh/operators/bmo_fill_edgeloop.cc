/* SPDX-FileCopyrightText: 2023 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup bmesh
 *
 * Fill discrete edge loop(s) with faces.
 */

#include "MEM_guardedalloc.h"

#include "bmesh.hh"

#include "intern/bmesh_operators_private.hh" /* own include */

#define VERT_USED 1
#define EDGE_MARK 2
#define ELE_OUT 4

void bmo_edgeloop_fill_exec(BMesh *bm, BMOperator *op)
{
  /* first collect an array of unique from the edges */
  const int tote = BMO_slot_buffer_len(op->slots_in, "edges");
  const int totv = tote; /* these should be the same */
  BMVert **verts = MEM_malloc_arrayN<BMVert *>(totv, __func__);

  BMVert *v;
  BMEdge *e;
  int i;
  bool ok = true;

  BMOIter oiter;

  const short mat_nr = BMO_slot_int_get(op->slots_in, "mat_nr");
  const bool use_smooth = BMO_slot_bool_get(op->slots_in, "use_smooth");

  /* 'VERT_USED' will be disabled, so enable and fill the array */
  i = 0;
  BMO_ITER (e, &oiter, op->slots_in, "edges", BM_EDGE) {
    BMIter viter;
    BMO_edge_flag_enable(bm, e, EDGE_MARK);
    BM_ITER_ELEM (v, &viter, e, BM_VERTS_OF_EDGE) {
      if (BMO_vert_flag_test(bm, v, VERT_USED) == false) {
        if (i == tote) {
          goto cleanup;
        }

        BMO_vert_flag_enable(bm, v, VERT_USED);
        verts[i++] = v;
      }
    }
  }

  /* we have a different number of verts to edges */
  if (i != tote) {
    goto cleanup;
  }

  /* loop over connected flagged edges and fill in faces,  this is made slightly more
   * complicated because there may be multiple disconnected loops to fill. */

  /* sanity check - that each vertex has 2 edge users */
  for (i = 0; i < totv; i++) {
    v = verts[i];
    /* count how many flagged edges this vertex uses */
    if (BMO_iter_elem_count_flag(bm, BM_EDGES_OF_VERT, v, EDGE_MARK, true) != 2) {
      ok = false;
      break;
    }
  }

  if (ok) {
    /* NOTE: in the case of multiple loops, this over-allocates (which is fine). */
    BMVert **f_verts = MEM_malloc_arrayN<BMVert *>(totv, __func__);
    BMIter eiter;

    /* build array of connected verts and edges */
    BMEdge *e_prev = nullptr;
    BMEdge *e_next = nullptr;
    int totv_used = 0;

    while (totv_used < totv) {
      for (i = 0; i < totv; i++) {
        v = verts[i];
        if (BMO_vert_flag_test(bm, v, VERT_USED)) {
          break;
        }
      }

      /* this should never fail, as long as (totv_used < totv)
       * we should have marked verts available */
      BLI_assert(BMO_vert_flag_test(bm, v, VERT_USED));

      /* watch it, 'i' is used for final face length */
      i = 0;
      do {
        /* we know that there are 2 edges per vertex so no need to check */
        BM_ITER_ELEM (e, &eiter, v, BM_EDGES_OF_VERT) {
          if (BMO_edge_flag_test(bm, e, EDGE_MARK)) {
            if (e != e_prev) {
              e_next = e;
              break;
            }
          }
        }

        /* fill in the array */
        f_verts[i] = v;
        BMO_vert_flag_disable(bm, v, VERT_USED);
        totv_used++;

        /* step over the edges */
        v = BM_edge_other_vert(e_next, v);
        e_prev = e_next;
        i++;
      } while (v != f_verts[0]);

      if (!BM_face_exists(f_verts, i)) {
        BMFace *f;

        /* don't use calc_edges option because we already have the edges */
        f = BM_face_create_ngon_verts(bm, f_verts, i, nullptr, BM_CREATE_NOP, true, false);
        BMO_face_flag_enable(bm, f, ELE_OUT);
        f->mat_nr = mat_nr;
        if (use_smooth) {
          BM_elem_flag_enable(f, BM_ELEM_SMOOTH);
        }
      }
    }
    MEM_freeN(f_verts);

    BMO_slot_buffer_from_enabled_flag(bm, op, op->slots_out, "faces.out", BM_FACE, ELE_OUT);
  }

cleanup:
  MEM_freeN(verts);
}
