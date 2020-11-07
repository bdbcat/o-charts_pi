/******************************************************************************
 *
 * Project:  
 * Purpose:   Plugin core
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2019 by David S. Register   *
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


#ifndef _OVALIDATOR_H_
#define _OVALIDATOR_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include "piScreenLog.h"

class ocValidator
{
public:
  ocValidator();
  ocValidator( itemChart *chart, oesu_piScreenLogContainer *log);
  ocValidator( itemChart *chart, oesu_piScreenLog *log);

  ~ocValidator();
  
  void init();
  void startValidation();
  void LogMessage( wxString msg);
  
private:
    
  itemChart *m_chart;
  oesu_piScreenLogContainer *m_clog;
  oesu_piScreenLog *m_log;
  
};  
  
  

#endif