/******************************************************************************
*
* Project:  o-charts_pi
* Purpose:
* Author:   David Register
*
***************************************************************************
*   Copyright (C) 2026 by David S. Register   *
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
#ifndef _OCHARTTPM_H_
#define _OCHARTTPM_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

bool DetectTPMAndPrepareAccess();

bool TPMInit();

class OCCopyableText : public wxTextCtrl {
public:
    OCCopyableText(wxWindow* parent, const wxString &text);
};

class TPMMessageDialog : public wxDialog {
public:
    TPMMessageDialog(wxWindow* parent, const wxString& line1,
        const wxString& line2, const wxString& line3,
        const wxString& caption = wxMessageBoxCaptionStr,
        long style = wxOK | wxCENTRE);

    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    int GetRetVal(void) { return ret_val; }

private:
    int ret_val;
    int m_style;
    DECLARE_EVENT_TABLE()
};
#endif

