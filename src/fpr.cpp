/******************************************************************************
 *
 * Project:  oesenc_pi
 * Purpose:  oesenc_pi Plugin core
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2016 by David S. Register   *
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

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/filename.h"
#include "wx/tokenzr.h"
#include "wx/dir.h"
#include <wx/textfile.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#include "fpr.h"
#include "ocpn_plugin.h"

#ifdef __OCPN__ANDROID__
#include "androidSupport.h"
#endif

extern wxString g_sencutil_bin;
extern wxString g_deviceInfo;
extern wxString g_systemName;

#ifdef __OCPN__ANDROID__
void androidGetDeviceName()
{
    if(!g_deviceInfo.Length())
        g_deviceInfo = callActivityMethod_vs("getDeviceInfo");

    wxStringTokenizer tkz(g_deviceInfo, _T("\n"));
    while( tkz.HasMoreTokens() )
    {
        wxString s1 = tkz.GetNextToken();
        if(wxNOT_FOUND != s1.Find(_T("Device"))){
            int a = s1.Find(_T(":"));
            if(wxNOT_FOUND != a){
                wxString b = s1.Mid(a+1).Trim(true).Trim(false);
                g_systemName = b;
            }
        }
    }

}
#endif

bool IsDongleAvailable()
{
#ifndef __OCPN__ANDROID__
///
#if 0
    wxString cmdls = _T("ls -la /Applications/OpenCPN.app/Contents/PlugIns/oernc_pi");

    wxArrayString lsret_array, lserr_array;
    wxExecute(cmdls, lsret_array, lserr_array );

    wxLogMessage(_T("ls results:"));
    for(unsigned int i=0 ; i < lsret_array.GetCount() ; i++){
        wxString line = lsret_array[i];
        wxLogMessage(line);
    }
#endif
///
    wxString cmd = g_sencutil_bin;
    cmd += _T(" -s ");                  // Available?

    wxArrayString ret_array, err_array;
    wxExecute(cmd, ret_array, err_array );

    wxLogMessage(_T("oexserverd results:"));
    for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
        wxString line = ret_array[i];
        wxLogMessage(line);

        if(line.IsSameAs(_T("1")))
            return true;
        if(line.IsSameAs(_T("0")))
            return false;
    }

    // Show error in log
    if(err_array.GetCount()){
        wxLogMessage(_T("oexserverd execution error:"));
        for(unsigned int i=0 ; i < err_array.GetCount() ; i++){
            wxString line = err_array[i];
            wxLogMessage(line);
        }
    }

#endif

    return false;
}

unsigned int GetDongleSN()
{
    unsigned int rv = 0;

#ifndef __OCPN__ANDROID__
    wxString cmd = g_sencutil_bin;
    cmd += _T(" -t ");                  // SN

    wxArrayString ret_array;
    wxExecute(cmd, ret_array, ret_array );

    for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
        wxString line = ret_array[i];
        long sn;
        line.ToLong(&sn, 10);
        rv = sn;
    }
#endif
    return rv;
}

wxString GetServerVersionString()
{
    wxString ver;
#ifndef __OCPN__ANDROID__

    wxString cmd = g_sencutil_bin;
    cmd += _T(" -a ");                  // Version

    wxArrayString ret_array;
    wxExecute(cmd, ret_array, ret_array );

    for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
        wxString line = ret_array[i];
        if(line.Length() > 2){
            ver = line;
            break;
        }
    }
#else
    ver = _T("1.0");
#endif
    return ver;
}

wxString getExpDate( wxString rkey)
{
    wxString ret;
#ifndef __OCPN__ANDROID__

    wxString cmd = g_sencutil_bin;
    cmd += _T(" -v ");                  // Expiry days
    cmd += rkey;

    wxArrayString ret_array;
    wxExecute(cmd, ret_array, ret_array );

    for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
        wxString line = ret_array[i];
        if(line.Length() > 2){
            ret = line;
            break;
        }
    }
#else
    ret = _T("9999 60");
#endif
    return ret;

}

wxString getFPR( bool bCopyToDesktop, bool &bCopyOK, bool bSGLock, wxString extra_info)
{
#ifndef __OCPN__ANDROID__

            wxString msg1;
            wxString fpr_file;
            wxString fpr_dir = *GetpPrivateApplicationDataLocation(); //GetWritableDocumentsDir();

#ifdef __WXMSW__

            //  On XP, we simply use the root directory, since any other directory may be hidden
            int major, minor;
            ::wxGetOsVersion( &major, &minor );
            if( (major == 5) && (minor == 1) )
                fpr_dir = _T("C:\\");
#endif

            if( fpr_dir.Last() != wxFileName::GetPathSeparator() )
                fpr_dir += wxFileName::GetPathSeparator();

            wxString cmd = g_sencutil_bin;
            if(extra_info.Length()){
                cmd += " -y ";
                cmd += "\'";
                cmd += extra_info;
                cmd += "\'";
            }

            if(bSGLock)
                cmd += _T(" -k ");                  // Make SGLock fingerprint
            else
                cmd += _T(" -g ");                  // Make fingerprint

#ifndef __WXMSW__
            cmd += _T("\"");
            cmd += fpr_dir;

            //cmd += _T("my fpr/");             // testing

            //            wxString tst_cedilla = wxString::Format(_T("my fpr copy %cCedilla/"), 0x00E7);       // testing French cedilla
            //            cmd += tst_cedilla;            // testing

            cmd += _T("\"");
#else
            cmd += wxString('\"');
            // use linux style path sep character for last sep, to allow double-quote on argument.
            wxString fpr_dir_mod = fpr_dir.BeforeLast('\\');
            fpr_dir_mod += "/";
            cmd += fpr_dir_mod;
            cmd += '\"';

            //            cmd += _T("my fpr\\");            // testing spaces in path

            //            wxString tst_cedilla = wxString::Format(_T("my%c\\"), 0x00E7);       // testing French cedilla
            //            cmd += tst_cedilla;            // testing
#endif
            wxLogMessage(_T("Create FPR command: ") + cmd);

            ::wxBeginBusyCursor();

            wxArrayString ret_array, err_array;
            wxExecute(cmd, ret_array, err_array );

            ::wxEndBusyCursor();
            wxLogMessage(_T("Create FPR oeaserverd results:"));

            bool berr = false;
            for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
                wxString line = ret_array[i];
                wxLogMessage(line);
                if(line.Upper().Find(_T("ERROR")) != wxNOT_FOUND){
                    berr = true;
                    break;
                }
                if(line.Upper().Find(_T("FPR")) != wxNOT_FOUND){
                    fpr_file = line.AfterFirst(':');
                }

            }

            if(err_array.GetCount()){
                wxLogMessage(_T("Create FPR oeaserverd execution error:"));
                for(unsigned int i=0 ; i < err_array.GetCount() ; i++){
                    wxString line = err_array[i];
                    wxLogMessage(line);
                }
            }

            if(!berr){
                if(fpr_file.IsEmpty()){                 // Probably dongle not present
                    fpr_file = _T("DONGLE_NOT_PRESENT");
                    return fpr_file;
                }
            }


            bool berror = false;

            if( bCopyToDesktop && !berr && fpr_file.Length()){

                bool bcopy = false;
                wxString sdesktop_path;

#ifdef __WXMSW__
                TCHAR desktop_path[MAX_PATH*2] = { 0 };
                bool bpathGood = false;
                HRESULT  hr;
                HANDLE ProcToken = NULL;
                OpenProcessToken( GetCurrentProcess(), TOKEN_READ, &ProcToken );

                hr = SHGetFolderPath( NULL,  CSIDL_DESKTOPDIRECTORY, ProcToken, 0, desktop_path);
                if (SUCCEEDED(hr))
                    bpathGood = true;

                CloseHandle( ProcToken );

                //                wchar_t *desktop_path = 0;
                //                bool bpathGood = false;

                //               if( (major == 5) && (minor == 1) ){             //XP
                //                    if(S_OK == SHGetFolderPath( (HWND)0,  CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, desktop_path))
                //                        bpathGood = true;


                //                 }
                //                 else{
                    //                     if(S_OK == SHGetKnownFolderPath( FOLDERID_Desktop, 0, 0, &desktop_path))
                //                         bpathGood = true;
                //                 }


                if(bpathGood){

                    char str[128];
                    wcstombs(str, desktop_path, 128);
                    wxString desktop_fpr(str, wxConvAuto());

                    sdesktop_path = desktop_fpr;
                    if( desktop_fpr.Last() != wxFileName::GetPathSeparator() )
                        desktop_fpr += wxFileName::GetPathSeparator();

                    wxFileName fn(fpr_file);
                    wxString desktop_fpr_file = desktop_fpr + fn.GetFullName();


                    wxString exe = _T("xcopy");
                    wxString parms = fpr_file.Trim() + _T(" ") + wxString('\"') + desktop_fpr + wxString('\"');
                    wxLogMessage(_T("FPR copy command: ") + exe + _T(" ") + parms);

                    const wchar_t *wexe = exe.wc_str(wxConvUTF8);
                    const wchar_t *wparms = parms.wc_str(wxConvUTF8);

                    if( (major == 5) && (minor == 1) ){             //XP
                        // For some reason, this does not work...
                        //8:43:13 PM: Error: Failed to copy the file 'C:\oc01W_1481247791.fpr' to '"C:\Documents and Settings\dsr\Desktop\oc01W_1481247791.fpr"'
                        //                (error 123: the filename, directory name, or volume label syntax is incorrect.)
                        //8:43:15 PM: oesenc fpr file created as: C:\oc01W_1481247791.fpr

                        bcopy = wxCopyFile(fpr_file.Trim(false), _T("\"") + desktop_fpr_file + _T("\""));
                    }
                    else{
                        ::wxBeginBusyCursor();

                        // Launch oexserverd as admin
                        SHELLEXECUTEINFO sei = { sizeof(sei) };
                        sei.lpVerb = L"runas";
                        sei.lpFile = wexe;
                        sei.hwnd = NULL;
                        sei.lpParameters = wparms;
                        sei.nShow = SW_SHOWMINIMIZED;
                        sei.fMask = SEE_MASK_NOASYNC;

                        if (!ShellExecuteEx(&sei))
                        {
                            DWORD dwError = GetLastError();
                            if (dwError == ERROR_CANCELLED)
                            {
                                // The user refused to allow privileges elevation.
                                OCPNMessageBox_PlugIn(NULL, _("Administrator priveleges are required to copy fpr.\n  Please try again...."), _("o-charts_pi Message"), wxOK);
                                berror = true;
                            }
                        }
                        else
                            bcopy = true;

                        ::wxEndBusyCursor();

                    }
                }
#endif            // MSW

#ifdef __WXOSX__
                wxFileName fn(fpr_file);
                wxString desktop_fpr_path = ::wxGetHomeDir() + wxFileName::GetPathSeparator() +
                _T("Desktop") + wxFileName::GetPathSeparator() + fn.GetFullName();

                bcopy =  ::wxCopyFile(fpr_file.Trim(false), desktop_fpr_path);
                sdesktop_path = desktop_fpr_path;
                msg1 += _T("\n\n OSX ");
#endif


                wxLogMessage(_T("oeRNC fpr file created as: ") + fpr_file);
                if(bCopyToDesktop && bcopy)
                    wxLogMessage(_T("oeRNC fpr file created in desktop folder: ") + sdesktop_path);

                if(bcopy)
                    bCopyOK = true;
        }
        else if(berr){
            wxLogMessage(_T("oernc_pi: oeaserverd results:"));
            for(unsigned int i=0 ; i < ret_array.GetCount() ; i++){
                wxString line = ret_array[i];
                wxLogMessage( line );
            }
            berror = true;
        }

        if(berror)
            return _T("");
        else
            return fpr_file;

#else   // android

        // Get XFPR from the oexserverd helper utility.
        //  The target binary executable
        wxString cmd = g_sencutil_bin;

//  Set up the parameter passed as the local app storage directory, and append "cache/" to it
        wxString dataLoc = *GetpPrivateApplicationDataLocation();
        wxFileName fn(dataLoc);
        wxString dataDir = fn.GetPath(wxPATH_GET_SEPARATOR);
        dataDir += _T("cache/");

        wxString rootDir = fn.GetPath(wxPATH_GET_SEPARATOR);

        //  Set up the parameter passed to runtime environment as LD_LIBRARY_PATH
        // This will be {dir of g_server_bin}
        wxFileName fnl(cmd);
//        wxString libDir = fnl.GetPath();
        wxString libDir = fnl.GetPath(wxPATH_GET_SEPARATOR) + _T("lib");

        wxLogMessage(_T("oernc_pi: Getting XFPR: Starting: ") + cmd );
        wxLogMessage(_T("oernc_pi: Getting XFPR: Parms: ") + rootDir + _T(" ") + dataDir + _T(" ") + libDir );

        wxString result = callActivityMethod_s6s("createProcSync4", cmd, _T("-q"), rootDir, _T("-g"), dataDir, libDir);

        wxLogMessage(_T("oernc_pi: Start Result: ") + result);

        bool berror = true;            //TODO

        // Find the file...
        wxArrayString files;
        wxString lastFile = _T("NOT_FOUND");
        time_t tmax = -1;
        size_t nf = wxDir::GetAllFiles(dataDir, &files, _T("*.fpr"), wxDIR_FILES);
        if(nf){
            for(size_t i = 0 ; i < files.GetCount() ; i++){
                //qDebug() << "looking at FPR file: " << files[i].mb_str();
                time_t t = ::wxFileModificationTime(files[i]);
                if(t > tmax){
                    tmax = t;
                    lastFile = files[i];
                }
            }
        }

        //qDebug() << "Selected FPR file: " << lastFile.mb_str();

        if(::wxFileExists(lastFile))
            return lastFile;
        else
            return _T("");

#endif
}

#ifdef __OCPN__ANDROID__
wxString androidGetSystemName()
{
    wxString detectedSystemName;
    wxString deviceInfo = callActivityMethod_vs("getDeviceInfo");

    wxStringTokenizer tkz(deviceInfo, _T("\n"));
    while( tkz.HasMoreTokens() )
    {
        wxString s1 = tkz.GetNextToken();
        if(wxNOT_FOUND != s1.Find(_T("systemName:"))){
            int pos = s1.Find(_T("systemName:"));
            detectedSystemName = s1.AfterFirst(':');
        }
    }
    qDebug() << "systemName by deviceInfo: " << detectedSystemName.mb_str();

    return detectedSystemName;

}
#endif
