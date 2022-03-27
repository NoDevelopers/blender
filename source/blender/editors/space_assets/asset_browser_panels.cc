/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup spassets
 */

#include <cstring>

#include "DNA_screen_types.h"
#include "DNA_space_types.h"

#include "BKE_screen.h"

#include "BLI_listbase.h"

#include "BLT_translation.h"

#include "ED_asset.h"

#include "MEM_guardedalloc.h"

#include "RNA_access.h"
#include "RNA_prototypes.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "WM_api.h"

#include "asset_browser_intern.hh"

static void assets_panel_asset_catalog_buttons_draw(const bContext *C, Panel *panel)
{
  bScreen *screen = CTX_wm_screen(C);
  SpaceAssets *assets_space = CTX_wm_space_assets(C);

  uiLayout *col = uiLayoutColumn(panel->layout, false);
  uiLayout *row = uiLayoutRow(col, true);

  PointerRNA assets_space_ptr;
  RNA_pointer_create(&screen->id, &RNA_SpaceAssetBrowser, assets_space, &assets_space_ptr);

  uiItemR(row, &assets_space_ptr, "asset_library_ref", 0, "", ICON_NONE);
  if (assets_space->asset_library_ref.type == ASSET_LIBRARY_LOCAL) {
    bContext *mutable_ctx = CTX_copy(C);
    if (WM_operator_name_poll(mutable_ctx, "asset.bundle_install")) {
      uiItemS(col);
      uiItemMenuEnumO(col,
                      mutable_ctx,
                      "asset.bundle_install",
                      "asset_library_ref",
                      "Copy Bundle to Asset Library...",
                      ICON_IMPORT);
    }
    CTX_free(mutable_ctx);
  }
  else {
    uiItemO(row, "", ICON_FILE_REFRESH, "ASSET_OT_library_refresh");
  }

  uiItemS(col);

  AssetLibrary *library = ED_assetlist_library_get(&assets_space->asset_library_ref);
  PropertyRNA *catalog_filter_prop = RNA_struct_find_property(&assets_space_ptr, "catalog_filter");

  asset_view_create_catalog_tree_view_in_layout(
      library, col, &assets_space_ptr, catalog_filter_prop, CTX_wm_message_bus(C));
}

void asset_browser_navigation_region_panels_register(ARegionType *art)
{
  PanelType *pt;

  pt = MEM_cnew<PanelType>("asset browser catalog buttons");
  strcpy(pt->idname, "FILE_PT_asset_catalog_buttons");
  strcpy(pt->label, N_("Asset Catalogs"));
  strcpy(pt->translation_context, BLT_I18NCONTEXT_DEFAULT_BPYRNA);
  pt->flag = PANEL_TYPE_NO_HEADER;
  pt->draw = assets_panel_asset_catalog_buttons_draw;
  BLI_addtail(&art->paneltypes, pt);
}
