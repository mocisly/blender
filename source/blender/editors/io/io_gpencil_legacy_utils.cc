/* SPDX-FileCopyrightText: 2020 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

/** \file
 * \ingroup editor/io
 */

#ifdef WITH_IO_GREASE_PENCIL

#  include "DNA_space_types.h"

#  include "BKE_context.hh"
#  include "BKE_screen.hh"

#  include "WM_api.hh"

#  include "io_gpencil_legacy.hh"

ARegion *get_invoke_region(bContext *C)
{
  bScreen *screen = CTX_wm_screen(C);
  if (screen == nullptr) {
    return nullptr;
  }
  ScrArea *area = BKE_screen_find_big_area(screen, SPACE_VIEW3D, 0);
  if (area == nullptr) {
    return nullptr;
  }

  ARegion *region = BKE_area_find_region_type(area, RGN_TYPE_WINDOW);

  return region;
}

View3D *get_invoke_view3d(bContext *C)
{
  bScreen *screen = CTX_wm_screen(C);
  if (screen == nullptr) {
    return nullptr;
  }
  ScrArea *area = BKE_screen_find_big_area(screen, SPACE_VIEW3D, 0);
  if (area == nullptr) {
    return nullptr;
  }
  if (area != nullptr) {
    return static_cast<View3D *>(area->spacedata.first);
  }

  return nullptr;
}

#endif /* WITH_IO_GREASE_PENCIL */
