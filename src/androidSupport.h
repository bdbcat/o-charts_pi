 
 /***************************************************************************
  * 
  * Project:  OpenCPN
  * Purpose:  OpenCPN Android support utilities
  * Author:   David Register
  *
  ***************************************************************************
  *   Copyright (C) 2015 by David S. Register                               *
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
  *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
  **************************************************************************/
 
 #ifndef ANDROIDSUPPORT_H
 #define ANDROIDSUPPORT_H
 
 #include "wx/wxprec.h"
 
 #ifndef  WX_PRECOMP
 #include "wx/wx.h"
 #endif //precompiled headers
 
#include "qdebug.h"

class ArrayOfCDI;
 
 #include <QString>
 
 bool AndroidUnzip(wxString zipFile, wxString destDir, int nStrip, bool bRemoveZip);
 wxString AndroidGetCacheDir();
 bool AndroidSecureCopyFile(wxString in, wxString out);
 int validateAndroidWriteLocation( const wxString& destination );

 void androidShowBusyIcon();
 void androidHideBusyIcon();

 void androidEnableRotation( void );
 void androidDisableRotation( void );
 wxSize getAndroidDisplayDimensions( void );


wxString callActivityMethod_s6s(const char *method, wxString parm1, wxString parm2="", wxString parm3="", wxString parm4="", wxString parm5="", wxString parm6="");
wxString callActivityMethod_s2s2i(const char *method, wxString parm1, wxString parm2, int parm3, int parm4);

 #endif
  