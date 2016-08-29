/***************************************************************************
* Copyright (C) 2015-2016 Alexander V. Popov.
* 
* This file is part of Tightly Associated Solvent Shell Extractor (TASSE) 
* source code.
* 
* TASSE is free software; you can redistribute it and/or modify it under 
* the terms of the GNU General Public License as published by the Free 
* Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
* 
* TASSE is distributed in the hope that it will be useful, but WITHOUT 
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
* for more details.
* 
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#ifndef TASSE_TRAITS_FILEUTILS_H
#define TASSE_TRAITS_FILEUTILS_H

#if defined(_WIN32)
typedef int64 fileOfs_t;
#define fu_seek	_fseeki64
#define fu_tell	_ftelli64
#else
typedef off64_t fileOfs_t;
#define fu_seek	fseeko64
#define fu_tell	ftello64
#endif

#endif //TASSE_TRAITS_FILEUTILS_H
