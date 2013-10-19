/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
 *                                                                         *
 *   This file is part of LuxRays.                                         *
 *                                                                         *
 *   LuxRays is free software; you can redistribute it and/or modify       *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   LuxRays is distributed in the hope that it will be useful,            *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   LuxRays website: http://www.luxrender.net                             *
 ***************************************************************************/

#ifndef _SLG_BSDFEVENTS_H
#define	_SLG_BSDFEVENTS_H

namespace slg {

typedef enum {
	NONE = 0,
	DIFFUSE = 1,
	GLOSSY = 2,
	SPECULAR = 4,
	REFLECT = 8,
	TRANSMIT = 16,

	ALL_TYPES = DIFFUSE | GLOSSY | SPECULAR,
	ALL_REFLECT = REFLECT | ALL_TYPES,
	ALL_TRANSMIT = TRANSMIT | ALL_TYPES,
	ALL = ALL_REFLECT | ALL_TRANSMIT
} BSDFEventType;

typedef int BSDFEvent;

}

#endif	/* _SLG_BSDFEVENTS_H */
