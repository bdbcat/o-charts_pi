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


#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/dir.h>
#include <wx/filename.h>

#include <string>
#include "ochartShop.h"
#include "fpr.h"
#include "validator.h"
#include <tinyxml.h>
#include "ocpn_plugin.h"
#include "eSENCChart.h"
#include "chart.h"

extern wxString         g_systemName;
extern wxString         g_dongleName;
extern unsigned int     g_dongleSN;
extern wxString         g_sencutil_bin;
extern InProgressIndicator *g_ipGauge;


std::vector < itemChartData *> installedChartListData;
std::vector < itemChartDataKeys *> installedKeyFileData;



// Utility Functions, some using global static memory

bool LoadChartList( wxString fileName )
{
    // If there is a ChartList.XML, read it, parse it and prepare the indicated operations
    if(fileName.Length()){
            
                FILE *iFile = fopen(fileName.mb_str(), "rb");
                if (iFile > (void *)0){
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

                    //  Parse the XML
                    TiXmlDocument * doc = new TiXmlDocument();
                    doc->Parse( iText);
    
                    TiXmlElement * root = doc->RootElement();
                    if(root){
                        //wxString rootName = wxString::FromUTF8( root->Value() );
                        if(!strcmp(root->Value(), "chartList")){    //rootName.IsSameAs(_T("chartList"))){
            
                            TiXmlNode *child;
                            for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){

                                if(!strcmp(child->Value(), "Chart")){
                                    TiXmlNode *childChart = child->FirstChild();
                                    itemChartData *cdata = new itemChartData;
                                    installedChartListData.push_back(cdata);
                                    
                                    for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                                        const char *chartVal = childChart->Value();
                                        if(!strcmp(chartVal, "Name")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->Name = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "ID")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->ID = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "SE")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->SE = childVal->Value();                                
                                        }
                                        else if(!strcmp(chartVal, "RE")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->RE = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "ED")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->ED = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "Scale")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->Scale = childVal->Value();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    free( iText );
                    return true;
                }
        }
        return false;
}


bool LoadKeyFile( wxString fileName )
{
    // If there is a ChartList.XML, read it, parse it and prepare the indicated operations
    if(fileName.Length()){
            
                FILE *iFile = fopen(fileName.mb_str(), "rb");
                if (iFile > (void *)0){
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

                    //  Parse the XML
                    TiXmlDocument * doc = new TiXmlDocument();
                    doc->Parse( iText);
    
                    TiXmlElement * root = doc->RootElement();
                    if(root){
                        if(!strcmp(root->Value(), "keyList")){    
            
                            TiXmlNode *child;
                            for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){

                                if(!strcmp(child->Value(), "Chart")){
                                    TiXmlNode *childChart = child->FirstChild();
                                    itemChartDataKeys *cdata = new itemChartDataKeys;
                                    installedKeyFileData.push_back(cdata);
                                    
                                    for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                                        const char *chartVal = childChart->Value();
                                        if(!strcmp(chartVal, "Name")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->Name = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "ID")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->ID = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "RInstallKey")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->RIK = childVal->Value();
                                        }
                                        else if(!strcmp(chartVal, "FileName")){
                                            TiXmlNode *childVal = childChart->FirstChild();
                                            if(childVal)
                                                cdata->fileName = childVal->Value();
                                        }
                                    }
                                }
                            }
                        }
                    }
                    free( iText );
                    return true;
                }
        }
        return false;
}






//    OCPN / o-Charts Installation Validator
//_____________________________________________________________________________

ocValidator::ocValidator()
{
    init();
}


ocValidator::ocValidator( itemChart *chart, oesu_piScreenLogContainer *log)
{
    init();

    m_clog = log;
    m_chart = chart;
    
}

ocValidator::ocValidator( itemChart *chart, oesu_piScreenLog *log)
{
    init();

    m_log = log;
    m_chart = chart;
    
}

ocValidator::~ocValidator()
{
}

void ocValidator::init()
{
    m_clog = NULL;
    m_log = NULL;
    m_chart = NULL;

}

void ocValidator::LogMessage( wxString msg)
{
    if(m_log){
        m_log->LogMessage(msg);
    }
    if(m_clog){
        m_clog->LogMessage(msg);
    }

}

void ocValidator::startValidation()
{
    installedChartListData.clear();
    installedKeyFileData.clear();
    
    if(!m_chart){
        LogMessage(_("No chartset selected.\n"));
        return;
    }
        
    LogMessage(_("Starting chartset validation.\n"));

    LogMessage(_("  Checking server installation."));
    if(!g_sencutil_bin.Length()){
        LogMessage(_("  Server not installed."));
        return;
    }
        
    LogMessage(_("  Server execute command: ") + g_sencutil_bin);
    
    wxString serverVersion = GetServerVersionString();
    if(!serverVersion.Length()){
        LogMessage(_("  Server unavailable."));
        return;
    }
    else{
        LogMessage(_("  Server reports version:  ") + serverVersion);
    }

    LogMessage(_T("\n"));

#ifndef __OCPN__ANDROID__    
    // Check the dongle
    g_dongleName.Clear();
    LogMessage(_("  Checking dongle..."));
    if(IsDongleAvailable()){
        g_dongleSN = GetDongleSN();
        char sName[20];
        snprintf(sName, 19, "sgl%08X", g_dongleSN);

        g_dongleName = wxString(sName);
        LogMessage(_("  dongle found: ") + g_dongleName);
    }
    else
        LogMessage(_("  dongle not found: ") );
#endif
    
    LogMessage(_T("\n"));

    int nslot, qId;
    bool bAssignedToDongle = false;
    
    // If dongle is available, look for slot assigned
    if(g_dongleName.Len()){
        nslot = m_chart->GetSlotAssignedToInstalledDongle( qId );
        if( nslot >= 0){
            LogMessage(_("  Validating chartset assigned to dongle: ") + g_dongleName);
            bAssignedToDongle = true;
        }
        else{
            nslot = m_chart->GetSlotAssignedToSystem( qId );
            if( nslot >= 0 ){
                LogMessage(_("  Validating chartset assigned to systemName: ") + g_systemName);
            }
        }
    }
    else{
        nslot = m_chart->GetSlotAssignedToSystem( qId );
        if( nslot >= 0 ){
            LogMessage(_("  Validating chartset assigned to systemName: ") + g_systemName);
        }
    }
    
    if( nslot < 0 ){
        LogMessage(_("  Unable to validate unassigned chartset."));
        return;
    }
    
    // Find the slot
    itemSlot *slot = m_chart->GetSlotPtr( nslot, qId );
    if( !slot ){
        LogMessage(_("  Unable to find slot assignment."));
        return;
    }
        
    //  Extract the installation directory name from the slot
    std::string installLocation = slot->installLocation;
    if(!installLocation.size()){
        if(bAssignedToDongle)
            LogMessage(_("  Selected chartset is not installed for dongle."));
        else
            LogMessage(_("  Selected chartset is not installed."));
            
        return;
    }
   
    wxString installDirectory = wxString(installLocation.c_str());
    LogMessage(_("  Installation directory: ") + installDirectory );

    wxString dirType = _T("oeuSENC-");
    if(m_chart->chartType == CHART_TYPE_OERNC)
        dirType = _T("oeuRNC-");
    wxString chartsetBaseDirectory = installDirectory + wxFileName::GetPathSeparator() + dirType + wxString(m_chart->chartID.c_str()) + wxFileName::GetPathSeparator();
            
    LogMessage(_("  ProcessingChartList.XML") );

    wxString ChartListFile = chartsetBaseDirectory + _T("ChartList.XML");
            
    LogMessage(_("    Checking for ChartList.XML at: ") + ChartListFile );
    
    //  Verify the existence of ChartList.XML

    if(!::wxFileExists(ChartListFile)){
        LogMessage(_("    ChartList.XML not found in installation directory."));
        return;
    }
        
    LogMessage(_("    Loading ChartList.XML") );
    
    if( !LoadChartList( ChartListFile )){
        LogMessage(_("    Error loading ChartList.XML") );
        return;
    }
    
    // Good ChartList, so check for consistency....
    
    // Pass 1
    // For each chart in the ChartList, does an oeRNC file exist?
    LogMessage(_("    Checking ChartList for presence of each chart referenced") );

    wxString extension = _T(".oesu");
    if(m_chart->chartType == CHART_TYPE_OERNC)
        extension = _T(".oernc");
    
    for(unsigned int i = 0 ; i < installedChartListData.size() ; i++){
        itemChartData *cData = installedChartListData[i];
        
        wxString targetChartFILE = chartsetBaseDirectory + wxString(cData->ID.c_str()) + extension;
        if(!::wxFileExists(targetChartFILE) ){
            LogMessage(_("    Referenced chart is not present: ") + targetChartFILE );
            return;
        }
        
        if(g_ipGauge)
            g_ipGauge->Pulse();
        wxYield();

    }
    
    // Pass 2
    // Is every file found in the base directory referenced in the ChartList.XML file?
    LogMessage(_("    Checking all charts for reference in ChartList") );

    wxArrayString fileList;
    wxString searchString = _T("*") + extension;
    wxDir::GetAllFiles(chartsetBaseDirectory, &fileList, searchString );
    for( unsigned int i=0 ; i < fileList.GetCount() ; i++){
        wxFileName fn(fileList.Item(i));
        wxString fileBaseName = fn.GetName();
        
        bool bFoundInList = false;
        for(unsigned int j = 0 ; j < installedChartListData.size() ; j++){
            itemChartData *cData = installedChartListData[j];
            wxString chartListFileName = wxString(cData->ID.c_str());
            if( chartListFileName.IsSameAs(fileBaseName)){
                bFoundInList = true;
                break;
            }
        }
    
        if(bFoundInList)
            continue;
        else{
            LogMessage(_("    Found chart file not referenced in ChartList: ") + fileList.Item(i) );
            g_ipGauge->Stop();
            return;
        }
        
        if(g_ipGauge)
            g_ipGauge->Pulse();
        wxYield();

    }

    LogMessage(_("    ChartList.XML validates OK.") );

    LogMessage(_T("\n"));
    
    LogMessage(_("  Checking chart key files.") );
    
    // For each key file found...
    wxArrayString keyFileList;
    wxDir::GetAllFiles(chartsetBaseDirectory, &keyFileList, _T("*.XML") );
    for( unsigned int i=0 ; i < keyFileList.GetCount() ; i++){
        wxFileName keyfn(keyFileList.Item(i));
        wxString keyFileBaseName = keyfn.GetName();
        if(keyFileBaseName.StartsWith(_T("ChartList")))
            continue;

        LogMessage(_("    Loading keyFile: ") + keyFileList.Item(i) );
        installedKeyFileData.clear();
        if( !LoadKeyFile( keyFileList.Item(i) )){
            LogMessage(_("    Error loading keyFile: ")  + keyFileList.Item(i));
            g_ipGauge->Stop();
            return;
        }
        
        // There must be a key for each file found in the ChartList
        for( unsigned int j=0 ; j < fileList.GetCount() ; j++){
            wxFileName fn(fileList.Item(i));
            wxString fileBaseName = fn.GetName();
        
            bool bFoundInList = false;
        
            for(unsigned int k = 0 ; k < installedKeyFileData.size() ; k++){
                itemChartDataKeys *kData = installedKeyFileData[k];
                wxString keyListFileName = wxString(kData->fileName.c_str());
                if( keyListFileName.IsSameAs(fileBaseName)){
                    bFoundInList = true;
                    break;
                }
            }
    
        
            if(bFoundInList)
                continue;
            else{
                LogMessage(_("    Key not found for chart file: ") + fileList.Item(i) );
                g_ipGauge->Stop();
                return;
            }
            
            if(g_ipGauge)
                g_ipGauge->Pulse();
            wxYield();

        }
    }

    LogMessage(_("    Keyfiles validate OK.") );

    LogMessage(_T("\n"));

        

   

    // Try a read of some charts....
    LogMessage(_("  Starting chart test load.") );
    wxString countMsg;
    countMsg.Printf(_T(" %zu"), fileList.GetCount());
    LogMessage(_("  Chart count: ") + countMsg );
    
    for( unsigned int i=0 ; i < fileList.GetCount() ; i++){
        wxString chartFile = fileList.Item(i);
        //LogMessage(_("  Checking chart: ") + chartFile );

        int rv;
        if(m_chart->chartType == CHART_TYPE_OERNC){
            Chart_oeuRNC *ptestChart = new Chart_oeuRNC;
            rv = ptestChart->Init( chartFile, PI_HEADER_ONLY );
        }
        else{
            oesuChart *ptestChart = new oesuChart;
            rv = ptestChart->Init( chartFile, PI_HEADER_ONLY );
        }

        if(rv){
            wxString errMsg;
            errMsg.Printf(_T(" %d"), rv);
            LogMessage(_("  Chart load error ") + "( " + chartFile + " ) : " + errMsg );
            //return;
        }
        
        if(g_ipGauge)
            g_ipGauge->Pulse();
        wxYield();
    }

    LogMessage(_("  Chart test load OK.") );

    LogMessage(_T("\n"));
    
    LogMessage(_("Chartset installation validates OK.") );
    
    g_ipGauge->Stop();

}


 