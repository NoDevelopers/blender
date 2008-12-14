/**
 * $Id:
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The Original Code is Copyright (C) 2008 Blender Foundation.
 * All rights reserved.
 *
 * 
 * Contributor(s): Blender Foundation
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#ifndef ED_AREA_H
#define ED_AREA_H

/* the pluginnable API for export to editors */

/* calls for registering default spaces */
void ED_spacetype_outliner(void);
void ED_spacetype_time(void);
void ED_spacetype_view3d(void);
void ED_spacetype_ipo(void);
void ED_spacetype_image(void);
void ED_spacetype_node(void);
void ED_spacetype_buttons(void);
void ED_spacetype_info(void);
void ED_spacetype_file(void);
void ED_spacetype_sound(void);
void ED_spacetype_action(void);
void ED_spacetype_nla(void);
void ED_spacetype_script(void);
void ED_spacetype_text(void);
void ED_spacetype_sequencer(void);

#endif /* ED_AREA_H */

