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
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#include <wx/artprov.h>

#include <sys/stat.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <fstream>
#include <string.h>
#include <vector>
#include <dirent.h>
#include <fcntl.h>

#include "tpmUtil.h"
#include "o-charts_pi.h"
#include "ocpn_plugin.h"

extern tpm_state_t g_TPMState;

#ifdef __WXGTK__
// UI Dialogs to manage TPM access

OCCopyableText::OCCopyableText(wxWindow* parent, const wxString &text)
    : wxTextCtrl(parent, wxID_ANY, text, wxDefaultPosition, wxDefaultSize,
        wxBORDER_NONE) {
    SetEditable(false);
    wxStaticText tmp(parent, wxID_ANY, text);
}

BEGIN_EVENT_TABLE(TPMMessageDialog, wxDialog)
EVT_BUTTON(wxID_OK, TPMMessageDialog::OnOK)
EVT_BUTTON(wxID_CANCEL, TPMMessageDialog::OnCancel)
EVT_CLOSE(TPMMessageDialog::OnClose)
END_EVENT_TABLE()

TPMMessageDialog::TPMMessageDialog(wxWindow* parent, const wxString& line1,
    const wxString& line2, const wxString& line3,
    const wxString& caption, long style)
    : wxDialog(parent, wxID_ANY, caption, wxDefaultPosition, wxDefaultSize,
        wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP) {
    m_style = style;
    wxFont* qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
    SetFont(*qFont);

    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* icon_text = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* icon_text2= new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* icon_text3= new wxBoxSizer(wxHORIZONTAL);

    // 2) text
    topsizer->Add(icon_text, 1, wxLEFT | wxRIGHT | wxTOP, 10);
    icon_text->Add(CreateTextSizer(line1), 0, wxLEFT, 10);

    topsizer->Add(icon_text2, 1, wxLEFT | wxRIGHT | wxTOP, 10);
    auto ctrl = new OCCopyableText(this, line2);
    ctrl->SetMinSize(wxSize(30 * wxWindow::GetCharWidth(), -1));
    icon_text2->Add(ctrl, 1, wxLEFT , 15 * wxWindow::GetCharWidth());

    topsizer->Add(icon_text3, 1, wxLEFT | wxRIGHT , 10);
    icon_text3->Add(CreateTextSizer(line3), 0, wxLEFT, 10);


    // 3) buttons
    int AllButtonSizerFlags =
        wxOK | wxCANCEL | wxYES | wxNO | wxHELP | wxNO_DEFAULT;
    int center_flag = wxEXPAND;
    if (style & wxYES_NO) center_flag = wxALIGN_CENTRE;
    wxSizer* sizerBtn = CreateSeparatedButtonSizer(style & AllButtonSizerFlags);
    if (sizerBtn) topsizer->Add(sizerBtn, 0, center_flag | wxALL, 10);

    SetAutoLayout(true);
    SetSizer(topsizer);

    topsizer->SetSizeHints(this);
    topsizer->Fit(this);
    wxSize size(GetSize());
    if (size.x < size.y * 3 / 2) {
        size.x = size.y * 3 / 2;
        SetSize(size);
    }

    Centre(wxBOTH | wxCENTER_FRAME);
}


void TPMMessageDialog::OnOK(wxCommandEvent& WXUNUSED(event)) {
    ret_val = wxID_OK;
    SetReturnCode(wxID_OK);
    EndModal(wxID_OK);
}

void TPMMessageDialog::OnCancel(wxCommandEvent& WXUNUSED(event)) {
    // Allow cancellation via ESC/Close button except if
    // only YES and NO are specified.
    if ((m_style & wxYES_NO) != wxYES_NO || (m_style & wxCANCEL)) {
        SetReturnCode(wxID_CANCEL);
        EndModal(wxID_CANCEL);
    }
}

void TPMMessageDialog::OnClose(wxCloseEvent& event) {
    SetReturnCode(wxID_CANCEL);
    EndModal(wxID_CANCEL);
}

static int TPMUIMessage1() {

    int dret = OCPNMessageBox_PlugIn(NULL,
        "Your system is capable of using a TPM chip for fingerprint creation.\n\
        Fingerprints using TPM are more secure and persistent than\n\
        other fingerprint methods, and are recommended for new users.\n\n\
        Use TPM fingerprint?",
        "o-charts Message", wxOK | wxCANCEL);

    return dret;
}

static int TPMUIMessage2() {

    int dret = OCPNMessageBox_PlugIn(NULL,
        "Your user account already has TPM access via the 'tss' group\n\
        Please exit OpenCPN, log out and log back in for it to take effect.\n",
        "o-charts Message", wxOK);

    return dret;
}

static int TPMUIMessage3() {

    int dret = wxID_OK;
    auto dialog = new TPMMessageDialog(NULL,
        "To enable TPM security features for o-charts, your linux user account\n\
        must be added to the 'tss' group.\n\n\
        Please exit OpenCPN and add your user name to 'tss' group, like this:",
        "$ sudo usermod -aG tss $USER",
        "Log out of linux desktop, wait 30 seconds, and log back in\n\
        to linux desktop in order to acrivate the new group setting.\n\n\
        Or, press CANCEL to disable o-charts support of TPM\n",
        "o-charts Message", wxOK | wxCANCEL);

    dialog->ShowModal();
    dret = dialog->GetRetVal();
    dialog->Destroy();
    return dret;
}


/* ============================
   UI / Policy STUBS
   ============================ */

// Called when TPM exists but user lacks permission.
// Should:
//  - Explain why TPM access is needed
//  - Ask for consent
//  - Trigger platform-specific group-add flow
//  - Tell user to re-login
void UI_RequestAddUserToTssGroup(const std::string& userName)
{
    // STUB: implement UI or CLI prompt here
    // Example text:
    //
    // "TPM hardware is present, but your user account does not have permission
    //  to access it. To enable TPM-backed security features, your account
    //  must be added to the 'tss' group. This requires admin approval.
    //  You will need to log out and log back in afterward."
}

// Optional: Informational message stub
void UI_Notify(const std::string& message)
{
    // STUB: log or display message
}

static bool DirExistsAndNonEmpty(const char* path)
{
    struct stat st{};
    if (stat(path, &st) != 0)
        return false;

    if (!S_ISDIR(st.st_mode))
        return false;

    DIR* d = opendir(path);
    if (!d)
    {
        // Permission denied ≠ no hardware
        if (errno == EACCES)
            return true;
        return false;
    }

    struct dirent* ent;
    while ((ent = readdir(d)) != nullptr)
    {
        if (strcmp(ent->d_name, ".") != 0 &&
            strcmp(ent->d_name, "..") != 0)
        {
            closedir(d);
            return true;
        }
    }

    closedir(d);
    return false;
}

static bool IsTPM2Device()
{
    const char* versionFile =
        "/sys/class/tpm/tpm0/tpm_version_major";

    int fd = open(versionFile, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
    {
        // Cannot read → assume TPM 2.0 on modern systems
        return true;
    }

    char buf[8]{};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0)
        return true;

    return buf[0] == '2';
}

static bool CanOpenReadWrite(const char* path)
{
    int fd = open(path, O_RDWR | O_CLOEXEC);
    if (fd >= 0)
    {
        close(fd);
        return true;
    }
    return false;
}

static std::string CurrentUserName()
{
    struct passwd* pw = getpwuid(getuid());
    return pw ? pw->pw_name : std::string{};
}

static bool UserHasEffectiveGroup(const char* groupName)
{
    struct group* grp = getgrnam(groupName);
    if (!grp)
        return false;

    int ngroups = getgroups(0, nullptr);
    if (ngroups <= 0)
        return false;

    std::vector<gid_t> groups(ngroups);
    if (getgroups(ngroups, groups.data()) < 0)
        return false;

    for (gid_t g : groups)
    {
        if (g == grp->gr_gid)
            return true;
    }

    return false;
}

static bool UserIsAssignedToGroup(const char* groupName,
    const std::string& userName)
{
    struct group* grp = getgrnam(groupName);
    if (!grp)
        return false;

    struct passwd* pw = getpwnam(userName.c_str());
    if (!pw)
        return false;

    int ngroups = 0;

    // First call to get required size
    getgrouplist(userName.c_str(), pw->pw_gid, nullptr, &ngroups);
    if (ngroups <= 0)
        return false;

    std::vector<gid_t> groups(ngroups);

    if (getgrouplist(userName.c_str(), pw->pw_gid,
            groups.data(), &ngroups) < 0)
        return false;

    for (gid_t g : groups)
    {
        if (g == grp->gr_gid)
            return true;
    }

    return false;
}


static bool IsRunningInFlatpak()
{
    // Flatpak guarantees this file exists
    return access("/.flatpak-info", F_OK) == 0;
}

bool DetectTPMAndPrepareAccess()
{
    if ((g_TPMState == TPMSTATE_REJECTED) ||
            (g_TPMState ==TPMSTATE_UNABLE))
        return false;

    if (g_TPMState == TPMSTATE_ACCPTED_VERIFIED)
        return true;

    const char* sysTpmPath = "/sys/class/tpm";
    const char* tpmrmPath  = "/dev/tpmrm0";
    const char* legacyPath = "/dev/tpm0";

    const std::string user = CurrentUserName();
    const bool inFlatpak = IsRunningInFlatpak();

    /* ---- Step 1: Detect TPM hardware ---- */
    if (!DirExistsAndNonEmpty(sysTpmPath))
    {
        UI_Notify("No TPM hardware detected or TPM disabled in firmware.");
        g_TPMState = TPMSTATE_UNABLE;
        return false;
    }

    /* ---- Step 2: Verify TPM generation ---- */
    if (!IsTPM2Device())
    {
        UI_Notify("TPM detected, but TPM 2.0 is required.");
        return false;
    }

    if (g_TPMState == TPMSTATE_UNKNOWN){
        int tpmr1 = TPMUIMessage1();
        if (tpmr1 != wxID_OK) {
            g_TPMState = TPMSTATE_REJECTED; // don't ask again
            UI_Notify("TPM detected, but rejected by user.");
            return false;
        }
    }

    g_TPMState = TPMSTATE_ACCPTED_UNVERIFIED;

    /* ---- Step 3: Try direct device access ---- */
    if (CanOpenReadWrite(tpmrmPath) ||
        CanOpenReadWrite(legacyPath))
    {
        g_TPMState = TPMSTATE_ACCPTED_VERIFIED;
        return true;
    }

    /* ---- Step 4: Flatpak mediation ---- */
    if (inFlatpak)
    {
        //UI_Notify(
        //    "TPM device detected, but access is blocked by Flatpak sandboxing.\n\n"
        //    "This application must be granted TPM device access, e.g.:\n"
        //    "  flatpak override --user --device=tpm <app-id>\n"
        //    "or run with:\n"
        //    "  flatpak run --device=tpm <app-id>"
        //);
        return false;
    }


    /* ---- Step 5: Group-based access resolution ---- */

#if 0
    if (UserHasEffectiveGroup("tss"))
    {
        UI_Notify(
            "TPM device detected, but access is still denied.\n"
            "This usually indicates a udev, ACL, or policy configuration issue."
        );
        return false;
    }
#endif

    if (UserIsAssignedToGroup("tss", user))
    {
        int tpmr2 = TPMUIMessage2();
        return false;  // ⬅ MUST terminate here
    }

    /* ---- Step 6: Request group membership ---- */

    int retv3 = TPMUIMessage3();
    if (retv3 != wxID_OK) {
        g_TPMState = TPMSTATE_REJECTED; // don't ask again
    }

    return false;
}

#endif      //__WXGTK__

bool TPMInit() {
#ifdef __WXGTK__
    return DetectTPMAndPrepareAccess();
#else
    return false;
#endif
}