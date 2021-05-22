/******************************************************************************
 * $Id: uKey.cpp,v 1.8 2010/06/21 01:54:37 bdbcat Exp $
 *
 * Project:  OpenCPN
 * Purpose:  
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2020 by David S. Register                               *
 *   $EMAIL$                                                               *
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

#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/fileconf.h>
 
#include "ocpn_plugin.h"
#include "tinyxml.h"
#include "o-charts_pi.h"

extern OKeyHash keyMapDongle;
extern OKeyHash keyMapSystem;

extern OKeyHash *pPrimaryKey;
extern OKeyHash *pAlternateKey;

wxString getChartInstallBase( wxString chartFileFullPath )
{
    //  Given a chart file full path, try to find the entry in the current core chart directory list that matches.
    //  If not found, return empty string
    
    wxString rv;
    wxArrayString chartDirsArray = GetChartDBDirArrayString();
    
    wxFileName fn(chartFileFullPath);
    bool bdone = false;
    while(!bdone){
        if(fn.GetDirCount() <= 2){
            return rv;
        }
        
        wxString val = fn.GetPath();
        
        for(unsigned int i=0 ; i < chartDirsArray.GetCount() ; i++){
            if(val.IsSameAs(chartDirsArray.Item(i))){
                rv = val;
                bdone = true;
                break;
            }
        }
        fn.RemoveLastDir();

    }
        
    return rv;
}    
        
        
        
        
bool parseKeyFile( wxString kfile, bool bDongle )
{
    FILE *iFile = fopen(kfile.mb_str(), "rb");
   
    if (iFile <= (void *) 0)
        return false;            // file error
        
    // compute the file length    
    fseek(iFile, 0, SEEK_END);
    size_t iLength = ftell(iFile);
    
    char *iText = (char *)calloc(iLength + 1, sizeof(char));
    
    // Read the file
    fseek(iFile, 0, SEEK_SET);
    size_t nread = 0;
    while (nread < iLength){
        nread += fread(iText + nread, 1, iLength - nread, iFile);
    }           
    fclose(iFile);

    wxFileName fn(kfile);
    
    //  Parse the XML
    TiXmlDocument * doc = new TiXmlDocument();
    doc->Parse( iText);
    
    TiXmlElement * root = doc->RootElement();
    if(!root)
        return false;                              // undetermined error??

    wxString RInstallKey, fileName;
    wxString rootName = wxString::FromUTF8( root->Value() );
    if(rootName.IsSameAs(_T("keyList"))){
            
        TiXmlNode *child;
        for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){
            wxString s = wxString::FromUTF8(child->Value());  //chart
            
            TiXmlNode *childChart = child->FirstChild();
            for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                const char *chartVal =  childChart->Value();
                            
                if(!strcmp(chartVal, "RInstallKey")){
                    TiXmlNode *childVal = childChart->FirstChild();
                    if(childVal){
                        RInstallKey = childVal->Value();
                        
                    }
                }
                if(!strcmp(chartVal, "FileName")){
                    TiXmlNode *childVal = childChart->FirstChild();
                    if(childVal){
                        fileName = childVal->Value();
                        
                    }
                }

            }
            wxString fileFullName = fn.GetPath( wxPATH_GET_VOLUME + wxPATH_GET_SEPARATOR ) + fileName + _T(".oesu");
            
            if(RInstallKey.Length() && fileName.Length()){
                if(bDongle){
                    OKeyHash::iterator search = keyMapDongle.find(fileFullName);
                    if (search == keyMapDongle.end()) {
                        keyMapDongle[fileFullName] = RInstallKey;
                    }
                }
                else{
                    OKeyHash::iterator search = keyMapSystem.find(fileFullName);
                    if (search == keyMapSystem.end()) {
                        keyMapSystem[fileFullName] = RInstallKey;
                    }
                }
            }
        }
        
        free( iText );
        return true;
    }           

    free( iText );
    return false;

}
    

bool loadKeyMaps( wxString file )
{
    wxString installBase = getChartInstallBase( file );
    wxLogMessage(_T("Computed installBase: ") + installBase);
    
    // Make a list of all XML or xml files found in the installBase directory of the chart itself.
    if(installBase.IsEmpty()){
        wxFileName fn(file);
        installBase = fn.GetPath();
    }

    wxArrayString xmlFiles;
    int nFiles = wxDir::GetAllFiles(installBase, &xmlFiles, _T("*.XML"));
    nFiles += wxDir::GetAllFiles(installBase, &xmlFiles, _T("*.xml"));
        
    //  Read and parse them all
    for(unsigned int i=0; i < xmlFiles.GetCount(); i++){
        wxString xmlFile = xmlFiles.Item(i);
        wxLogMessage(_T("Loading keyFile: ") + xmlFile);
        // Is this a dongle key file?
        if(wxNOT_FOUND != xmlFile.Find(_T("-sgl")))
            parseKeyFile(xmlFile, true);
        else
            parseKeyFile(xmlFile, false);
    }
    
 
    return true;
}


wxString getPrimaryKey(wxString file)
{
    if(pPrimaryKey){
        OKeyHash::iterator search = pPrimaryKey->find(file);
        if (search != pPrimaryKey->end()) {
            return search->second;
        }
        loadKeyMaps(file);

        search = pPrimaryKey->find(file);
        if (search != pPrimaryKey->end()) {
            return search->second;
        }
    }
    return wxString();
}

wxString getAlternateKey(wxString file)
{
    if(pAlternateKey){
        OKeyHash::iterator search = pAlternateKey->find(file);
        if (search != pAlternateKey->end()) {
            return search->second;
        }

        loadKeyMaps(file);

        search = pAlternateKey->find(file);
        if (search != pAlternateKey->end()) {
            return search->second;
        }
    }
    return wxString();
}

void SwapKeyHashes()
{
    OKeyHash *tmp = pPrimaryKey;
    pPrimaryKey = pAlternateKey;
    pAlternateKey = tmp;
}

    
 
