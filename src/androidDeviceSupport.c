/******************************************************************************
 *
 * Project:  o-charts_pi
 * Purpose:  Android specific device support
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2024 by David S. Register                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 */

//  _Unwind_Resume required for Android-10, 64bit versions.
//  The subroutine is not found in llvm/clang libraries.
//  We do not use exceptions in native code, so sufficient to just stub
//  the subroutine


#ifdef __OCPN__ANDROID__
void
_Unwind_Resume (struct _Unwind_Exception *exception_object)
{
    abort ();
}
#endif
