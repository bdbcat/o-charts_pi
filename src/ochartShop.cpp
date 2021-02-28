/******************************************************************************
 *
 * Project:  oesenc_pi
 * Purpose:  oesenc_pi Plugin core
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2018 by David S. Register                               *
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

#include <iostream>
#include <fstream>
#include <wx/fileconf.h>
#include <wx/uri.h>
#include "wx/tokenzr.h"
#include <wx/dir.h>
#include "wx/artprov.h"

#include "ochartShop.h"
#include "ocpn_plugin.h"
#ifdef __OCPN_USE_CURL__
    #include "wxcurl/wx/curl/http.h"
    #include "wxcurl/wx/curl/thread.h"
#endif    
#include <tinyxml.h>
#include "wx/wfstream.h"
#include <wx/zipstrm.h>
#include <memory>
#include "fpr.h"
#include "sha256.h"
#include "piScreenLog.h"
#include "validator.h"
#include "o-charts_pi.h"

#include <wx/arrimpl.cpp> 
WX_DEFINE_OBJARRAY(ArrayOfCharts);
WX_DEFINE_OBJARRAY(ArrayOfChartPanels);

#ifdef __OCPN__ANDROID__
#include "qdebug.h"
#include "androidSupport.h"
#endif

//  Static variables
ArrayOfCharts g_ChartArray;
std::vector<itemChart *> ChartVector;

wxString userURL(_T("https://o-charts.org/shop/index.php"));
wxString adminURL(_T("https://test.o-charts.org/shop/index.php"));
int g_timeout_secs = 10;

wxArrayString g_systemNameChoiceArray;
wxArrayString g_systemNameServerArray;
wxArrayString g_systemNameDisabledArray;
wxString g_lastSlotUUID;

extern int g_admin;
extern wxString g_lastEULAFile;
extern wxString g_versionString;
extern wxString g_systemOS;

extern wxString g_systemName;
extern wxString g_loginKey;
extern wxString g_loginUser;
extern wxString g_PrivateDataDir;
extern wxString g_debugShop;

shopPanel *g_shopPanel;
oesu_piScreenLogContainer *g_shopLogFrame;

#ifdef __OCPN_USE_CURL__
    OESENC_CURL_EvtHandler *g_CurlEventHandler;
    wxCurlDownloadThread *g_curlDownloadThread;
    wxFFileOutputStream *downloadOutStream;
#endif
    
bool g_chartListUpdatedOK;
wxString g_statusOverride;
wxString g_lastInstallDir;
wxString g_LastErrorMessage;
wxString g_lastQueryResult;

unsigned int    g_dongleSN;
wxString        g_dongleName;

itemSlot        *gtargetSlot;
itemChart       *gtargetChart;

InProgressIndicator *g_ipGauge;

double dl_now;
double dl_total;
time_t g_progressTicks;
int g_downloadChainIdentifier;
itemChart* g_chartProcessing;

long g_FileDownloadHandle;

extern OKeyHash keyMapDongle;
extern OKeyHash keyMapSystem;

extern o_charts_pi *g_pi;

#define ID_CMD_BUTTON_INSTALL 7783
#define ID_CMD_BUTTON_INSTALL_CHAIN 7784
#define ID_CMD_BUTTON_VALIDATE 7785


// Private class implementations

#define ANDROID_DIALOG_BACKGROUND_COLOR    wxColour(_T("#7cb0e9"))
#define ANDROID_DIALOG_BODY_COLOR         wxColour(192, 192, 192)


// Private class implementations

class  OERNCMessageDialog: public wxDialog
{
    
public:
    OERNCMessageDialog(wxWindow *parent, const wxString& message,
                      const wxString& caption = wxMessageBoxCaptionStr,
                      long style = wxOK|wxCENTRE);
    
    void OnYes(wxCommandEvent& event);
    void OnNo(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose( wxCloseEvent& event );
    
private:
    int m_style;
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(OERNCMessageDialog, wxDialog)
EVT_BUTTON(wxID_YES, OERNCMessageDialog::OnYes)
EVT_BUTTON(wxID_NO, OERNCMessageDialog::OnNo)
EVT_BUTTON(wxID_CANCEL, OERNCMessageDialog::OnCancel)
EVT_CLOSE(OERNCMessageDialog::OnClose)
END_EVENT_TABLE()


OERNCMessageDialog::OERNCMessageDialog( wxWindow *parent,
                                      const wxString& message,
                                      const wxString& caption,
                                      long style)
: wxDialog( parent, wxID_ANY, caption, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxSTAY_ON_TOP )
{
    SetBackgroundColour(ANDROID_DIALOG_BACKGROUND_COLOR);
    
    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( topsizer );
    
    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox( this, wxID_ANY, caption );
    
    wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer( itemStaticBoxSizer4Static, wxVERTICAL );
    topsizer->Add( itemStaticBoxSizer4, 0, wxEXPAND | wxALL, 5 );
    
    itemStaticBoxSizer4->AddSpacer(10);
    
    wxStaticLine *staticLine121 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxLI_HORIZONTAL);
    itemStaticBoxSizer4->Add(staticLine121, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    
    
    
    wxPanel *messagePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxBG_STYLE_ERASE );
    itemStaticBoxSizer4->Add(messagePanel, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    messagePanel->SetForegroundColour(wxColour(200, 200, 200));
    
    wxBoxSizer *boxSizercPanel = new wxBoxSizer(wxVERTICAL);
    messagePanel->SetSizer(boxSizercPanel);
    
    messagePanel->SetBackgroundColour(ANDROID_DIALOG_BODY_COLOR);

    
    m_style = style;
    wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
    SetFont( *qFont );
    
    
    wxBoxSizer *icon_text = new wxBoxSizer( wxHORIZONTAL );
    boxSizercPanel->Add( icon_text, 1, wxCENTER | wxLEFT|wxRIGHT|wxTOP, 10 );
    
    #if wxUSE_STATBMP
    // 1) icon
    if (style & wxICON_MASK)
    {
        wxBitmap bitmap;
        switch ( style & wxICON_MASK )
        {
            default:
                wxFAIL_MSG(_T("incorrect log style"));
                // fall through
                
            case wxICON_ERROR:
                bitmap = wxArtProvider::GetIcon(wxART_ERROR, wxART_MESSAGE_BOX);
                break;
                
            case wxICON_INFORMATION:
                bitmap = wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MESSAGE_BOX);
                break;
                
            case wxICON_WARNING:
                bitmap = wxArtProvider::GetIcon(wxART_WARNING, wxART_MESSAGE_BOX);
                break;
                
            case wxICON_QUESTION:
                bitmap = wxArtProvider::GetIcon(wxART_QUESTION, wxART_MESSAGE_BOX);
                break;
        }
        wxStaticBitmap *icon = new wxStaticBitmap(this, wxID_ANY, bitmap);
        icon_text->Add( icon, 0, wxCENTER );
    }
    #endif // wxUSE_STATBMP
    
    
    wxStaticText *textMessage = new wxStaticText( messagePanel, wxID_ANY, message );
    textMessage->Wrap(-1);
    icon_text->Add( textMessage, 0, wxALIGN_CENTER | wxLEFT, 10 );
    
   
    // 3) buttons
    int AllButtonSizerFlags = wxOK|wxCANCEL|wxYES|wxNO|wxHELP|wxNO_DEFAULT;
    int center_flag = wxEXPAND;
    if (style & wxYES_NO)
        center_flag = wxALIGN_CENTRE;
    wxSizer *sizerBtn = CreateSeparatedButtonSizer(style & AllButtonSizerFlags);
    if ( sizerBtn )
        topsizer->Add(sizerBtn, 0, center_flag | wxALL, 10 );
    
    SetAutoLayout( true );
    
    topsizer->SetSizeHints( this );
    topsizer->Fit( this );
    Centre( wxBOTH | wxCENTER_FRAME);
}

void OERNCMessageDialog::OnYes(wxCommandEvent& WXUNUSED(event))
{
    SetReturnCode(wxID_YES);
    EndModal( wxID_YES );
}

void OERNCMessageDialog::OnNo(wxCommandEvent& WXUNUSED(event))
{
    SetReturnCode(wxID_NO);
    EndModal( wxID_NO );
}

void OERNCMessageDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    // Allow cancellation via ESC/Close button except if
    // only YES and NO are specified.
    if ( (m_style & wxYES_NO) != wxYES_NO || (m_style & wxCANCEL) )
    {
        SetReturnCode(wxID_CANCEL);
        EndModal( wxID_CANCEL );
    }
}

void OERNCMessageDialog::OnClose( wxCloseEvent& event )
{
    SetReturnCode(wxID_CANCEL);
    EndModal( wxID_CANCEL );
}


int ShowOERNCMessageDialog(wxWindow *parent, const wxString& message,  const wxString& caption, long style)
{
#ifdef __OCPN__ANDROID__
    OERNCMessageDialog dlg( parent, message, caption, style);
    dlg.ShowModal();
    return dlg.GetReturnCode();
#else
    return OCPNMessageBox_PlugIn(parent, message, caption, style);
#endif    
}






#ifdef __OCPN_USE_CURL__
size_t wxcurl_string_write_UTF8(void* ptr, size_t size, size_t nmemb, void* pcharbuf)
{
    size_t iRealSize = size * nmemb;
    wxCharBuffer* pStr = (wxCharBuffer*) pcharbuf;
    
//     if(pStr)
//     {
//         wxString str = wxString(*pStr, wxConvUTF8) + wxString((const char*)ptr, wxConvUTF8);
//         *pStr = str.mb_str();
//     }
 
    if(pStr)
    {
#ifdef __WXMSW__        
        wxString str1a = wxString(*pStr);
        wxString str2 = wxString((const char*)ptr, wxConvUTF8, iRealSize);
        *pStr = (str1a + str2).mb_str();
#else        
        wxString str = wxString(*pStr, wxConvUTF8) + wxString((const char*)ptr, wxConvUTF8, iRealSize);
        *pStr = str.mb_str(wxConvUTF8);
#endif        
    }
 
    return iRealSize;
}

static int xferinfo(void *p,
                    curl_off_t dltotal, curl_off_t dlnow,
                    curl_off_t ultotal, curl_off_t ulnow)
{
    if(g_ipGauge){
        g_ipGauge->Pulse();
        wxYieldIfNeeded();
    }

    return 0;
}

class wxCurlHTTPNoZIP : public wxCurlHTTP
{
public:
    wxCurlHTTPNoZIP(const wxString& szURL = wxEmptyString,
               const wxString& szUserName = wxEmptyString,
               const wxString& szPassword = wxEmptyString,
               wxEvtHandler* pEvtHandler = NULL, int id = wxID_ANY,
               long flags = wxCURL_DEFAULT_FLAGS);
    
   ~wxCurlHTTPNoZIP();
    
   bool Post(wxInputStream& buffer, const wxString& szRemoteFile /*= wxEmptyString*/);
   bool Post(const char* buffer, size_t size, const wxString& szRemoteFile /*= wxEmptyString*/);
   std::string GetResponseBody() const;
   
protected:
    void SetCurlHandleToDefaults(const wxString& relativeURL);
    
};

wxCurlHTTPNoZIP::wxCurlHTTPNoZIP(const wxString& szURL /*= wxEmptyString*/, 
                       const wxString& szUserName /*= wxEmptyString*/, 
                       const wxString& szPassword /*= wxEmptyString*/, 
                       wxEvtHandler* pEvtHandler /*= NULL*/, 
                       int id /*= wxID_ANY*/,
                       long flags /*= wxCURL_DEFAULT_FLAGS*/)
: wxCurlHTTP(szURL, szUserName, szPassword, pEvtHandler, id, flags)

{
}

wxCurlHTTPNoZIP::~wxCurlHTTPNoZIP()
{
    ResetPostData();
}

void wxCurlHTTPNoZIP::SetCurlHandleToDefaults(const wxString& relativeURL)
{
    wxCurlBase::SetCurlHandleToDefaults(relativeURL);
    
    SetOpt(CURLOPT_ENCODING, "identity");               // No encoding, plain ASCII
    
    if(m_bUseCookies)
    {
        SetStringOpt(CURLOPT_COOKIEJAR, m_szCookieFile);
    }
}

bool wxCurlHTTPNoZIP::Post(const char* buffer, size_t size, const wxString& szRemoteFile /*= wxEmptyString*/)
{
    wxMemoryInputStream inStream(buffer, size);
    
    return Post(inStream, szRemoteFile);
}

bool wxCurlHTTPNoZIP::Post(wxInputStream& buffer, const wxString& szRemoteFile /*= wxEmptyString*/)
{
    curl_off_t iSize = 0;
    
    if(m_pCURL && buffer.IsOk())
    {
        SetCurlHandleToDefaults(szRemoteFile);
        
        SetHeaders();
        iSize = buffer.GetSize();
        
        if(iSize == (~(ssize_t)0))      // wxCurlHTTP does not know how to upload unknown length streams.
            return false;
        
        SetOpt(CURLOPT_POST, TRUE);
        SetOpt(CURLOPT_POSTFIELDSIZE_LARGE, iSize);
        SetStreamReadFunction(buffer);
        
        //  Use a private data write trap function to handle UTF8 content
        //SetStringWriteFunction(m_szResponseBody);
        SetOpt(CURLOPT_WRITEFUNCTION, wxcurl_string_write_UTF8);         // private function
        SetOpt(CURLOPT_WRITEDATA, (void*)&m_szResponseBody);
        
        curl_easy_setopt(m_pCURL, CURLOPT_XFERINFOFUNCTION, xferinfo);
        curl_easy_setopt(m_pCURL, CURLOPT_NOPROGRESS, 0L);
        
        if(Perform())
        {
            ResetHeaders();
            return IsResponseOk();
        }
    }
    
    return false;
}

std::string wxCurlHTTPNoZIP::GetResponseBody() const
{
#ifndef OCPN_ARMHF
    wxString s = wxString((const char *)m_szResponseBody, wxConvLibc);
    return std::string(s.mb_str());
    
#else    
    return std::string((const char *)m_szResponseBody);
#endif
    
}
#endif          //__OCPN_USE_CURL__


//    ChartSetData()
//------------------------------------------------------------------------------------------
ChartSetData::ChartSetData( std::string fileXML)
{
    // Open and parse the given file
    FILE *iFile = fopen(fileXML.c_str(), "rb");
   
    if (iFile <= (void *)0)
        return;            // file error
        
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
    const char *rr = doc->Parse( iText);
    
    TiXmlElement * root = doc->RootElement();
    if(!root){
        free(iText);
        return;                              // undetermined error??
    }

    wxString rootName = wxString::FromUTF8( root->Value() );
    if(rootName.IsSameAs(_T("chartList"))){
            
        TiXmlNode *child;
        for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){
            itemChartData *cdata = new itemChartData();
            chartList.push_back(cdata);
            
            wxString s = wxString::FromUTF8(child->Value());  //chart
            
            TiXmlNode *childChart = child->FirstChild();
            for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                const char *chartVal =  childChart->Value();
                
/*                
    <Name>Kemer Turkiz Marina</Name>
    <ID>G40-E-2014</ID>
    <SE></SE>
    <RE>01</RE>
    <ED>2019-06-03</ED>
    <Scale>17500</Scale>
*/
                            
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
        
        free( iText );
        return;
    }           

    free( iText );
    return;

}

bool ChartSetData::RemoveChart( std::string fileNameKap )
{
    size_t nl = fileNameKap.find(".kap");
    std::string search;
    if(nl != std::string::npos)
        search = fileNameKap.substr(0, nl);
    else{
        nl = fileNameKap.find(".KAP");
        if(nl != std::string::npos)
            search = fileNameKap.substr(0, nl);
    }

        // Search for the chart

    for(unsigned int i=0 ; i < chartList.size() ; i++){
        itemChartData *pd = chartList[i];
        if(!search.compare(pd->ID)){
            chartList.erase(chartList.begin()+i);
            delete pd;
            return true;
        }
    }
    
    return false;
}

bool ChartSetData::AddChart(itemChartData *cdata){

    //Search for an existing entry, by ID

    itemChartData *target = NULL;
    for(unsigned int i=0 ; i < chartList.size() ; i++){
        itemChartData *pd = chartList[i];
        if(!pd->ID.compare(cdata->ID)){
            target = pd;
            break;
        }
    }
    
    // If no current chart item found, create one and add it to the vector
    if(!target){
        target = new itemChartData;
        chartList.push_back(target);
    }
    
    target->Name = cdata->Name;
    target->ID = cdata->ID;
    target->SE = cdata->SE;
    target->RE = cdata->RE;
    target->ED = cdata->ED;
    target->Scale = cdata->Scale;
        
    return true;
    
}

bool ChartSetData::WriteFile( std::string fileName)
{
    TiXmlDocument doc;  
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );  
    doc.LinkEndChild( decl );  
 
    TiXmlElement * root = new TiXmlElement( "chartList" );  
    doc.LinkEndChild( root );  
    root->SetAttribute("version", "1.0");
    root->SetAttribute("creator", "OpenCPN");
    root->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    root->SetAttribute("xmlns:opencpn", "http://www.opencpn.org");

    //Write the editionTag
    TiXmlElement *itemEdition = new TiXmlElement( "Edition" );  
    itemEdition->LinkEndChild( new TiXmlText( editionTag.c_str()) );  
    root->LinkEndChild( itemEdition );  
    
    
    for(size_t i=0 ; i < chartList.size() ;i++){
        TiXmlElement * chart = new TiXmlElement( "Chart" );  
        root->LinkEndChild( chart );  
    
        TiXmlElement *item = new TiXmlElement( "Name" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->Name.c_str()) );  
        chart->LinkEndChild( item );  

        item = new TiXmlElement( "ID" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->ID.c_str()) );  
        chart->LinkEndChild( item );  
        
        item = new TiXmlElement( "SE" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->SE.c_str()) );  
        chart->LinkEndChild( item );  
        
        item = new TiXmlElement( "RE" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->RE.c_str()) );  
        chart->LinkEndChild( item );  
        
        item = new TiXmlElement( "ED" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->ED.c_str()) );  
        chart->LinkEndChild( item );  

        item = new TiXmlElement( "Scale" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->Scale.c_str()) );  
        chart->LinkEndChild( item );  

    }
    
    return(doc.SaveFile( fileName.c_str() ));  
    
        
/*
<?xml version="1.0"?>
<chartList version="1.0" creator="OpenCPN" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:opencpn="http://www.opencpn.org">
  <Chart>
    <Name>Chart AB 16 Treasure Cay</Name>
    <ID>AB16</ID>
    <SE>2017</SE>
    <RE>01</RE>
    <ED>2019-04-22</ED>
    <Scale>20000</Scale>
  </Chart>
*/

}



//    ChartSetKeys()
//------------------------------------------------------------------------------------------
ChartSetKeys::ChartSetKeys( std::string fileXML)
{
    m_bOK = Load( fileXML );
    
}

bool ChartSetKeys::Load( std::string fileXML)
{
    // Open and parse the given file
    FILE *iFile = fopen(fileXML.c_str(), "rb");
   
    if (iFile <= (void *)0)
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

    
    //  Parse the XML
    TiXmlDocument * doc = new TiXmlDocument();
    const char *rr = doc->Parse( iText);
    
    TiXmlElement * root = doc->RootElement();
    if(!root){
        free(iText);
        return false;                              // undetermined error??
    }

    wxString rootName = wxString::FromUTF8( root->Value() );
    if(rootName.IsSameAs(_T("keyList"))){
            
        TiXmlNode *child;
        for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){

            itemChartDataKeys *cdata = new itemChartDataKeys();
            chartList.push_back(cdata);
           
            TiXmlNode *childChart = child->FirstChild();
            for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                const char *chartVal =  childChart->Value();
  
/*
    <Name>Chart AB 6 Walkers &amp; Grand Cays</Name>
    <FileName>AB6</FileName>
    <ID>AB6</ID>
    <RInstallKey>AF2A6D76</RInstallKey>
*/
                if(!strcmp(chartVal, "RInstallKey")){
                    TiXmlNode *childVal = childChart->FirstChild();
                    if(childVal)
                        cdata->RIK = childVal->Value();
                }
                if(!strcmp(chartVal, "FileName")){
                    TiXmlNode *childVal = childChart->FirstChild();
                    if(childVal)
                        cdata->fileName = childVal->Value();
                }
                if(!strcmp(chartVal, "Name")){
                    TiXmlNode *childVal = childChart->FirstChild();
                    if(childVal)
                        cdata->Name = childVal->Value();
                }
                if(!strcmp(chartVal, "ID")){
                    TiXmlNode *childVal = childChart->FirstChild();
                    if(childVal)
                        cdata->ID = childVal->Value();
                }

            }
        }
        
        free( iText );
        m_bOK = true;
        return true;
    }           

    free( iText );
    m_bOK = true;
    return true;

}

bool ChartSetKeys::RemoveKey( std::string fileNameKap )
{
    size_t nl = fileNameKap.find(".kap");
    std::string search;
    if(nl != std::string::npos)
        search = fileNameKap.substr(0, nl);
    else{
        nl = fileNameKap.find(".KAP");
        if(nl != std::string::npos)
            search = fileNameKap.substr(0, nl);
    }

        // Search for the chart

    for(unsigned int i=0 ; i < chartList.size() ; i++){
        itemChartDataKeys *pd = chartList[i];
        if(!search.compare(pd->ID)){
            chartList.erase(chartList.begin()+i);
            delete pd;
            return true;
        }
    }
    
    return false;
}

bool ChartSetKeys::AddKey(itemChartDataKeys *kdata)
{
    //Search for an existing entry, by ID

    itemChartDataKeys *target = NULL;
    for(unsigned int i=0 ; i < chartList.size() ; i++){
        itemChartDataKeys *pd = chartList[i];
        if(!pd->ID.compare(kdata->ID)){
            target = pd;
            break;
        }
    }
    
    // If no current chart item found, create one and add it to the vector
    if(!target){
        target = new itemChartDataKeys;
        chartList.push_back(target);
    }
    
    target->Name = kdata->Name;
    target->ID = kdata->ID;
    target->fileName = kdata->fileName;
    target->RIK = kdata->RIK;
        
    return true;

}

bool ChartSetKeys::WriteFile( std::string fileName)
{
    TiXmlDocument doc;  
    TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );  
    doc.LinkEndChild( decl );  
 
    TiXmlElement * root = new TiXmlElement( "keyList" );  
    doc.LinkEndChild( root );  
    root->SetAttribute("version", "1.0");
    root->SetAttribute("creator", "OpenCPN");
    root->SetAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    root->SetAttribute("xmlns:opencpn", "http://www.opencpn.org");

    for(size_t i=0 ; i < chartList.size() ;i++){
        TiXmlElement * chart = new TiXmlElement( "Chart" );  
        root->LinkEndChild( chart );  
    
        TiXmlElement *item = new TiXmlElement( "Name" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->Name.c_str()) );  
        chart->LinkEndChild( item );  

        item = new TiXmlElement( "FileName" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->fileName.c_str()) );  
        chart->LinkEndChild( item );  
        
        item = new TiXmlElement( "ID" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->ID.c_str()) );  
        chart->LinkEndChild( item );  
        
        item = new TiXmlElement( "RInstallKey" );  
        item->LinkEndChild( new TiXmlText( chartList[i]->RIK.c_str()) );  
        chart->LinkEndChild( item );  
    }
    
/*
    <Chart>
    <Name>Chart AB 6 Walkers &amp; Grand Cays</Name>
    <FileName>AB6</FileName>
    <ID>AB6</ID>
    <RInstallKey>AF2A6D</RInstallKey>
    </Chart>
*/    
    return(doc.SaveFile( fileName.c_str() ));  
    
}

// itemChart
//------------------------------------------------------------------------------------------

void itemChart::Update(itemChart *other)
{
    orderRef = other->orderRef;
    purchaseDate = other->purchaseDate;
    expDate = other->expDate;
    chartName = other->chartName;
    chartID = other->chartID;
    serverChartEdition = other->serverChartEdition;
    editionDate = other->editionDate;
    thumbLink = other->thumbLink;

    maxSlots = other->maxSlots;
    bExpired = other->bExpired;
    
    baseChartListArray.Clear();
    for(unsigned int i = 0 ; i < other->baseChartListArray.GetCount() ; i++)
        baseChartListArray.Add(other->baseChartListArray.Item(i));

    updateChartListArray.Clear();
    for(unsigned int i = 0 ; i < other->updateChartListArray.GetCount() ; i++)
        updateChartListArray.Add(other->baseChartListArray.Item(i));
    
    std::vector<itemQuantity> quantityListTemp;

    for(unsigned int i = 0 ; i < other->quantityList.size() ; i++){
        
        itemQuantity Qty;
        Qty.quantityId = other->quantityList[i].quantityId;
        
        for(unsigned int j = 0 ; j < other->quantityList[i].slotList.size() ; j++){
            itemSlot *slot = GetSlotPtr( wxString(other->quantityList[i].slotList[j]->slotUuid.c_str()) );
            if(!slot)
                slot = new itemSlot;
            
            slot->slotUuid = other->quantityList[i].slotList[j]->slotUuid;
            slot->assignedSystemName = other->quantityList[i].slotList[j]->assignedSystemName;
            if(!slot->lastRequested.size())
                slot->lastRequested = other->quantityList[i].slotList[j]->lastRequested;
            if(!slot->installLocation.size())
                slot->installLocation = other->quantityList[i].slotList[j]->installLocation;
            
            Qty.slotList.push_back(slot);
        }
        
        quantityListTemp.push_back(Qty);
    }

    
    quantityList.clear();
    for(unsigned int i = 0 ; i < quantityListTemp.size() ; i++){
        itemQuantity Qty = quantityListTemp[i];
        quantityList.push_back(Qty);
    }
        

    
}

int s_dlbusy;

bool itemChart::isThumbnailReady()
{       // Look for cached copy
    wxString fileKey = _T("ChartImage-");
    fileKey += chartID;
    fileKey += _T(".jpg");
 
    wxString file = g_PrivateDataDir + fileKey;
    return ::wxFileExists(file);
}
 
    
wxBitmap& itemChart::GetChartThumbnail(int size, bool bDL_If_Needed)
{
#if 1    
    if(!m_ChartImage.IsOk()){

        // Look for cached copy
        wxString fileKey = _T("ChartImage-");
        fileKey += chartID;
        fileKey += _T(".jpg");
 
        wxString file = g_PrivateDataDir + fileKey;
        if(::wxFileExists(file)){
            m_ChartImage = wxImage( file, wxBITMAP_TYPE_ANY);
        }
        else if(bDL_If_Needed){
            int iResponseCode = 0;
            if(g_chartListUpdatedOK && thumbLink.length()){  // Do not access network until after first "getList"
#ifndef __OCPN__ANDROID__                
                wxCurlHTTP get;
                get.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
                bool getResult = get.Get(file, wxString(thumbLink));

            // get the response code of the server
                
                get.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
#else
                if(!s_dlbusy){
                    
                    wxString fileKeytmp = _T("ChartImage-");
                    fileKeytmp += chartID;
                    fileKeytmp += _T(".tmp");
 
                    wxString filetmp = g_PrivateDataDir + fileKeytmp;

                    wxString file_URI = _T("file://") + filetmp;
                    
                    s_dlbusy = 1;
                    _OCPN_DLStatus ret = OCPN_downloadFile( wxString(thumbLink), file_URI, _T(""), _T(""), wxNullBitmap, NULL /*g_shopPanel*/, 0/*OCPN_DLDS_DEFAULT_STYLE*/, 15);

                    wxLogMessage(_T("DLRET"));
                    qDebug() << "DL done";
                    if(OCPN_DL_NO_ERROR == ret){
                        wxCopyFile(filetmp, file);
                        iResponseCode = 200;
                    }
                    else
                        iResponseCode = ret;
                    
                    wxRemoveFile( filetmp );
                    s_dlbusy = 0;
                }
                else
                    qDebug() << "Busy";
                
#endif
                
            
                if(iResponseCode == 200){
                    if(::wxFileExists(file)){
                        m_ChartImage = wxImage( file, wxBITMAP_TYPE_ANY);
                    }
                }
            }
        }
    }
#endif

    if(m_ChartImage.IsOk()){
        int scaledHeight = size;
        int scaledWidth = m_ChartImage.GetWidth() * scaledHeight / m_ChartImage.GetHeight();
        wxImage scaledImage = m_ChartImage.Rescale(scaledWidth, scaledHeight);
        m_bm = wxBitmap(scaledImage);
        
        return m_bm;
    }
    else{
        wxImage img(size, size);
        unsigned char *data = img.GetData();
        for(int i=0 ; i < size * size * 3 ; i++)
            data[i] = 200;
        
        m_bm = wxBitmap(img);   // Grey bitmap
        return m_bm;
    }
    
}

wxString itemChart::GetDisplayedChartEdition()
{
    if(GetActiveSlot()){
        return wxString(GetActiveSlot()->installedEdition.c_str());
    }
    else
        return wxEmptyString;
}


bool itemChart::isChartsetAssignedToSystemKey(wxString key)
{
    if(!key.Length())
        return false;
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(!strcmp(key.mb_str(), slot->assignedSystemName.c_str())){
                return true;
            }
        }
    }
    
    return false;
    
}


int itemChart::getChartAssignmentCount()
{
    int rv = 0;
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(strlen(slot->slotUuid.c_str())){
                rv++;
            }
        }
    }
    return rv;
}

bool itemChart::isUUIDAssigned( wxString UUID)
{
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(!strcmp(slot->slotUuid.c_str(), UUID.mb_str())){
                return true;
            }
        }
    }
    return false;
}

itemSlot *itemChart::GetSlotPtr( wxString UUID )
{
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(!strcmp(slot->slotUuid.c_str(), UUID.mb_str())){
                return slot;
            }
        }
    }
    return NULL;
 
}

itemSlot *itemChart::GetSlotPtr( int slot, int qId )
{
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        if(Qty.quantityId == qId){
            return Qty.slotList[slot];
        }
    }
    return NULL;
}


bool itemChart::isChartsetFullyAssigned()
{
    int qtyIndex = -1;
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
            itemQuantity Qty = quantityList[i];
            if(Qty.slotList.size() < maxSlots){
                qtyIndex = i;
                break;
            }
    }
        
    return (qtyIndex < 0);
}

bool itemChart::isChartsetExpired()
{
    
    return bExpired;
}

bool itemChart::isChartsetAssignedToAnyDongle() {
    
    int tmpQ;
    if(GetSlotAssignedToInstalledDongle( tmpQ ) >= 0)
        return true;
    return false;
}

   
  

bool itemChart::isChartsetDontShow()
{
    if(isChartsetFullyAssigned() && !isChartsetAssignedToSystemKey(g_systemName))
        return true;
    
    else if(isChartsetExpired() && !isChartsetAssignedToSystemKey(g_systemName))
        return true;
    
    else
        return false;
}
    
bool itemChart::isChartsetShow()
{
    return true;
#if 0    
    if(isChartsetAssignedToMe(g_systemName))
        return true;
    
    if(!isChartsetFullyAssigned())
        return true;

    if(isChartsetAssignedToAnyDongle())
        return true;

    return false;
#endif    
}

bool itemChart::isChartsetAssignedToMe()
{
    //  Check if I am already assigned to this chart
    //  either by installed dongle, or systemName
    bool bAssigned = false;
    if(g_dongleName.Len()){
        if(isChartsetAssignedToSystemKey(g_dongleName))
            bAssigned = true;
    }
    else{
        if(isChartsetAssignedToSystemKey(g_systemName))
            bAssigned = true;
    }
    
    return bAssigned;
}

int itemChart::GetSlotAssignedToInstalledDongle( int &qId )
{
    if(!g_dongleName.Length())
        return (-1);
    
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(!strcmp(g_dongleName.mb_str(), slot->assignedSystemName.c_str())){
                qId = Qty.quantityId;
                return j;
            }
        }
    }
    
    return (-1);
}
            
int itemChart::GetSlotAssignedToSystem( int &qId )
{
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(!strcmp(g_systemName.mb_str(), slot->assignedSystemName.c_str())){
                qId = Qty.quantityId;
                return j;
            }
        }
    }
    
    return (-1);
}
            
int itemChart::FindQuantityIndex( int nqtyID){
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        if(Qty.quantityId == nqtyID)
            return i;
    }
    
    return -1;
}

itemSlot *itemChart::GetActiveSlot(){
    itemSlot *rv = NULL;
    if((m_activeQtyID < 0) || (m_assignedSlotIndex < 0))
        return rv;
    
    int qtyIndex = FindQuantityIndex( m_activeQtyID );
    return quantityList[qtyIndex].slotList[m_assignedSlotIndex];
}
                
//  Current status can be one of:
/*
 *      1.  Available for Installation.
 *      2.  Installed, Up-to-date.
 *      3.  Installed, Update available.
 *      4.  Expired.
 */        

int itemChart::getChartStatus()
{
    
    if(!g_chartListUpdatedOK){
        m_status = STAT_NEED_REFRESH;
        return m_status;
    }

    if(isChartsetExpired()){
        m_status = STAT_EXPIRED;
        return m_status;
    }

    
    // We determine which, if any, assignments are valid.
    
    // All slots consumed?
    int assignedCount = getChartAssignmentCount();
    int availCount = quantityList.size() * maxSlots;
    if(assignedCount >= availCount){            // fully assigned
                                                // so chartset must be assigned to dongle
                                                // or system to be processed further.
                                                // If not, then report "Fully assigned"
        bool bAvail = false;
        if(g_dongleName.Len()){
            if(isChartsetAssignedToAnyDongle())
                bAvail = true;
        }
        if(isChartsetAssignedToSystemKey( g_systemName ))
            bAvail = true;
        
        if(!bAvail){
            m_status = STAT_PURCHASED_NOSLOT;
            return m_status;
        }
    }
     
        

    // If a dongle is present, all operations will apply to dongle assigned slot, if any
    if(g_dongleName.Len()){
        
        // If chartset is not assigned to any dongle, result (for dongle) is clear
        //  If it is already assigned to a dongle, we pass thru and determine the slot later
        if(!isChartsetAssignedToAnyDongle()){
            m_status = STAT_PURCHASED;
            return m_status;
        }
    }
    else{               // No dongle present
        if(!isChartsetAssignedToSystemKey( g_systemName )){
            m_status = STAT_PURCHASED;
            return m_status;
        }
    }

    // We know that chart is assigned to me, so identify the slot
    m_assignedSlotIndex = -1;
    int tmpQtyID = -1;
    
    int slotIndex = GetSlotAssignedToInstalledDongle( tmpQtyID );
    if(slotIndex >= 0){
        m_assignedSlotIndex = slotIndex;
        m_activeQtyID = tmpQtyID;
    }
    else{
        slotIndex = GetSlotAssignedToSystem( tmpQtyID );
        if(slotIndex >= 0){
           m_assignedSlotIndex = slotIndex;
           m_activeQtyID = tmpQtyID;
        }
    }
    
    if(m_assignedSlotIndex < 0)
        return m_status;

    // From here, chart may be:
    // a.  Requestable
    // b.  Preparing
    // c.  Ready for download
    // d.  Downloading now.
    
    m_status = STAT_REQUESTABLE;

    
    //  Now check for installation state
    itemSlot *slot = GetActiveSlot();
    
    // Check for update
    if(slot->installedEdition.size()){
        if(GetServerEditionInt() > slot->GetInstalledEditionInt())
            m_status = STAT_STALE;
        else
            m_status = STAT_CURRENT;
    }
    
    return m_status;

#if 0    
    
    wxString cStat = statusID0;
    int slot = 0;
    if(isChartsetAssignedToAnyDongle()){
        if(isSlotAssignedToMyDongle( 1 )){
            cStat = statusID1;
            slot = 1;
        }
    }
    
    else{
        if(sysID1.IsSameAs(g_systemName)){
            cStat = statusID1;
            slot = 1;
        }
    }
        
    if(cStat.IsSameAs(_T("requestable"))){
        m_status = STAT_REQUESTABLE;
        return m_status;
    }

    if(cStat.IsSameAs(_T("processing"))){
        m_status = STAT_PREPARING;
        return m_status;
    }

    if(cStat.IsSameAs(_T("download"))){
        m_status = STAT_READY_DOWNLOAD;
        
        if(slot == 0){
            if(  (installLocation0.Length() > 0) && (installedFileDownloadPath0.Length() > 0) ){
                m_status = STAT_CURRENT;
                if(!installedEdition0.IsSameAs(currentChartEdition)){
                    m_status = STAT_STALE;
                }
            }
        }
        else if(slot == 1){
            if(  (installLocation1.Length() > 0) && (installedFileDownloadPath1.Length() > 0) ){
                m_status = STAT_CURRENT;
                if(!installedEdition1.IsSameAs(currentChartEdition)){
                    m_status = STAT_STALE;
                }
                
            }
        }
    }
#endif
    
    return m_status;
    
}
wxString itemChart::getStatusString()
{
    getChartStatus();
    
    wxString sret;
    
    switch(m_status){
        
        case STAT_UNKNOWN:
            break;
        
        case STAT_PURCHASED_NOSLOT:
            sret = _("Fully Assigned.");
            break;
            
        case STAT_PURCHASED:
            sret = _("Available.");
            break;
            
        case STAT_CURRENT:
            sret = _("Installed, Up-to-date.");
            break;
            
        case STAT_STALE:
            sret = _("Installed, Update available.");
            break;
            
        case STAT_EXPIRED:
        case STAT_EXPIRED_MINE:
            sret = _("Expired.");
            break;
            
        case STAT_PREPARING:
            sret = _("Preparing your chartset.");
            break;
            
        case STAT_READY_DOWNLOAD:
            sret = _("Ready for download.");
            break;
            
        case STAT_NEED_REFRESH:
            sret = _("Please update Chart List.");
            break;

        case STAT_REQUESTABLE:
            sret = _("Ready for Download Request.");
            break;
            
        default:
            break;
    }
    
    return sret;
    

}

wxString itemChart::getKeytypeString( std::string slotUUID ){
    
    // Find the slot
    for(unsigned int i=0 ; i < quantityList.size() ; i++){
        itemQuantity Qty = quantityList[i];
        for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
            itemSlot *slot = Qty.slotList[j];
            if(!strcmp(slotUUID.c_str(), slot->slotUuid.c_str())){
                wxString akey = wxString(slot->assignedSystemName.c_str());
                if(akey.StartsWith("sgl"))
                    return _("USB Key Dongle");
                else
                    return _("System Key");
            }
        }
    }
 
    return _T("");
}


int GetEditionInt(std::string edition)
{
    if(!edition.size())
        return 0;
    
    wxString sed(edition.c_str());
    wxString smaj = sed.BeforeFirst('-');
    wxString smin = sed.AfterFirst('-');

        
    long major = 0;
    smaj.ToLong(&major);
    long minor = 0;
    smin.ToLong(&minor);
    
    return (major * 100) + minor;
}

int itemChart::GetServerEditionInt()
{
    return GetEditionInt(serverChartEdition);
}


#if 0
// oitemChart
//------------------------------------------------------------------------------------------

oitemChart::oitemChart( wxString &order_ref, wxString &chartid, wxString &quantity) {
    //ident = id;
    orderRef = order_ref;
    chartID = chartid;
    quantityId = quantity;
    m_status = STAT_UNKNOWN;
}




// void itemChart::setDownloadPath(int slot, wxString path) {
//     if (slot == 0)
//         fileDownloadPath0 = path;
//     else if (slot == 1)
//         fileDownloadPath1 = path;
// }

// wxString itemChart::getDownloadPath(int slot) {
//     if (slot == 0)
//         return fileDownloadPath0;
//     else if (slot == 1)
//         return fileDownloadPath1;
//     else
//         return _T("");
// }

bool oitemChart::isChartsetAssignedToMe(wxString systemName){
    
    // is this chartset assigned to this system?
        if (sysID0.IsSameAs(systemName)) {
            return true;
        }
    
        if (sysID1.IsSameAs(systemName)) {
            return true;
        }
    
    return false;
    
}


bool oitemChart::isChartsetFullyAssigned() {
    
    if (statusID0.IsSameAs("unassigned") || !statusID0.Len())
        return false;
    
    if (statusID1.IsSameAs("unassigned") || !statusID1.Len())
        return false;
    
    return true;
}

bool oitemChart::isChartsetExpired() {
    
    bool bExp = false;
    if (statusID0.IsSameAs("expired") || statusID1.IsSameAs("expired")) {
        bExp = true;
    }
    return bExp;
}

bool oitemChart::isChartsetAssignedToAnyDongle() {
    
    if(isSlotAssignedToAnyDongle(0))
        return true;
    if(isSlotAssignedToAnyDongle(1))
        return true;
    return false;
}

bool oitemChart::isSlotAssignedToAnyDongle( int slot ) {
    long tl;
    if( slot == 0 ){
        if (sysID0.StartsWith("sgl")){
            if(sysID0.Mid(4).ToLong(&tl, 16))
                return true;
        }
    }
    else{
        if (sysID1.StartsWith("sgl")){
            if(sysID1.Mid(4).ToLong(&tl, 16))
                return true;
        }
    }
    return false;
}   
    
bool oitemChart::isSlotAssignedToMyDongle( int slot ) {
    long tl;
    if( slot == 0 ){
        if (sysID0.StartsWith("sgl")){
            if(sysID0.Mid(4).ToLong(&tl, 16)){
                if(tl == g_dongleSN)
                    return true;
            }
        }
    }
    else{
        if (sysID1.StartsWith("sgl")){
            if(sysID1.Mid(4).ToLong(&tl, 16)){
                if(tl == g_dongleSN)
                    return true;
            }
        }
    }
    return false;
}   
    

bool oitemChart::isChartsetDontShow()
{
    if(isChartsetFullyAssigned() && !isChartsetAssignedToMe(g_systemName))
        return true;
    
    else if(isChartsetExpired() && !isChartsetAssignedToMe(g_systemName))
        return true;
    
    else
        return false;
}
    
bool oitemChart::isChartsetShow()
{
    if(isChartsetAssignedToMe(g_systemName))
        return true;
    
    if(!isChartsetFullyAssigned())
        return true;

    if(isChartsetAssignedToAnyDongle())
        return true;

    return false;
}

    
//  Current status can be one of:
/*
 *      1.  Available for Installation.
 *      2.  Installed, Up-to-date.
 *      3.  Installed, Update available.
 *      4.  Expired.
 */        

int oitemChart::getChartStatus()
{
    if(!g_chartListUpdatedOK){
        m_status = STAT_NEED_REFRESH;
        return m_status;
    }

    if(isChartsetExpired()){
        m_status = STAT_EXPIRED;
        return m_status;
    }
    
    if(!isChartsetAssignedToMe( g_systemName )){
        if(!g_dongleName.Len()){
            if(!isChartsetAssignedToAnyDongle()){
                m_status = STAT_PURCHASED;
                return m_status;
            }
        }
        else{
            if(!isChartsetAssignedToAnyDongle()){
                m_status = STAT_PURCHASED;
                return m_status;
            }
        }
    }
    
    // We know that chart is assigned to me, so one of the sysIDx fields will match
    wxString cStat = statusID0;
    int slot = 0;
    if(isChartsetAssignedToAnyDongle()){
        if(isSlotAssignedToMyDongle( 1 )){
            cStat = statusID1;
            slot = 1;
        }
    }
    
    else{
        if(sysID1.IsSameAs(g_systemName)){
            cStat = statusID1;
            slot = 1;
        }
    }
        
    if(cStat.IsSameAs(_T("requestable"))){
        m_status = STAT_REQUESTABLE;
        return m_status;
    }

    if(cStat.IsSameAs(_T("processing"))){
        m_status = STAT_PREPARING;
        return m_status;
    }

    if(cStat.IsSameAs(_T("download"))){
        m_status = STAT_READY_DOWNLOAD;
        
        if(slot == 0){
            if(  (installLocation0.Length() > 0) && (installedFileDownloadPath0.Length() > 0) ){
                m_status = STAT_CURRENT;
                if(!installedEdition0.IsSameAs(currentChartEdition)){
                    m_status = STAT_STALE;
                }
            }
        }
        else if(slot == 1){
            if(  (installLocation1.Length() > 0) && (installedFileDownloadPath1.Length() > 0) ){
                m_status = STAT_CURRENT;
                if(!installedEdition1.IsSameAs(currentChartEdition)){
                    m_status = STAT_STALE;
                }
                
            }
        }
    }

     
    return m_status;
    
}
wxString oitemChart::getStatusString()
{
    getChartStatus();
    
    wxString sret;
    
    switch(m_status){
        
        case STAT_UNKNOWN:
            break;
            
        case STAT_PURCHASED:
            sret = _("Available.");
            break;
            
        case STAT_CURRENT:
            sret = _("Installed, Up-to-date.");
            break;
            
        case STAT_STALE:
            sret = _("Installed, Update available.");
            break;
            
        case STAT_EXPIRED:
        case STAT_EXPIRED_MINE:
            sret = _("Expired.");
            break;
            
        case STAT_PREPARING:
            sret = _("Preparing your chartset.");
            break;
            
        case STAT_READY_DOWNLOAD:
            sret = _("Ready for download.");
            break;
            
        case STAT_NEED_REFRESH:
            sret = _("Please update Chart List.");
            break;

        case STAT_REQUESTABLE:
            sret = _("Ready for Download Request.");
            break;
            
        default:
            break;
    }
    
    return sret;
    

}

wxString oitemChart::getKeytypeString(){
 
    if(isChartsetAssignedToAnyDongle())
        return _("USB Dongle Key");
    else if(isChartsetAssignedToMe(g_systemName))
        return _("System Key");
    else
        return _T("");
}

wxBitmap& oitemChart::GetChartThumbnail(int size)
{
    if(!m_ChartImage.IsOk()){
        // Look for cached copy
        wxString fileKey = _T("ChartImage-");
        fileKey += chartID;
        fileKey += _T(".jpg");
 
        wxString file = g_PrivateDataDir + fileKey;
        if(::wxFileExists(file)){
            m_ChartImage = wxImage( file, wxBITMAP_TYPE_ANY);
        }
        else{
            if(g_chartListUpdatedOK && thumbnailURL.Length()){  // Do not access network until after first "getList"
                wxCurlHTTP get;
                get.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
                bool getResult = get.Get(file, thumbnailURL);

            // get the response code of the server
                int iResponseCode;
                get.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
            
                if(iResponseCode == 200){
                    if(::wxFileExists(file)){
                        m_ChartImage = wxImage( file, wxBITMAP_TYPE_ANY);
                    }
                }
            }
        }
    }
    
    if(m_ChartImage.IsOk()){
        int scaledHeight = size;
        int scaledWidth = m_ChartImage.GetWidth() * scaledHeight / m_ChartImage.GetHeight();
        wxImage scaledImage = m_ChartImage.Rescale(scaledWidth, scaledHeight);
        m_bm = wxBitmap(scaledImage);
        
        return m_bm;
    }
    else{
        wxImage img(size, size);
        unsigned char *data = img.GetData();
        for(int i=0 ; i < size * size * 3 ; i++)
            data[i] = 200;
        
        m_bm = wxBitmap(img);   // Grey bitmap
        return m_bm;
    }
    
}

#endif


//Utility Functions
//  Search g_ChartArray for chart having specified parameters

#if 0

int findOrderRefChartId(wxString &orderRef, wxString &chartId, wxString &quantity)
{
    for(unsigned int i = 0 ; i < g_ChartArray.GetCount() ; i++){
        if(g_ChartArray.Item(i)->orderRef.IsSameAs(orderRef)
            && g_ChartArray.Item(i)->chartID.IsSameAs(chartId) &&
            g_ChartArray.Item(i)->quantityId.IsSameAs(quantity) ){
                return (i);
            }
    }
    return -1;
}
#endif

int findOrderRefChartId( std::string orderRef, std::string chartId)
{
    for(unsigned int i = 0 ; i < ChartVector.size() ; i++){
        if(
            (!strcmp(ChartVector[i]->orderRef.c_str(), orderRef.c_str()))
            && (!strcmp(ChartVector[i]->chartID.c_str(), chartId.c_str()))){
            return (i);
        }
    }
    return -1;
}


void loadShopConfig()
{
    //    Get a pointer to the opencpn configuration object
    wxFileConfig *pConf = GetOCPNConfigObject();
    
    if( pConf ) {
        pConf->SetPath( _T("/PlugIns/ocharts") );
        
        pConf->Read( _T("systemName"), &g_systemName);
        pConf->Read( _T("loginUser"), &g_loginUser);
        pConf->Read( _T("loginKey"), &g_loginKey);
        pConf->Read( _T("lastInstllDir"), &g_lastInstallDir);
        
        pConf->Read( _T("ADMIN"), &g_admin);
        pConf->Read( _T("DEBUG_SHOP"), &g_debugShop);
        pConf->Read( _T("LastEULAFile"), &g_lastEULAFile);

//        g_systemName = _T("");
//        g_loginKey = _T("");
        
#if 1        
        // Get the list of charts
        wxArrayString chartIDArray;
        
        pConf->SetPath ( _T ( "/PlugIns/ocharts/oeus/charts" ) );
        wxString strk;
        wxString kval;
        long dummyval;
        bool bContk = pConf->GetFirstEntry( strk, dummyval );
        while( bContk ) {
            pConf->Read( strk, &kval );
            chartIDArray.Add(kval);
            bContk = pConf->GetNextEntry( strk, dummyval );

        }
            
        for(unsigned int i=0 ; i < chartIDArray.GetCount() ; i++){
            wxString chartConfigIdent = chartIDArray[i];
            pConf->SetPath ( _T ( "/PlugIns/ocharts/oeus/charts/" ) + chartConfigIdent );
            
            wxString orderRef, chartName, overrideChartEdition, chartID;
            pConf->Read( _T("chartID"), &chartID);
            pConf->Read( _T("orderRef"), &orderRef);
            pConf->Read( _T("chartName"), &chartName);
            pConf->Read( _T("overrideChartEdition"), &overrideChartEdition);


            // Add a chart if necessary
            int index = findOrderRefChartId((const char *)orderRef.mb_str(), (const char *)chartID.mb_str());
            itemChart *chart;
            if(index < 0){
                chart = new itemChart;
                chart->orderRef = orderRef.mb_str();
                chart->chartID = chartID.mb_str();
                chart->chartName = chartName.mb_str();
                chart->chartType = CHART_TYPE_OEUSENC;
                //chart->installedChartEdition = installedChartEdition.mb_str();
                chart->overrideChartEdition = overrideChartEdition.mb_str();
                ChartVector.push_back(chart);
            }
            else
                chart = ChartVector[index];
            
            // Process Slots
            
            pConf->SetPath ( _T ( "/PlugIns/ocharts/oeus/charts/" ) + chartConfigIdent + _T("/Slots" ));

            bContk = pConf->GetFirstEntry( strk, dummyval );
            while( bContk ) {
                pConf->Read( strk, &kval );
                if(strk.StartsWith("Slot")){
                    wxStringTokenizer tkz( kval, _T(";") );
                    while(tkz.HasMoreTokens()){
                        wxString sqid = tkz.GetNextToken(); 
                        wxString slotUUID = tkz.GetNextToken();
                        wxString assignedSystemName = tkz.GetNextToken();
                        wxString installLocation = tkz.GetNextToken();
                        wxString installedEdition = tkz.GetNextToken();
                        
                        // Make a new "quantity" clause if necessary
                        long nqid = -1;
                        
                        if(sqid.ToLong(&nqid)){
                            int qtyIndex = chart->FindQuantityIndex(nqid);
                            if(qtyIndex < 0){
                                itemQuantity new_qty;
                                new_qty.quantityId = nqid;
                                itemSlot *slot = new itemSlot;
                                slot->installLocation = std::string(installLocation.mb_str());
                                slot->assignedSystemName = std::string(assignedSystemName.mb_str());
                                slot->slotUuid = std::string(slotUUID.mb_str());
                                slot->installedEdition = std::string(installedEdition.mb_str());
                                
                                new_qty.slotList.push_back(slot);

                                chart->quantityList.push_back(new_qty);
                            }
                            else{
                                itemQuantity *exist_qty = &(chart->quantityList[qtyIndex]);
                                itemSlot *slot = chart->GetSlotPtr( slotUUID );
                                if(!slot)
                                    slot = new itemSlot;
                                
                                slot->installLocation = std::string(installLocation.mb_str());
                                slot->assignedSystemName = std::string(assignedSystemName.mb_str());
                                slot->slotUuid = std::string(slotUUID.mb_str());
                                slot->installedEdition = std::string(installedEdition.mb_str());

                                exist_qty->slotList.push_back(slot);
                            }
                        }
                    }

                }
                bContk = pConf->GetNextEntry( strk, dummyval );

            }
        }
#endif            

#if 1        
        // Get the list of charts
        chartIDArray.Clear();
        
        pConf->SetPath ( _T ( "/PlugIns/ocharts/oeur/charts" ) );
        bContk = pConf->GetFirstEntry( strk, dummyval );
        while( bContk ) {
            pConf->Read( strk, &kval );
            chartIDArray.Add(kval);
            bContk = pConf->GetNextEntry( strk, dummyval );

        }
            
        for(unsigned int i=0 ; i < chartIDArray.GetCount() ; i++){
            wxString chartConfigIdent = chartIDArray[i];
            pConf->SetPath ( _T ( "/PlugIns/ocharts/oeur/charts/" ) + chartConfigIdent );
           
            wxString orderRef, chartName, overrideChartEdition, chartID;
            pConf->Read( _T("chartID"), &chartID);
            pConf->Read( _T("orderRef"), &orderRef);
            pConf->Read( _T("chartName"), &chartName);
            pConf->Read( _T("overrideChartEdition"), &overrideChartEdition);


            // Add a chart if necessary
            int index = findOrderRefChartId((const char *)orderRef.mb_str(), (const char *)chartID.mb_str());
            itemChart *chart;
            if(index < 0){
                chart = new itemChart;
                chart->orderRef = orderRef.mb_str();
                chart->chartID = chartID.mb_str();
                chart->chartName = chartName.mb_str();
                chart->chartType = CHART_TYPE_OERNC;

                //chart->installedChartEdition = installedChartEdition.mb_str();
                chart->overrideChartEdition = overrideChartEdition.mb_str();
                ChartVector.push_back(chart);
            }
            else
                chart = ChartVector[index];
            
            // Process Slots
            
            pConf->SetPath ( _T ( "/PlugIns/ocharts/oeur/charts/" ) + chartConfigIdent + _T("/Slots" ));

            bContk = pConf->GetFirstEntry( strk, dummyval );
            while( bContk ) {
                pConf->Read( strk, &kval );
                if(strk.StartsWith("Slot")){
                    wxStringTokenizer tkz( kval, _T(";") );
                    while(tkz.HasMoreTokens()){
                        wxString sqid = tkz.GetNextToken(); 
                        wxString slotUUID = tkz.GetNextToken();
                        wxString assignedSystemName = tkz.GetNextToken();
                        wxString installLocation = tkz.GetNextToken();
                        wxString installedEdition = tkz.GetNextToken();
                        
                        // Make a new "quantity" clause if necessary
                        long nqid = -1;
                        
                        if(sqid.ToLong(&nqid)){
                            int qtyIndex = chart->FindQuantityIndex(nqid);
                            if(qtyIndex < 0){
                                itemQuantity new_qty;
                                new_qty.quantityId = nqid;
                                itemSlot *slot = new itemSlot;
                                slot->installLocation = std::string(installLocation.mb_str());
                                slot->assignedSystemName = std::string(assignedSystemName.mb_str());
                                slot->slotUuid = std::string(slotUUID.mb_str());
                                slot->installedEdition = std::string(installedEdition.mb_str());
                                
                                new_qty.slotList.push_back(slot);

                                chart->quantityList.push_back(new_qty);
                            }
                            else{
                                itemQuantity *exist_qty = &(chart->quantityList[qtyIndex]);
                                itemSlot *slot = chart->GetSlotPtr( slotUUID );
                                if(!slot)
                                    slot = new itemSlot;
                                
                                slot->installLocation = std::string(installLocation.mb_str());
                                slot->assignedSystemName = std::string(assignedSystemName.mb_str());
                                slot->slotUuid = std::string(slotUUID.mb_str());
                                slot->installedEdition = std::string(installedEdition.mb_str());

                                exist_qty->slotList.push_back(slot);
                            }
                        }
                    }

                }
                bContk = pConf->GetNextEntry( strk, dummyval );

            }
        }
#endif            

    }
}

void saveShopConfig()
{
    wxFileConfig *pConf = GetOCPNConfigObject();
        
   if( pConf ) {
      pConf->SetPath( _T("/PlugIns/ocharts") );
            
      pConf->Write( _T("systemName"), g_systemName);
      pConf->Write( _T("loginUser"), g_loginUser);
      pConf->Write( _T("loginKey"), g_loginKey);
      pConf->Write( _T("lastInstllDir"), g_lastInstallDir);
      pConf->Write( _T("LastEULAFile"), g_lastEULAFile);

#if 1      
      pConf->DeleteGroup( _T("/PlugIns/ocharts/oeus/charts") );
      pConf->SetPath( _T("/PlugIns/ocharts/oeus/charts") );

     for(unsigned int i = 0 ; i < ChartVector.size() ; i++){
          itemChart *chart = ChartVector[i];
          if(chart->chartType != CHART_TYPE_OEUSENC)
              continue;
          wxString keyChart;
          keyChart.Printf(_T("Chart%d"), i);
          pConf->Write(keyChart, wxString(chart->chartID + "-" + chart->orderRef));
     }
      
     for(unsigned int i = 0 ; i < ChartVector.size() ; i++){
          itemChart *chart = ChartVector[i];
          if(chart->chartType != CHART_TYPE_OEUSENC)
              continue;
          wxString chartConfigIdent = _T("/PlugIns/ocharts/oeus/charts/") + wxString(chart->chartID + "-" + chart->orderRef);
          pConf->DeleteGroup( chartConfigIdent );
          pConf->SetPath( chartConfigIdent );
           
          pConf->Write( _T("chartID"), wxString(chart->chartID) );
          pConf->Write( _T("chartName"), wxString(chart->chartName) );
          pConf->Write( _T("orderRef"), wxString(chart->orderRef) );
          //pConf->Write( _T("installedChartEdition"), wxString(chart->installedChartEdition) );
          if(chart->overrideChartEdition.size())
            pConf->Write( _T("overrideChartEdition"), wxString(chart->overrideChartEdition) );

          wxString slotsPath = chartConfigIdent + _T("/Slots");
          pConf->DeleteGroup( slotsPath );
          pConf->SetPath( slotsPath );
         
          for( unsigned int j = 0 ; j < chart->quantityList.size() ; j++){
              int qID = chart->quantityList[j].quantityId;
              if(qID < 0)
                  continue;
              wxString sqid;
              sqid.Printf(_T("%d"), qID);
              
              wxString key = _T("Slot");
              key += sqid;
              itemSlot *slot;
              wxString val;

              for( unsigned int k = 0 ; k < chart->quantityList[j].slotList.size() ; k++){
                  slot = chart->quantityList[j].slotList[k];
                  if( slot->assignedSystemName.size()){
                    val += sqid + _T(";");
                    val += slot->slotUuid + _T(";");
                    val += slot->assignedSystemName + _T(";");
                    val += slot->installLocation + _T(";");
                    val += slot->installedEdition + _T(";");
                  }
              }
              if(val.Length())
                  pConf->Write( key, val );
          }
      }
#endif
      
#if 1      
      pConf->DeleteGroup( _T("/PlugIns/ocharts/oeur/charts") );
      pConf->SetPath( _T("/PlugIns/ocharts/oeur/charts") );

     for(unsigned int i = 0 ; i < ChartVector.size() ; i++){
          itemChart *chart = ChartVector[i];
          if(chart->chartType != CHART_TYPE_OERNC)
              continue;
          wxString keyChart;
          keyChart.Printf(_T("Chart%d"), i);
          pConf->Write(keyChart, wxString(chart->chartID + "-" + chart->orderRef));
     }
      
     for(unsigned int i = 0 ; i < ChartVector.size() ; i++){
          itemChart *chart = ChartVector[i];
          if(chart->chartType != CHART_TYPE_OERNC)
              continue;
          wxString chartConfigIdent = _T("/PlugIns/ocharts/oeur/charts/") + wxString(chart->chartID + "-" + chart->orderRef);
          pConf->DeleteGroup( chartConfigIdent );
          pConf->SetPath( chartConfigIdent );
           
          pConf->Write( _T("chartID"), wxString(chart->chartID) );
          pConf->Write( _T("chartName"), wxString(chart->chartName) );
          pConf->Write( _T("orderRef"), wxString(chart->orderRef) );
          //pConf->Write( _T("installedChartEdition"), wxString(chart->installedChartEdition) );
          if(chart->overrideChartEdition.size())
            pConf->Write( _T("overrideChartEdition"), wxString(chart->overrideChartEdition) );

          wxString slotsPath = chartConfigIdent + _T("/Slots");
          pConf->DeleteGroup( slotsPath );
          pConf->SetPath( slotsPath );
         
          for( unsigned int j = 0 ; j < chart->quantityList.size() ; j++){
              int qID = chart->quantityList[j].quantityId;
              if(qID < 0)
                  continue;
              wxString sqid;
              sqid.Printf(_T("%d"), qID);
              
              wxString key = _T("Slot");
              key += sqid;
              itemSlot *slot;
              wxString val;

              for( unsigned int k = 0 ; k < chart->quantityList[j].slotList.size() ; k++){
                  slot = chart->quantityList[j].slotList[k];
                  if( slot->assignedSystemName.size()){
                    val += sqid + _T(";");
                    val += slot->slotUuid + _T(";");
                    val += slot->assignedSystemName + _T(";");
                    val += slot->installLocation + _T(";");
                    val += slot->installedEdition + _T(";");
                  }
              }
              if(val.Length())
                  pConf->Write( key, val );
          }
      }
#endif
   }
}

int checkResult(wxString &result, bool bShowLoginErrorDialog = true)
{
    if(g_shopPanel){
        g_ipGauge->Stop();
    }
    
    wxString resultDigits = result.BeforeFirst(':');
    
    bool bSkipErrorDialog = false;
    long dresult;
    if(resultDigits.ToLong(&dresult)){
        if(dresult == 1){
            return 0;
        }
        else{
           wxString msg = _("o-charts API error code: ");
           wxString msg1;
           msg1.Printf(_T("{%ld}\n\n"), dresult);
           msg += msg1;
           if( bShowLoginErrorDialog ){
                switch(dresult){
                    case 4:
                        msg += _("User does not exist.");
                        break;
                    case 5:
                    case 6:
                        msg += _("Invalid user/email name or password.");
                        break;
                    default:
                        if(result.AfterFirst(':').Length()){
                            msg += result.AfterFirst(':');
                            msg += _T("\n");
                        }
                        msg += _("Operation cancelled");
                        break;
  
                }
                OCPNMessageBox_PlugIn(NULL, msg, _("o-charts_pi Message"), wxOK);

           }
                
           else{
                switch(dresult){
                    case 4:
                    case 5:
                    case 6:
                        bSkipErrorDialog = true;
                        break;
                    case 27:
                        msg += _("This oeRNC plugin version is obsolete.");
                        msg += _T("\n");
                        msg += _("Please update your plugin.");
                        msg += _T("\n");
                        msg +=  _("Operation cancelled");
                        break;
                    default:
                        if(result.AfterFirst(':').Length()){
                            msg += result.AfterFirst(':');
                            msg += _T("\n");
                        }
                        msg += _("Operation cancelled");
                        break;
                }
                
                if(!bSkipErrorDialog)
                    OCPNMessageBox_PlugIn(NULL, msg, _("o-charts_pi Message"), wxOK);
            }
            return dresult;
        }
    }
    else{
        // Check the "number + letter" cases...
        wxString msg;
        if(result.IsSameAs("3d"))
            msg = _("void username");
        else if(result.IsSameAs("3e"))
            msg = _("invalid username");
        else if(result.IsSameAs("3f"))
            msg = _("void password");
        else if(result.IsSameAs("3g"))
            msg = _("wrong password");
        else if(result.IsSameAs("8l"))
            msg = _("There is not a system name for this device yet.");
        else if(result.IsSameAs("8h"))
            msg = _("Something has changed in the device assigned to this system name.");
        
        OCPNMessageBox_PlugIn(NULL, _("o-Charts shop interface error") + _T("\n") + result + _T("\n") + msg, _("o-charts_pi Message"), wxOK);
    }
     
    return 98;
}

int checkResponseCode(int iResponseCode)
{
    if(iResponseCode != 200){
        wxString msg = _("internet communications error code: ");
        wxString msg1;
        msg1.Printf(_T("{%d}\n "), iResponseCode);
        msg += msg1;
        msg += _("Check your connection and try again.");
        ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxOK);
    }
    
    // The wxCURL library returns "0" as response code,
    // even when it should probably return 404.
    // We will use "99" as a special code here.
    
    if(iResponseCode < 100)
        return 99;
    else
        return iResponseCode;
        
}

int doLogin( wxWindow *parent )
{
    oeUniLogin login(parent);
    login.ShowModal();
    if(!(login.GetReturnCode() == 0)){
        //g_shopPanel->setStatusText( _("Invalid Login."));
        wxYield();
        return 55;
    }
    
    g_loginUser = login.m_UserNameCtl->GetValue().Trim( true).Trim( false );
    wxString pass = login.m_PasswordCtl->GetValue().Trim( true).Trim( false );

    wxString url = userURL;
    if(g_admin)
        url = adminURL;
    
    url +=_T("?fc=module&module=occharts&controller=apioesu");

    wxString loginParms;
    loginParms += _T("taskId=login");
    loginParms += _T("&username=") + g_loginUser;
    loginParms += _T("&password=") + pass;
    if(g_debugShop.Len())
        loginParms += _T("&debug=") + g_debugShop;
    loginParms += _T("&version=") + g_systemOS + g_versionString;
    
    int iResponseCode =0;
    TiXmlDocument *doc = 0;
    size_t res = 0;
#ifdef __OCPN_USE_CURL__    
    wxCurlHTTPNoZIP post;
    post.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
    res = post.Post( loginParms.ToAscii(), loginParms.Len(), url );
    
    // get the response code of the server
    post.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
    if(iResponseCode == 200){
        doc = new TiXmlDocument();
        doc->Parse( post.GetResponseBody().c_str());
    }

#else

    qDebug() << url.mb_str();
    qDebug() << loginParms.mb_str();
    
    wxString postresult;
    _OCPN_DLStatus stat = OCPN_postDataHttp( url, loginParms, postresult, 5 );

    qDebug() << "doLogin Post Stat: " << stat;
    
    if(stat != OCPN_DL_FAILED){
        wxCharBuffer buf = postresult.ToUTF8();
        std::string response(buf.data());
        
        qDebug() << response.c_str();
        doc = new TiXmlDocument();
        doc->Parse( response.c_str());
        iResponseCode = 200;
        res = 1;
    }

#endif
    
    if(iResponseCode == 200){
//        const char *rr = doc->Parse( post.GetResponseBody().c_str());
//         wxString p = wxString(post.GetResponseBody().c_str(), wxConvUTF8);
//         wxLogMessage(_T("doLogin results:"));
//         wxLogMessage(p);
        
        wxString queryResult;
        wxString loginKey;
        
        if( res )
        {
            TiXmlElement * root = doc->RootElement();
            if(!root){
                wxString r = _T("50");
                checkResult(r);                              // undetermined error??
                return false;
            }
            
            wxString rootName = wxString::FromUTF8( root->Value() );
            TiXmlNode *child;
            for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){
                wxString s = wxString::FromUTF8(child->Value());
                
                if(!strcmp(child->Value(), "result")){
                    TiXmlNode *childResult = child->FirstChild();
                    queryResult =  wxString::FromUTF8(childResult->Value());
                }
                else if(!strcmp(child->Value(), "key")){
                    TiXmlNode *childResult = child->FirstChild();
                    loginKey =  wxString::FromUTF8(childResult->Value());
                }
            }
        }
        
        if(queryResult == _T("1")){
            g_loginKey = loginKey;
        }
        else{
            checkResult(queryResult, true);
        }
        
        long dresult;
        if(queryResult.ToLong(&dresult)){
            return dresult;
        }
        else{
            return 53;
        }
    }
    else
        return checkResponseCode(iResponseCode);
    
}

itemChart *FindChartForSlotUUID(wxString UUID)
{
    itemChart *rv = NULL;
    for(unsigned int i=0 ; i < ChartVector.size() ; i++){
        itemChart *chart = ChartVector[i];
        if(chart->isUUIDAssigned(UUID))
            return chart;
    }
 
    return rv;
}

wxString ProcessResponse(std::string body, bool bsubAmpersand)
{
//    std::string db("<?xml version=\"1.0\" encoding=\"utf-8\"?><response><result>1</result><chart> <quantity> <quantityId>1</quantityId> <slot> <slotUuid>123e4567-e89b-12d3-a456-426655440000</slotUuid> <assignedSystemName>mylaptop</assignedSystemName> <lastRequested>2-1</lastRequested>     </slot> <slot> </slot> </quantity> </chart></response>");
//    body = db;
        TiXmlDocument * doc = new TiXmlDocument();
        if(bsubAmpersand){
            std::string oldStr("&");
            std::string newStr("&amp;");
            std::string::size_type pos = 0u;
            while((pos = body.find(oldStr, pos)) != std::string::npos){
                body.replace(pos, oldStr.length(), newStr);
                pos += newStr.length();
            }
        }

        const char *rr = doc->Parse( body.c_str());
    
        doc->Print();
        
        itemChart *pChart = NULL;

        wxString queryResult = _T("50");  // Default value, general error.
        wxString chartOrder;
        wxString chartPurchase;
        wxString chartExpiration;
        wxString chartID;
        wxString edition;
        wxString editionDate;
        wxString maxSlots;
        wxString expired;
        wxString chartName;
        wxString chartQuantityID;
        wxString chartSlot;
        wxString chartAssignedSystemName;
        wxString chartLastRequested;
        wxString chartState;
        wxString chartLink;
        wxString chartSize;
        wxString chartThumbURL;
        itemSlot *activeSlot = NULL;

         wxString p = wxString(body.c_str(), wxConvUTF8);
         //  wxMSW does not like trying to format this string containing "%" characters
#ifdef __WXGTK__         
         wxLogMessage(_T("ProcessResponse results:"));
         wxLogMessage(p);
#endif
        
            TiXmlElement * root = doc->RootElement();
            if(!root){
                return _T("57");                              // undetermined error??
            }
            
            wxString rootName = wxString::FromUTF8( root->Value() );
            TiXmlNode *child;
            for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){
                pChart = NULL;
                wxString s = wxString::FromUTF8(child->Value());
                
                if(!strcmp(child->Value(), "result")){
                    TiXmlNode *childResult = child->FirstChild();
                    queryResult =  wxString::FromUTF8(childResult->Value());
                }
                
                else if(!strcmp(child->Value(), "systemName")){
                    TiXmlNode *childsystemName = child->FirstChild();
                    wxString sName =  wxString::FromUTF8(childsystemName->Value());
                    if(g_systemNameChoiceArray.Index(sName) == wxNOT_FOUND)
                        g_systemNameChoiceArray.Add(sName);
                    
                    //  Maintain a separate list of systemNames known to the server
                    if(g_systemNameServerArray.Index(sName) == wxNOT_FOUND)
                        g_systemNameServerArray.Add(sName);

                }
                
                else if(!strcmp(child->Value(), "disabledSystemName")){
                    TiXmlNode *childsystemNameDisabled = child->FirstChild();
                    wxString sName =  wxString::FromUTF8(childsystemNameDisabled->Value());
                    if(g_systemNameDisabledArray.Index(sName) == wxNOT_FOUND)
                        g_systemNameDisabledArray.Add(sName);
                    
                }
                else if(!strcmp(child->Value(), "slotUuid")){
                    TiXmlNode *childslotuuid = child->FirstChild();
                    g_lastSlotUUID =  wxString::FromUTF8(childslotuuid->Value());
 
                    // This is a response from ChartRequest
                    // Find the slot, and clear out the array of itemTaskFileInfo
                    itemChart *pChart = FindChartForSlotUUID(g_lastSlotUUID);
                    if(pChart){
                        itemSlot *pslot = pChart->GetSlotPtr(g_lastSlotUUID);
                        if(pslot){
                            pslot->taskFileList.clear();
                        }
                    }
                }
                
                else if(!strcmp(child->Value(), "file")){
                    // Get a pointer to slot referenced by last UUID seen
                    itemChart *pChart = FindChartForSlotUUID(g_lastSlotUUID);
                    if(pChart){
                        itemSlot *pslot = pChart->GetSlotPtr(g_lastSlotUUID);
                        activeSlot = pslot;
                        if(activeSlot){
                            
                            // Create a new set of task files definitions
                            itemTaskFileInfo *ptfi = new itemTaskFileInfo;
                            activeSlot->taskFileList.push_back(ptfi);
                        
                            TiXmlNode *childFile = child->FirstChild();
                            for ( childFile = child->FirstChild(); childFile!= 0; childFile = childFile->NextSibling()){
                                const char *fileVal =  childFile->Value();
                            
                                if(!strcmp(fileVal, "link")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->link = childVal->Value();
                                }
                                
                                else if(!strcmp(fileVal, "size")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->size = childVal->Value();
                                }

                                else if(!strcmp(fileVal, "sha256")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->sha256 = childVal->Value();
                                }

                                else if(!strcmp(fileVal, "chartKeysLink")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->linkKeys = childVal->Value();
                                }
                                
                                else if(!strcmp(fileVal, "chartKeysSha256")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->sha256Keys = childVal->Value();
                                }

                                else if(!strcmp(fileVal, "editionTarget")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->target = childVal->Value();
                                }

                                else if(!strcmp(fileVal, "editionResult")){
                                    TiXmlNode *childVal = childFile->FirstChild();
                                    if(childVal) 
                                        ptfi->result = childVal->Value();
                                }
                            }
                        }
                    }
                }
                    
                else if(!strcmp(child->Value(), "chart")){
                                    
                    pChart = new itemChart();
                    
                    
                    TiXmlNode *childChart = child->FirstChild();
                    for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                        const char *chartVal =  childChart->Value();
  
                        /*
                                b.0. order: order reference.
                                b.1. purchase: purchase date YYYY-MM-DD.
                                b.2. expiration: expiration date YYYY-MM-DD.
                                b.3. expired: 0 > not expired, 1 > expired.
                                b.4. thumbLink: link to thumbimage in shop.
                                b.5. edition: current chart edition on shop. Format: year/edition-update (2020/1-2).
                                b.6. editionDate: publication date of current edition YYYY-MM-DD.
                                b.7. chartName: Name of chartset in user language.
                                b.8. maxSlots: maximum slots allowed for this chart.
                                b.9. baseChartList (list): list of links to all base ChartList.xml files of the current edition.
                                b.10. updateChartList (list): list of links to all update ChartList.xml files of the current edition. This field is not sent if the current edition has not updates.
                                b.11. chartid: unique chart id on shop.
                                
                                b.12. quantity (list): list of charts for the same country in the same order.
                                        b.12.0. quantityId: integer
                                        b.12.1. slot (list): list of allowed slots.
                                                b.12.1.0. slotUuid: unique ID. Format: UUID AAAAAAAA-AAAA-AAAA-AAAA-AAAAAAAAAAAA.
                                                b.12.1.1. assignedSystemName: assigned system name for that slot.
                                b.13. chartType: oeSENC/oeRNC

                        */
                        if(!strcmp(chartVal, "order")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->orderRef = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "purchase")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->purchaseDate = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "expiration")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->expDate = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "chartId")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->chartID = childVal->Value();
                        }                        
                        else if(!strcmp(chartVal, "thumbLink")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->thumbLink = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "edition")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->serverChartEdition = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "editionDate")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->editionDate = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "chartName")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal) pChart->chartName = childVal->Value();
                        }
                        else if(!strcmp(chartVal, "expired")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal){
                                if(!strcmp(childVal->Value(), "1"))
                                    pChart->bExpired = true;
                                else
                                    pChart->bExpired =false;
                            }
                        }
                        else if(!strcmp(chartVal, "chartType")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal){
                                if(!strcmp(childVal->Value(), "oeuSENC"))
                                    pChart->chartType = (int)CHART_TYPE_OEUSENC;
                                else
                                    pChart->chartType = (int)CHART_TYPE_OERNC;
                            }
                        }
                        else if(!strcmp(chartVal, "maxSlots")){
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal){
                                wxString mxSlots = wxString::FromUTF8(childVal->Value());
                                long cslots = -1;
                                mxSlots.ToLong(&cslots);
                                pChart->maxSlots = cslots;
                            }
                        }

                        else if(!strcmp(chartVal, "baseChartList")){
                            wxString URL;
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal){
                                URL = wxString::FromUTF8(childVal->Value());
                                pChart->baseChartListArray.Add( URL );
                            }
                        }
                        else if(!strcmp(chartVal, "updateChartList")){
                            wxString URL;
                            TiXmlNode *childVal = childChart->FirstChild();
                            if(childVal){
                                URL = wxString::FromUTF8(childVal->Value());
                                pChart->updateChartListArray.Add( URL );
                            }
                        }

                        else if(!strcmp(chartVal, "quantity")){
                            itemQuantity Qty;
                            TiXmlNode *childQuantityChild;
                            for ( childQuantityChild = childChart->FirstChild(); childQuantityChild!= 0; childQuantityChild = childQuantityChild->NextSibling()){
                                const char *quantityVal =  childQuantityChild->Value();

                                if(!strcmp(quantityVal, "quantityId")){
                                    TiXmlNode *quantityNode = childChart->FirstChild();
                                    if(quantityNode){
                                        wxString qid = wxString::FromUTF8(quantityNode->FirstChild()->Value());
                                        long sv;
                                        qid.ToLong(&sv);
                                        Qty.quantityId = sv;
                                    }
                                }
                                else if(!strcmp(quantityVal, "slot")){
                                     itemSlot *Slot = new itemSlot;
                                     TiXmlNode *childSlotChild;
                                     for ( childSlotChild = childQuantityChild->FirstChild(); childSlotChild!= 0; childSlotChild = childSlotChild->NextSibling()){
                                         const char *slotVal =  childSlotChild->Value();
 
                                         if(!strcmp(slotVal, "slotUuid")){
                                             TiXmlNode *childVal = childSlotChild->FirstChild();
                                             if(childVal) Slot->slotUuid = childVal->Value();
                                         }
                                         else if(!strcmp(slotVal, "assignedSystemName")){
                                             TiXmlNode *childVal = childSlotChild->FirstChild();
                                             if(childVal) Slot->assignedSystemName = childVal->Value();
                                         }
                                         else if(!strcmp(slotVal, "lastRequested")){
                                             TiXmlNode *childVal = childSlotChild->FirstChild();
                                             if(childVal) Slot->lastRequested = childVal->Value();
                                         }
                                     }
                                     Qty.slotList.push_back(Slot);
                                 }
                            }
                            pChart->quantityList.push_back(Qty);
                        }
                    }
                    
                    if( pChart == NULL )
                        continue;
                        
                    // Process this chart node
                    
                    // As identified uniquely by order, and chartid....
                    // Does this chart already exist in the table?
                    int index = findOrderRefChartId(pChart->orderRef, pChart->chartID);
                    if(index < 0){
                        pChart->bshopValidated = true;
                        ChartVector.push_back(pChart);
                    }
                    else{
                        ChartVector[index]->Update(pChart);
                        ChartVector[index]->bshopValidated = true;

                        delete pChart;
                    }
                    
                }
            }
        
        
        return queryResult;
}

    

int getChartList( bool bShowErrorDialogs = true){
    
    // We query the server for the list of charts associated with our account
    wxString url = userURL;
    if(g_admin)
        url = adminURL;
    
    url +=_T("?fc=module&module=occharts&controller=apioesu");
    
    wxString loginParms;
    loginParms += _T("taskId=getlist");
    loginParms += _T("&username=") + g_loginUser;
    loginParms += _T("&key=") + g_loginKey;
    if(g_debugShop.Len())
        loginParms += _T("&debug=") + g_debugShop;
    loginParms += _T("&version=") + g_systemOS + g_versionString;

    int iResponseCode = 0;
    size_t res = 0;
    std::string responseBody;
    
#ifdef __OCPN_USE_CURL__    
    wxCurlHTTPNoZIP post;
    post.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
    
    res = post.Post( loginParms.ToAscii(), loginParms.Len(), url );
    
    // get the response code of the server
    
    post.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
    
    std::string a = post.GetDetailedErrorString();
    std::string b = post.GetErrorString();
    std::string c = post.GetResponseBody();
    
    responseBody = post.GetResponseBody();
    printf("%s", post.GetResponseBody().c_str());
    
    //wxString tt(post.GetResponseBody().data(), wxConvUTF8);
    //wxLogMessage(tt);
#else
     wxString postresult;
    qDebug() << url.mb_str();
    qDebug() << loginParms.mb_str();

    _OCPN_DLStatus stat = OCPN_postDataHttp( url, loginParms, postresult, g_timeout_secs );

    qDebug() << "getChartList Post Stat: " << stat;
    
    if(stat != OCPN_DL_FAILED){
        wxCharBuffer buf = postresult.ToUTF8();
        std::string response(buf.data());
        
        qDebug() << response.c_str();
        responseBody = response.c_str();
        iResponseCode = 200;
        res = 1;
    }

#endif    
    
    if(iResponseCode == 200){
        wxString result = ProcessResponse(responseBody);
        
        //  Scan for and delete any chartsets that are not recognized in server response
        for (auto it = ChartVector.begin(); it != ChartVector.end(); ) {
            if (!(*it)->bshopValidated) {
                it = ChartVector.erase(it);
            } else {
                ++it;
            }
        }

        return checkResult( result, bShowErrorDialogs );
    }
    else
        return checkResponseCode(iResponseCode);
}


int doAssign(itemChart *chart, int qtyIndex, wxString systemName)
{
    wxString msg = _("This action will PERMANENTLY assign the chart:");
    msg += _T("\n"); //        ");
    msg += chart->chartName;
    msg += _T("\n\n");
    msg += _("to this systemName:");
    msg += _T("\n        ");
    msg += systemName;
    if(systemName.StartsWith("sgl")){
        msg += _T(" (") + _("USB Key Dongle") + _T(")");
    }
    
    msg += _T("\n\n");
    msg += _("Proceed?");
    
    int ret = ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxYES_NO);
    
    if(ret != wxID_YES){
        return 1;
    }
        
    // Assign a chart to this system name
    wxString url = userURL;
    if(g_admin)
        url = adminURL;
    
    url +=_T("?fc=module&module=occharts&controller=apioesu");
    
    wxString loginParms;
    loginParms += _T("taskId=assign");
    loginParms += _T("&username=") + g_loginUser;
    loginParms += _T("&key=") + g_loginKey;
    if(g_debugShop.Len())
        loginParms += _T("&debug=") + g_debugShop;

    loginParms += _T("&systemName=") + systemName;
    loginParms += _T("&order=") + chart->orderRef;
    loginParms += _T("&chartid=") + chart->chartID;
    wxString sqid;
    sqid.Printf(_T("%1d"), chart->quantityList[qtyIndex].quantityId);
    loginParms += _T("&quantityId=") + sqid;
    loginParms += _T("&version=") + g_systemOS + g_versionString;
    
    int iResponseCode = 0;
    size_t res = 0;
    std::string responseBody;

#ifdef __OCPN_USE_CURL__    
    wxCurlHTTPNoZIP post;
    post.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
    res = post.Post( loginParms.ToAscii(), loginParms.Len(), url );
    
    // get the response code of the server
    post.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
    if(iResponseCode == 200)
        responseBody= post.GetResponseBody();
#else
    qDebug() << "do assign";
    wxString postresult;
    _OCPN_DLStatus stat = OCPN_postDataHttp( url, loginParms, postresult, 5 );

    qDebug() << "doAssign Post Stat: " << stat;

    if(stat != OCPN_DL_FAILED){
        wxCharBuffer buf = postresult.ToUTF8();
        std::string response(buf.data());
        
        qDebug() << response.c_str();
        responseBody = response.c_str();
       
        iResponseCode = 200;
    }

#endif
    
    if(iResponseCode == 200){
        wxString result = ProcessResponse(responseBody);

        if(result.IsSameAs(_T("1"))){                    // Good result
            // Create a new slot and record the assigned slotUUID, etc
            itemSlot *slot = new itemSlot;
            slot->assignedSystemName = systemName.mb_str();
            slot->slotUuid = g_lastSlotUUID.mb_str();
            chart->quantityList[qtyIndex].slotList.push_back(slot);
            return 0;
        }
        
        return checkResult(result);
    }
    else
        return checkResponseCode(iResponseCode);
}


int doUploadXFPR(bool bDongle)
{
    wxString err;
    
    // Generate the FPR file
    bool b_copyOK = false;
    
    wxString fpr_file = getFPR( false, b_copyOK, bDongle);              // No copy needed
    
    fpr_file = fpr_file.Trim(false);            // Trim leading spaces...
    
    if(fpr_file.Len()){
        
        wxString stringFPR;
        
        //Read the file, convert to ASCII hex, and build a string
        if(::wxFileExists(fpr_file)){
            wxString stringFPR;
            wxFileInputStream stream(fpr_file);
            while(stream.IsOk() && !stream.Eof() ){
                unsigned char c = stream.GetC();
                if(!stream.Eof()){
                    wxString sc;
                    sc.Printf(_T("%02X"), c);
                    stringFPR += sc;
                }
            }
            
            // Prepare the upload command string
            wxString url = userURL;
            if(g_admin)
                url = adminURL;
            
            url +=_T("?fc=module&module=occharts&controller=apioesu");
            
            wxFileName fnxpr(fpr_file);
            wxString fprName = fnxpr.GetFullName();
            
            wxString loginParms;
            loginParms += _T("taskId=xfpr");
            loginParms += _T("&username=") + g_loginUser;
            loginParms += _T("&key=") + g_loginKey;
            if(g_debugShop.Len())
                loginParms += _T("&debug=") + g_debugShop;

            if(!bDongle)
                loginParms += _T("&systemName=") + g_systemName;
            else
                loginParms += _T("&systemName=") + g_dongleName;
                
            loginParms += _T("&xfpr=") + stringFPR;
            loginParms += _T("&xfprName=") + fprName;
            loginParms += _T("&version=") + g_systemOS + g_versionString;
            
            wxLogMessage(loginParms);

            int iResponseCode = 0;
            size_t res = 0;
            std::string responseBody;
            
#ifdef __OCPN_USE_CURL__            
            wxCurlHTTPNoZIP post;
            post.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
            res = post.Post( loginParms.ToAscii(), loginParms.Len(), url );
            
            // get the response code of the server
            post.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
            if(iResponseCode == 200)
                responseBody = post.GetResponseBody();

#else
            qDebug() << "do xfpr upload";
            wxString postresult;
            _OCPN_DLStatus stat = OCPN_postDataHttp( url, loginParms, postresult, 5 );

            qDebug() << "doUploadXFPR Post Stat: " << stat;

            
            if(stat != OCPN_DL_FAILED){
                wxCharBuffer buf = postresult.ToUTF8();
                std::string response(buf.data());
        
                qDebug() << response.c_str();
                responseBody = response.c_str();
       
                iResponseCode = 200;
            }

#endif            
            if(iResponseCode == 200){
                wxString result = ProcessResponse(responseBody);
                
                int iret = checkResult(result);
                
                return iret;
            }
            else
                return checkResponseCode(iResponseCode);
            
        }
        else if(fpr_file.IsSameAs(_T("DONGLE_NOT_PRESENT")))
            err = _("  {USB Dongle not found.}");
            
        else
            err = _("  {fpr file not found.}");
    }
    else{
        err = _("  {fpr file not created.}");
    }
    
    if(err.Len()){
        wxString msg = _("ERROR Creating Fingerprint file") + _T("\n");
        msg += _("Check OpenCPN log file.") + _T("\n"); 
        msg += err;
        ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxOK);
        return 1;
    }
        
    return 0;
}


int doPrepare(oeXChartPanel *chartPrepare, itemSlot *slot)
{
    // Request a chart preparation
    wxString url = userURL;
    if(g_admin)
        url = adminURL;
    
    url +=_T("?fc=module&module=occharts&controller=apioesu");
    
   
    itemChart *chart = chartPrepare->GetSelectedChart();
    
    
/*
                        a. taskId: request
                        b. username
                        c. key
                        d. assignedSystemName: The current systemName assigned to the machine or connected dongle.
                        e. slotUuid: got after request 4
                        f. requestedFile: base|update (all files or just updated files from previous versions)
                        g. requestedEdition: E-U
                        h. currentEdition: E-U (it could be void if requestedFile = base but it can not if requestedFile = update)
                        i. debug
*/       
    wxString loginParms;
    loginParms += _T("taskId=request");
    loginParms += _T("&username=") + g_loginUser;
    loginParms += _T("&key=") + g_loginKey;
    if(g_debugShop.Len())
        loginParms += _T("&debug=") + g_debugShop;

    loginParms += _T("&assignedSystemName=") + wxString(slot->assignedSystemName.c_str());
    loginParms += _T("&slotUuid=") + wxString(slot->slotUuid.c_str());
    loginParms += _T("&requestedFile=") + chart->taskRequestedFile;
    loginParms += _T("&requestedVersion=") + chart->taskRequestedEdition;
    loginParms += _T("&currentVersion=") + chart->taskCurrentEdition;
    loginParms += _T("&version=") + g_systemOS + g_versionString;
    
    wxLogMessage(loginParms);
    
    int iResponseCode = 0;
    size_t res = 0;
    std::string responseBody;
            
#ifdef __OCPN_USE_CURL__            
    wxCurlHTTPNoZIP post;
    post.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
    res = post.Post( loginParms.ToAscii(), loginParms.Len(), url );
    
    // get the response code of the server
    post.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
    if(iResponseCode == 200)
        responseBody = post.GetResponseBody();
#else
    wxString postresult;
    qDebug() << url.mb_str();
    qDebug() << loginParms.mb_str();

    _OCPN_DLStatus stat = OCPN_postDataHttp( url, loginParms, postresult, 5 );

    qDebug() << "doPrepare Post Stat: " << stat;
    
    if(stat != OCPN_DL_FAILED){
        wxCharBuffer buf = postresult.ToUTF8();
        std::string response(buf.data());
        
        qDebug() << response.c_str();
        responseBody = response.c_str();
        iResponseCode = 200;
        res = 1;
    }


#endif
    
    if(iResponseCode == 200){
        // Expecting complex links with embedded entities, so process the "&" correctly
        wxString result = ProcessResponse(responseBody, true);
        
        return checkResult(result);
    }
    else
        return checkResponseCode(iResponseCode);
    

    return 0;
}

int doDownload(itemChart *targetChart, itemSlot *targetSlot)
{
    //  Create the download queue for all files necessary.

    wxString Prefix = _T("oeRNC");
    if(targetChart->GetChartType() == CHART_TYPE_OEUSENC)
        Prefix = _T("oeuSENC");
    
    targetSlot->dlQueue.clear();
    
    for(unsigned int i=0 ; i < targetSlot->taskFileList.size() ; i++){
    
        // First, the shorter Key file
        itemDLTask task1;
        wxString downloadURL = wxString(targetSlot->taskFileList[i]->linkKeys.c_str());
        
        //  /oeRNC-IMR-CRBeast-1-0-base-hp64linux.XML
        wxString fileTarget = Prefix +_T("-") + wxString(targetChart->chartID.c_str()) + _T("-") + wxString(targetChart->serverChartEdition.c_str());
        fileTarget += _T("-base-");
        if(g_dongleName.Length())
            fileTarget +=  g_dongleName + _T(".XML");
        else
            fileTarget +=  g_systemName + _T(".XML");


        task1.url = downloadURL;
        task1.localFile = wxString(g_PrivateDataDir + _T("DownloadCache") + wxFileName::GetPathSeparator() + fileTarget).mb_str();
        task1.SHA256 = targetSlot->taskFileList[i]->sha256Keys;
        targetSlot->taskFileList[i]->cacheKeysLocn = task1.localFile;
        targetSlot->dlQueue.push_back(task1);
        
        // Next, the chart payload file
        itemDLTask task2;
        downloadURL = wxString(targetSlot->taskFileList[i]->link.c_str());

        //  /oeRNC-IMR-GR-2-0-base.zip
        fileTarget = Prefix + _T("-") + wxString(targetChart->chartID.c_str()) + _T("-") + wxString(targetChart->serverChartEdition.c_str());
        fileTarget += _T("-base.zip");

        task2.url = downloadURL;
        task2.localFile = wxString(g_PrivateDataDir + _T("DownloadCache") + wxFileName::GetPathSeparator() + fileTarget).mb_str();
        task2.SHA256 = targetSlot->taskFileList[i]->sha256;
        targetSlot->taskFileList[i]->cacheLinkLocn = task2.localFile;
        targetSlot->dlQueue.push_back(task2);
    }
    
    // Store the targetSlot pointer globally for general access
    gtargetSlot = targetSlot;
    gtargetSlot->idlQueue = 0;
    gtargetChart = targetChart;
    
    //Send an event to kick off the download chain
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED);
    event.SetId( ID_CMD_BUTTON_INSTALL_CHAIN );
    g_shopPanel->GetEventHandler()->AddPendingEvent(event);

    return 0;
}

bool ExtractZipFiles( const wxString& aZipFile, const wxString& aTargetDir, bool aStripPath, wxDateTime aMTime, bool aRemoveZip )
{
    bool ret = true;
#ifdef __OCPN__ANDROID__
    int nStrip = 0;
    if(aStripPath)
        nStrip = 1;
    
    ret = AndroidUnzip(aZipFile, aTargetDir, nStrip, false);
#else
    
    std::unique_ptr<wxZipEntry> entry(new wxZipEntry());
    
    do
    {
        //wxLogError(_T("chartdldr_pi: Going to extract '")+aZipFile+_T("'."));
        wxFileInputStream in(aZipFile);
        
        if( !in )
        {
            wxLogError(_T("Can not open file '")+aZipFile+_T("'."));
            ret = false;
            break;
        }
        wxZipInputStream zip(in);
        ret = false;

        if(g_ipGauge)
            g_ipGauge->Start();
        
        while( entry.reset(zip.GetNextEntry()), entry.get() != NULL )
        {
            // access meta-data
            wxString name = entry->GetName();
            if( aStripPath )
            {
                wxFileName fn(name);
                /* We can completly replace the entry path */
                //fn.SetPath(aTargetDir);
                //name = fn.GetFullPath();
                /* Or only remove the first dir (eg. ENC_ROOT) */
                if (fn.GetDirCount() > 0)
                    fn.RemoveDir(0);
                name = aTargetDir + wxFileName::GetPathSeparator() + fn.GetFullPath();
            }
            else
            {
                name = aTargetDir + wxFileName::GetPathSeparator() + name;
            }
            
            // read 'zip' to access the entry's data
            if( entry->IsDir() )
            {
                int perm = entry->GetMode();
                if( !wxFileName::Mkdir(name, perm, wxPATH_MKDIR_FULL) )
                {
                    wxLogError(_T("Can not create directory '") + name + _T("'."));
                    ret = false;
                    break;
                }
            }
            else
            {
                if( !zip.OpenEntry(*entry.get()) )
                {
                    wxLogError(_T("Can not open zip entry '") + entry->GetName() + _T("'."));
                    ret = false;
                    break;
                }
                if( !zip.CanRead() )
                {
                    wxLogError(_T("Can not read zip entry '") + entry->GetName() + _T("'."));
                    ret = false;
                    break;
                }
                
                wxFileName fn(name);
                if( !fn.DirExists() )
                {
                    if( !wxFileName::Mkdir(fn.GetPath(), 0755, wxPATH_MKDIR_FULL) )
                    {
                        wxLogError(_T("Can not create directory '") + fn.GetPath() + _T("'."));
                        ret = false;
                        break;
                    }
                }
                
                wxFileOutputStream file(name);
                
                g_shopPanel->setStatusText( _("Unzipping chart files...") + fn.GetFullName());
                
                if(g_ipGauge)
                    g_ipGauge->Pulse();
                wxYield();
                
                if( !file )
                {
                    wxLogError(_T("Can not create file '")+name+_T("'."));
                    ret = false;
                    break;
                }
                zip.Read(file);
                fn.SetTimes(&aMTime, &aMTime, &aMTime);
                ret = true;
            }
            
        }
        
    }
    while(false);
    
    if( aRemoveZip )
        wxRemoveFile(aZipFile);

#endif
    
    if(g_ipGauge)
        g_ipGauge->Stop();

    return ret;
}



    

int doShop(){
    
    loadShopConfig();
   
    // Check the dongle
    g_dongleName.Clear();
    if(IsDongleAvailable()){
        g_dongleSN = GetDongleSN();
        char sName[20];
        snprintf(sName, 19, "sgl%08X", g_dongleSN);

        g_dongleName = wxString(sName);
    }

    if(g_shopPanel)
        g_shopPanel->RefreshSystemName();

    //  Do we need an initial login to get the persistent key?
    if(g_loginKey.Len() == 0){
        doLogin( g_shopPanel );
        saveShopConfig();
    }
    
    getChartList();
    
    return 0;
}


class MyStaticTextCtrl : public wxStaticText {
public:
    MyStaticTextCtrl(wxWindow* parent,
                                       wxWindowID id,
                                       const wxString& label,
                                       const wxPoint& pos,
                                       const wxSize& size = wxDefaultSize,
                                       long style = 0,
                                       const wxString& name= "staticText" ):
                                       wxStaticText(parent,id,label,pos,size,style,name){};
                                       void OnEraseBackGround(wxEraseEvent& event) {};
                                       DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(MyStaticTextCtrl,wxStaticText)
EVT_ERASE_BACKGROUND(MyStaticTextCtrl::OnEraseBackGround)
END_EVENT_TABLE()


BEGIN_EVENT_TABLE(oeXChartPanel, wxPanel)
EVT_PAINT ( oeXChartPanel::OnPaint )
EVT_ERASE_BACKGROUND(oeXChartPanel::OnEraseBackground)
END_EVENT_TABLE()

oeXChartPanel::oeXChartPanel(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, itemChart *p_itemChart, shopPanel *pContainer)
:wxPanel(parent, id, pos, size, wxBORDER_NONE)
{
    m_pContainer = pContainer;
    m_pChart = p_itemChart;
    m_bSelected = false;

    int refHeight = GetCharHeight();
    SetMinSize(wxSize(-1, 5 * refHeight));
    m_unselectedHeight = 5 * refHeight;
    
#ifdef __OCPN__ANDROID__
    Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(oeXChartPanel::OnChartSelected), NULL, this);
#else    
     Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(oeXChartPanel::OnChartSelected), NULL, this);
#endif     
    
}

oeXChartPanel::~oeXChartPanel()
{
}

void oeXChartPanel::OnChartSelected( wxMouseEvent &event )
{
    // Do not allow de-selection by mouse if this chart is busy, i.e. being prepared, or being downloaded 
    if(m_pChart){
       if(g_statusOverride.Length())
           return;
    }
           
    if(!m_bSelected){
        SetSelected( true );
        m_pContainer->SelectChart( this );
    }
    else{
        SetSelected( false );
        m_pContainer->SelectChart( (oeXChartPanel*)NULL );
    }
}

void oeXChartPanel::SetSelected( bool selected )
{
    m_bSelected = selected;
    wxColour colour;
    int refHeight = GetCharHeight();
    int width, height;
    GetSize( &width, &height );
 
    if (selected)
    {
        GetGlobalColor(_T("UIBCK"), &colour);
        m_boxColour = colour;

        // Calculate minimum size required
        int lcount = 9;
        if(width < (30 *refHeight)){
            lcount += 2;
        }

        if(m_pChart){
        int nAssign = 0;
            for(unsigned int i=0 ; i < m_pChart->quantityList.size() ; i++){
                itemQuantity Qty = m_pChart->quantityList[i];
                nAssign += Qty.slotList.size();
            }
            SetMinSize(wxSize(-1, (lcount + nAssign) * refHeight));
        }
        else
            SetMinSize(wxSize(-1, 5 * refHeight));
    }
    else
    {
        GetGlobalColor(_T("DILG0"), &colour);
        m_boxColour = colour;
        SetMinSize(wxSize(-1, 5 * refHeight));
    }
    
    Refresh( true );
    
}


extern "C"  DECL_EXP bool GetGlobalColor(wxString colorName, wxColour *pcolour);

void oeXChartPanel::OnEraseBackground( wxEraseEvent &event )
{
}

void oeXChartPanel::OnPaint( wxPaintEvent &event )
{
    int width, height;
    GetSize( &width, &height );
    wxPaintDC dc( this );
 
    //dc.SetBackground(*wxLIGHT_GREY);
    
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.DrawRectangle(GetVirtualSize());
    
    wxColour c;
    
    wxString nameString = m_pChart->chartName;
    //if(!m_pChart->quantityId.IsSameAs(_T("1")))
      //  nameString += _T(" (") + m_pChart->quantityId + _T(")");
    
    // Thumbnail border color depends on chart type and status
    wxColor thumbColor;
    if(m_pChart->GetChartType() == CHART_TYPE_OERNC){
        if( m_pChart->isChartsetExpired() || ( m_pChart->isChartsetFullyAssigned()  &&  !m_pChart->isChartsetAssignedToMe()))
            GetGlobalColor( "BLUE2", &thumbColor );
        else
            GetGlobalColor( "BLUE2", &thumbColor );
    }
    else if(m_pChart->GetChartType() == CHART_TYPE_OEUSENC){
        if( m_pChart->isChartsetExpired() || ( m_pChart->isChartsetFullyAssigned()  &&  !m_pChart->isChartsetAssignedToMe()))
            GetGlobalColor( "GREEN2", &thumbColor );
        else
            GetGlobalColor( "GREEN2", &thumbColor );
    }
    else
        thumbColor = wxColor(164,164,164);

    if(!g_chartListUpdatedOK)
        thumbColor = wxColor(220,220,220);

        
    int thumbColorWidth = 5;
    
    if(m_bSelected){
        bool bCompact = false;
        if(width < (30 *GetCharHeight()))
            bCompact = true;
        
        dc.SetBrush( wxBrush( m_boxColour ) );
        
        GetGlobalColor( _T ( "UITX1" ), &c );
        dc.SetPen( wxPen( wxColor(0xCE, 0xD5, 0xD6), 3 ));
        
        dc.DrawRoundedRectangle( 0, 0, width-1, height-1, height / 10);
        
        int base_offset = height / 10;
        
        // Draw the thumbnail
        int scaledWidth = height;
        
        int scaledHeight = (height - (2 * base_offset)) * 95 / 100;
        if(bCompact){
            scaledHeight = (height - (2 * base_offset)) * 50 / 100;
            scaledWidth = scaledHeight;
        }

        wxBitmap& bm = m_pChart->GetChartThumbnail( scaledHeight );
        wxSize bmSize = bm.GetSize();

        dc.SetBrush( wxBrush( thumbColor ));
            
        dc.SetPen( wxPen( wxColor(0xCE, 0xD5, 0xD6), 1 ));
        dc.DrawRectangle( base_offset + 3 - thumbColorWidth, base_offset + 3 - thumbColorWidth,
                          bmSize.x + thumbColorWidth * 2, bmSize.y + thumbColorWidth * 2);

        if(bm.IsOk())
            dc.DrawBitmap(bm, base_offset + 3, base_offset + 3);

        dc.SetPen( wxPen( wxColor(0xCE, 0xD5, 0xD6), 3 ));

        bool bsplit_line = false;

        int text_x = scaledWidth * 11 / 10;
        int text_x_val = scaledWidth + ((width - scaledWidth) * 45/ 100);
        int yPitch = GetCharHeight();

        wxFont *dFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
        double font_size = dFont->GetPointSize() * 3/2;

        if(bCompact){
            bsplit_line = true;
            text_x = scaledWidth * 14 / 10;
            text_x_val = text_x + (3 * yPitch);
            font_size = dFont->GetPointSize() * 5/4;
        }

        wxFont *qFont = wxTheFontList->FindOrCreateFont( font_size, dFont->GetFamily(), dFont->GetStyle(), dFont->GetWeight());

        // Make sure the text fits, adjust font size to make it so...
        int nameWidth, nameHeight;
        int nameWidthAvail = width - text_x - 5;
        dc.GetTextExtent( nameString, &nameWidth, &nameHeight, NULL, NULL, qFont);
        bool bfit = nameWidth < nameWidthAvail;
        while (!bfit && (font_size > 8)){
            font_size--;
            qFont = wxTheFontList->FindOrCreateFont( font_size, dFont->GetFamily(), dFont->GetStyle(), dFont->GetWeight());
            dc.GetTextExtent( nameString, &nameWidth, &nameHeight, NULL, NULL, qFont);
            bfit = nameWidth < nameWidthAvail;
        }
            
        dc.SetFont( *qFont );
        dc.SetTextForeground(wxColour(0,0,0));
        dc.DrawText(nameString, text_x, height * 5 / 100);

        int hTitle = dc.GetCharHeight();
        int y_line = (height * 5 / 100) + hTitle;

        dc.SetFont( *dFont );           // Restore default font
        int offset = GetCharHeight();
        
        dc.DrawLine( text_x, y_line, width - base_offset, y_line);

        int yPos = y_line + 4;
        wxString tx;
        
        // Create and populate the current chart information
        tx = _("Installed Chart Edition:");
        dc.DrawText( tx, text_x, yPos);
        tx = m_pChart->GetDisplayedChartEdition();
        dc.DrawText( tx, text_x_val, yPos);
        yPos += yPitch;
        
        tx = _("Current Chart Edition:");
        dc.DrawText( tx, text_x, yPos);
        tx = m_pChart->serverChartEdition;
        dc.DrawText( tx, text_x_val, yPos);
        yPos += yPitch;
        
        tx = _("Status:");
        dc.DrawText( tx, text_x, yPos);
        tx = m_pChart->getStatusString();
        if(g_statusOverride.Len())
            tx = g_statusOverride;
        else{
            if(m_pChart->isChartsetExpired()){
                dc.SetTextForeground(wxColour(200,0,0));
            }
        }
                
        dc.DrawText( tx, text_x_val, yPos);
        yPos += yPitch;
        dc.SetTextForeground(wxColour(0,0,0));

        tx = _("Order Reference:");
        dc.DrawText( tx, text_x, yPos);
        if(bsplit_line) yPos += yPitch;
        tx = m_pChart->orderRef;
        dc.DrawText( tx, text_x_val, yPos);
        yPos += yPitch;
        
        tx = _("Updates available through:");
        dc.DrawText( tx, text_x, yPos);
        if(bsplit_line) yPos += yPitch;
        size_t n = m_pChart->expDate.find_first_of(' ');
        if(n != std::string::npos)
            tx = m_pChart->expDate.substr(0,n);
        else
            tx = m_pChart->expDate;
        dc.DrawText( tx, text_x_val, yPos);
        yPos += yPitch;
        

        tx = _("Assignments:");
        int assignedCount = m_pChart->getChartAssignmentCount();
        int availCount = m_pChart->quantityList.size() * m_pChart->maxSlots;
        wxString acv;
        acv.Printf(_T("   %d/%d"), assignedCount, availCount);
        if(assignedCount)
            tx += acv;
        else
            tx += _("    None");
        dc.DrawText( tx, text_x, yPos);
        yPos += yPitch;

        wxString id;
        int nid = 1;
        for(unsigned int i=0 ; i < m_pChart->quantityList.size() ; i++){
            itemQuantity Qty = m_pChart->quantityList[i];
            for(unsigned int j = 0 ; j < Qty.slotList.size() ; j++){
                itemSlot *slot = Qty.slotList[j];
                tx = m_pChart->getKeytypeString(slot->slotUuid);
                id.Printf(_("%d) "), nid);
                tx.Prepend(id);
                tx += _T("    ") + wxString(slot->assignedSystemName.c_str());
                //tx += _T(" ") + wxString(slot->slotUuid.c_str());
                dc.DrawText( tx, text_x_val, yPos);
                yPos += yPitch;
                nid++;
            }
        }
    }
    else{
        dc.SetBrush( wxBrush( m_boxColour ) );
    
        GetGlobalColor( _T ( "UITX1" ), &c );
        dc.SetPen( wxPen( c, 1 ) );
    
        int offset = height / 10;
        dc.DrawRectangle( offset, offset, width - (2 * offset), height - (2 * offset));
    
        // Draw the thumbnail
        int scaledHeight = (height - (2 * offset)) * 95 / 100;
        wxBitmap& bm = m_pChart->GetChartThumbnail( scaledHeight );
        wxSize bmSize = bm.GetSize();
        
        dc.SetBrush( wxBrush( thumbColor ));
            
        //dc.DrawRectangle( offset, offset, scaledHeight + 6, scaledHeight + 6);
        dc.DrawRectangle( offset + 3 - thumbColorWidth, offset + 3 - thumbColorWidth,
                          bmSize.x + thumbColorWidth * 2, bmSize.y + thumbColorWidth * 2);

        
        if(bm.IsOk()){
            dc.DrawBitmap(bm, offset + 3, offset + 3);
        }
        
        int scaledWidth = bm.GetWidth() * scaledHeight / bm.GetHeight();
        
        int text_x = scaledWidth * 15 / 10;
        
        wxFont *dFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
        double font_size = dFont->GetPointSize() * 3/2;
        wxFont *qFont = wxTheFontList->FindOrCreateFont( font_size, dFont->GetFamily(), dFont->GetStyle(), dFont->GetWeight());

        // Make sure the text fits, adjust font size to make it so...
        int nameWidth, nameHeight;
        int nameWidthAvail = width - text_x - 5;
        dc.GetTextExtent( nameString, &nameWidth, &nameHeight, NULL, NULL, qFont);
        bool bfit = nameWidth < nameWidthAvail;
        while (!bfit && (font_size > 8)){
            font_size--;
            qFont = wxTheFontList->FindOrCreateFont( font_size, dFont->GetFamily(), dFont->GetStyle(), dFont->GetWeight());
            dc.GetTextExtent( nameString, &nameWidth, &nameHeight, NULL, NULL, qFont);
            bfit = nameWidth < nameWidthAvail;
        }

        dc.SetFont( *qFont );
        dc.SetTextForeground(wxColour(64, 64, 64));
        if( m_pChart->isChartsetFullyAssigned()  &&  !m_pChart->isChartsetAssignedToMe())
            dc.SetTextForeground(wxColour(128,128,128));

        if( m_pChart->isChartsetExpired())
            dc.SetTextForeground(wxColour(128,128,128));
        
        if(m_pContainer->GetSelectedChartPanel())
            dc.SetTextForeground(wxColour(220,220,220));
        
        dc.DrawText(nameString, text_x, height * 35 / 100);
        
        if(m_pChart->isChartsetExpired()){
            wxFont *iFont = wxTheFontList->FindOrCreateFont( font_size, dFont->GetFamily(), wxFONTSTYLE_ITALIC, dFont->GetWeight());
            dc.SetFont( *iFont );
            dc.DrawText(_("Expired"), scaledWidth * 18 / 10, height * 60 / 100);
        }
    }
}


BEGIN_EVENT_TABLE( chartScroller, wxScrolledWindow )
//EVT_PAINT(chartScroller::OnPaint)
EVT_ERASE_BACKGROUND(chartScroller::OnEraseBackground)
END_EVENT_TABLE()

chartScroller::chartScroller(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
: wxScrolledWindow(parent, id, pos, size, style)
{
    int yyp = 3;
}

void chartScroller::OnEraseBackground(wxEraseEvent& event)
{
    wxASSERT_MSG
    (
        GetBackgroundStyle() == wxBG_STYLE_ERASE,
     "shouldn't be called unless background style is \"erase\""
    );
    
    wxDC& dc = *event.GetDC();
    dc.SetPen(*wxGREEN_PEN);
    
    // clear any junk currently displayed
    dc.Clear();
    
    PrepareDC( dc );
    
    const wxSize size = GetVirtualSize();
    for ( int x = 0; x < size.x; x += 15 )
    {
        dc.DrawLine(x, 0, x, size.y);
    }
    
    for ( int y = 0; y < size.y; y += 15 )
    {
        dc.DrawLine(0, y, size.x, y);
    }
    
    dc.SetTextForeground(*wxRED);
    dc.SetBackgroundMode(wxSOLID);
    dc.DrawText("This text is drawn from OnEraseBackground", 60, 160);
    
}

void chartScroller::DoPaint(wxDC& dc)
{
    PrepareDC(dc);
    
//    if ( m_eraseBgInPaint )
    {
        dc.SetBackground(*wxRED_BRUSH);
        
        // Erase the entire virtual area, not just the client area.
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(GetBackgroundColour());
        dc.DrawRectangle(GetVirtualSize());
        
        dc.DrawText("Background erased in OnPaint", 65, 110);
    }
//     else if ( GetBackgroundStyle() == wxBG_STYLE_PAINT )
//     {
//         dc.SetTextForeground(*wxRED);
//         dc.DrawText("You must enable erasing background in OnPaint to avoid "
//         "display corruption", 65, 110);
//     }
//     
//     dc.DrawBitmap( m_bitmap, 20, 20, true );
//     
//     dc.SetTextForeground(*wxRED);
//     dc.DrawText("This text is drawn from OnPaint", 65, 65);
}

void chartScroller::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
//     if ( m_useBuffer )
//     {
//         wxAutoBufferedPaintDC dc(this);
//         DoPaint(dc);
//     }
//     else
    {
        wxPaintDC dc(this);
        DoPaint(dc);
    }
}


BEGIN_EVENT_TABLE( shopPanel, wxPanel )
EVT_TIMER( 4357, shopPanel::OnPrepareTimer )
EVT_BUTTON( ID_CMD_BUTTON_INSTALL, shopPanel::OnButtonInstall )
EVT_BUTTON( ID_CMD_BUTTON_INSTALL_CHAIN, shopPanel::OnButtonInstallChain )
EVT_BUTTON( ID_CMD_BUTTON_VALIDATE, shopPanel::ValidateChartset )
END_EVENT_TABLE()


shopPanel::shopPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
: wxPanel(parent, id, pos, size, style)
{
    m_shopLog = NULL;
    g_shopLogFrame = NULL;
    m_validator = NULL;
    m_bconnected = false;

    loadShopConfig();
    
#ifdef __OCPN_USE_CURL__
    g_CurlEventHandler = new OESENC_CURL_EvtHandler;
#endif
    
    g_shopPanel = this;
    m_binstallChain = false;
    m_bAbortingDownload = false;
    
    m_ChartPanelSelected = NULL;
    m_choiceSystemName = NULL;
    int ref_len = GetCharHeight();
    
    bool bCompact = false;
    wxSize sz = ::wxGetDisplaySize();
    //if((sz.x < 500) | (sz.y < 500))
        bCompact = true;

    wxBoxSizer* boxSizerTop = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(boxSizerTop);

    wxString sn = _("System Name:");
    sn += _T(" ");
    if(g_systemName.Length())
        sn += g_systemName;
    //else
    //    sn += _T("                                            ");
#ifndef __OCPN__ANDROID__     
    wxFlexGridSizer *sysBox = new wxFlexGridSizer(2);
    sysBox->AddGrowableCol(0);
    boxSizerTop->Add(sysBox, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
   
    m_staticTextSystemName = new wxStaticText(this, wxID_ANY, sn, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    //m_staticTextSystemName->Wrap(-1);
    sysBox->Add(m_staticTextSystemName, 1, wxALL | wxALIGN_LEFT, WXC_FROM_DIP(5));

    m_buttonUpdate = new wxButton(this, wxID_ANY, _("Refresh Chart List"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    m_buttonUpdate->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(shopPanel::OnButtonUpdate), NULL, this);
    sysBox->Add(m_buttonUpdate, 1, wxRIGHT | wxALIGN_RIGHT, WXC_FROM_DIP(5));
#else
    wxBoxSizer *sysBox = new wxBoxSizer(wxVERTICAL);
    boxSizerTop->Add(sysBox, 0, wxTOP | wxEXPAND, WXC_FROM_DIP(5));
   
    m_staticTextSystemName = new wxStaticText(this, wxID_ANY, sn, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    sysBox->Add(m_staticTextSystemName, 0, wxLEFT | wxALIGN_LEFT | wxEXPAND, WXC_FROM_DIP(5));

    m_buttonUpdate = new wxButton(this, wxID_ANY, _("Refresh Chart List"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    m_buttonUpdate->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(shopPanel::OnButtonUpdate), NULL, this);
    sysBox->Add(m_buttonUpdate, 0, wxRIGHT | wxALIGN_RIGHT, WXC_FROM_DIP(5));
#endif
   
    int add_prop_flag = 0;
#ifdef __OCPN__ANDROID__
    add_prop_flag = 1;
#endif
    
    wxStaticBoxSizer* staticBoxSizerChartList = new wxStaticBoxSizer( new wxStaticBox(this, wxID_ANY, _("My Chart Sets")), wxVERTICAL);
    boxSizerTop->Add(staticBoxSizerChartList, add_prop_flag, wxALL|wxEXPAND, WXC_FROM_DIP(5));

    wxPanel *cPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxBG_STYLE_ERASE );
    staticBoxSizerChartList->Add(cPanel, add_prop_flag, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    wxBoxSizer *boxSizercPanel = new wxBoxSizer(wxVERTICAL);
    cPanel->SetSizer(boxSizercPanel);
    
    m_scrollWinChartList = new wxScrolledWindow(cPanel, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxBORDER_RAISED | wxVSCROLL | wxBG_STYLE_ERASE );
    m_scrollWinChartList->SetScrollRate(5, 5);
    boxSizercPanel->Add(m_scrollWinChartList, add_prop_flag, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    boxSizerCharts = new wxBoxSizer(wxVERTICAL);
    m_scrollWinChartList->SetSizer(boxSizerCharts);

    int size_scrollerLines = 15;
    if(bCompact)
        size_scrollerLines = 10;
    
    m_scrollWinChartList->SetMinSize(wxSize(-1,size_scrollerLines * GetCharHeight()));
    staticBoxSizerChartList->SetMinSize(wxSize(-1,(size_scrollerLines + 1) * GetCharHeight()));
    
    wxStaticBoxSizer* staticBoxSizerAction = new wxStaticBoxSizer( new wxStaticBox(this, wxID_ANY, _("Actions")), wxVERTICAL);
    boxSizerTop->Add(staticBoxSizerAction, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));

    m_staticLine121 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxLI_HORIZONTAL);
    staticBoxSizerAction->Add(m_staticLine121, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    ///Buttons
    //wxGridSizer* gridSizerActionButtons = new wxGridSizer(1, 3, 0, 0);
    gridSizerActionButtons = new wxBoxSizer(wxVERTICAL);
    staticBoxSizerAction->Add(gridSizerActionButtons, 1, wxALL|wxEXPAND, WXC_FROM_DIP(2));
    
    m_buttonInstall = new wxButton(this, ID_CMD_BUTTON_INSTALL, _("Reinstall Selection"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    gridSizerActionButtons->Add(m_buttonInstall, 1, wxTOP | wxBOTTOM , WXC_FROM_DIP(2));
    
    m_buttonCancelOp = new wxButton(this, wxID_ANY, _("Cancel Operation"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    m_buttonCancelOp->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(shopPanel::OnButtonCancelOp), NULL, this);
    gridSizerActionButtons->Add(m_buttonCancelOp, 1, wxTOP | wxBOTTOM, WXC_FROM_DIP(2));

     m_buttonValidate = new wxButton(this, ID_CMD_BUTTON_VALIDATE, _("Validate Installed Chart Set"), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
     gridSizerActionButtons->Add(m_buttonValidate, 1, wxTOP | wxBOTTOM, WXC_FROM_DIP(2));
 
    wxStaticLine* sLine1 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxLI_HORIZONTAL);
    staticBoxSizerAction->Add(sLine1, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    
    ///Status
    m_staticTextStatus = new wxStaticText(this, wxID_ANY, _("Status: Chart List Refresh required."), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    //m_staticTextStatus->Wrap(-1);
    staticBoxSizerAction->Add(m_staticTextStatus, 0, wxALL|wxALIGN_LEFT, WXC_FROM_DIP(5));

    g_ipGauge = new InProgressIndicator(this, wxID_ANY, 100, wxDefaultPosition, wxSize(ref_len * 12, ref_len));
    staticBoxSizerAction->Add(g_ipGauge, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, WXC_FROM_DIP(5));

    ///Last Error Message
    m_staticTextLEM = new wxStaticText(this, wxID_ANY, _("Last Error Message: "), wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), 0);
    m_staticTextLEM->Wrap(-1);
    staticBoxSizerAction->Add(m_staticTextLEM, 0, wxALL|wxALIGN_LEFT, WXC_FROM_DIP(5));
    
    m_shopLog = new oesu_piScreenLog(this);
    m_shopLog->SetMinSize(wxSize(-1, 1 * GetCharHeight()));
    boxSizerTop->Add(m_shopLog, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    SetName(wxT("shopPanel"));
    //SetSize(500,600);
    
    if (GetSizer()) {
        GetSizer()->Fit(this);
    }
    
    //  Turn off all buttons initially.
//     m_buttonInstall->Hide();
//     m_buttonCancelOp->Hide();
//     m_staticTextLEM->Hide();
//     m_buttonValidate->Hide();

        // Check the dongle
    g_dongleName.Clear();
    if(IsDongleAvailable()){
        g_dongleSN = GetDongleSN();
        char sName[20];
        snprintf(sName, 19, "sgl%08X", g_dongleSN);

        g_dongleName = wxString(sName);
    }

    RefreshSystemName();
    
    UpdateChartList();
    
}

shopPanel::~shopPanel()
{
    delete m_shopLog;
    delete m_validator;
}

void shopPanel::SetErrorMessage()
{
    if(g_LastErrorMessage.Length()){
        wxString head = _("Last Error Message: ");
        head += g_LastErrorMessage;
        m_staticTextLEM->SetLabel( head );
        m_staticTextLEM->Show();
    }
    else
        m_staticTextLEM->Hide();
    
    g_statusOverride.Clear();
    setStatusText( _("Status: Ready"));
}

void shopPanel::RefreshSystemName()
{
    wxString sn;
    if(g_dongleName.Length()){
        sn = _("System Name:");
        sn += _T(" ");
        sn += g_dongleName + _T(" (") + _("USB Key Dongle") + _T(")");
        m_staticTextSystemName->SetLabel( sn );
    }
    else{
        sn = _("System Name:");
        sn += _T(" ");
        sn += g_systemName;
    }
    
    m_staticTextSystemName->SetLabel(sn);
    m_staticTextSystemName->Refresh();
    wxYield();
}


void shopPanel::SelectChart( oeXChartPanel *panel )
{
    if (m_ChartPanelSelected == panel)
        return;
    
    if (m_ChartPanelSelected)
        m_ChartPanelSelected->SetSelected(false);
    
    m_ChartPanelSelected = panel;
    if (m_ChartPanelSelected)
        m_ChartPanelSelected->SetSelected(true);
    
    m_scrollWinChartList->GetSizer()->Layout();
    
    MakeChartVisible(m_ChartPanelSelected);
    
    UpdateActionControls();
    
    Layout();
    
    Refresh( true );
}


void shopPanel::SelectChartByID( std::string id, std::string order)
{
    for(unsigned int i = 0 ; i < panelVector.size() ; i++){
        itemChart *chart = panelVector[i]->GetSelectedChart();
        if(wxString(id).IsSameAs(chart->chartID) && wxString(order).IsSameAs(chart->orderRef)){
            SelectChart(panelVector[i]);
            MakeChartVisible(m_ChartPanelSelected);
        }
    }
}


void shopPanel::MakeChartVisible(oeXChartPanel *chart)
{
    if(!chart)
        return;
    
    itemChart *vchart = chart->GetSelectedChart();
    
    for(unsigned int i = 0 ; i < panelVector.size() ; i++){
        itemChart *lchart = panelVector[i]->GetSelectedChart();
        if( !strcmp(vchart->chartID.c_str(), lchart->chartID.c_str()) && !strcmp(vchart->orderRef.c_str(), lchart->orderRef.c_str())){
            
            int offset = i * chart->GetUnselectedHeight();
            
            m_scrollWinChartList->Scroll(-1, offset / 5);
        }
    }
    
}


void shopPanel::OnButtonUpdate( wxCommandEvent& event )
{
    m_shopLog->ClearLog();

    loadShopConfig();

#ifdef __OCPN__ANDROID__
    if(!g_systemName.Length()){
        extern wxString androidGetSystemName();
        g_systemName = androidGetSystemName();
        //qDebug() << "Set systemName: " << g_systemName.mb_str();
        saveShopConfig();
    }
#endif    
    
    g_LastErrorMessage.Clear();
    SetErrorMessage();

    // Check the dongle
    bool bDongleFound = false;
    g_dongleName.Clear();
    if(IsDongleAvailable()){
        g_dongleSN = GetDongleSN();
        char sName[20];
        snprintf(sName, 19, "sgl%08X", g_dongleSN);

        g_dongleName = wxString(sName);
        bDongleFound = true;
    }
 
    RefreshSystemName();
    
    //  Do we need an initial login to get the persistent key?
    if(g_loginKey.Len() == 0){
        if(doLogin( g_shopPanel ) != 1)
            return;
        saveShopConfig();
    }
    
     setStatusText( _("Contacting o-charts server..."));
     g_ipGauge->Start();
     wxYield();

    ::wxBeginBusyCursor();
    int err_code = getChartList( false );               // no login error code dialog, we handle here
    ::wxEndBusyCursor();
 
    // Could be a change in login_key, userName, or password.
    // if so, force a full (no_key) login, and retry
    if((err_code == 4) || (err_code == 5) || (err_code == 6)){
        setStatusText( _("Status: Login error."));
        g_ipGauge->Stop();
        wxYield();
        if(doLogin(g_shopPanel) != 1)      // if the second login attempt fails, return to GUI
            return;
        saveShopConfig();
        
        // Try to get the status one more time only.
        ::wxBeginBusyCursor();
        int err_code_2 = getChartList( false );               // no error code dialog, we handle here
        ::wxEndBusyCursor();
        
        if(err_code_2 != 0){                  // Some error on second getlist() try, if so just return to GUI
         
            if((err_code_2 == 4) || (err_code_2 == 5) || (err_code_2 == 6))
                setStatusText( _("Status: Login error."));
            else{
                wxString ec;
                ec.Printf(_T(" { %d }"), err_code_2);
                setStatusText( _("Status: Communications error.") + ec);
            }
            g_ipGauge->Stop();
            wxYield();
            return;
        }
    }
    
    else if(err_code != 0){                  // Some other error
        wxString ec;
        ec.Printf(_T(" { %d }"), err_code);
        setStatusText( _("Status: Communications error.") + ec);
        g_ipGauge->Stop();
        wxYield();
        return;
    }
    g_chartListUpdatedOK = true;
    
    SortChartList();
    
    bool bNeedSystemName = false;
    
    // Has this systemName been disabled?
    if(g_dongleName.Len()){
        if(g_systemNameDisabledArray.Index(g_dongleName) != wxNOT_FOUND){
            bNeedSystemName = true;
            g_dongleName.Clear();
        }
    }
    if(g_systemName.Len()){
        if(g_systemNameDisabledArray.Index(g_systemName) != wxNOT_FOUND){
            bNeedSystemName = true;
            g_systemName.Clear();
        }
    }
        
    // User reset system name, and removed dongle
    if(!g_systemName.Len() && !g_dongleName.Len())
         bNeedSystemName = true;
    
    
    if(bNeedSystemName ){
        // Check shop to see if systemName is already known for this system
        GetShopNameFromFPR();
        
        // If the shop does not know about this system yet, then select an existing or new name by user GUI
        if(!g_systemName.Len() && !g_dongleName.Len()){
            if(g_lastQueryResult.IsSameAs(_T("8l"))){            // very special case, new system, no match
                if(!bDongleFound)                                // no dongle, so only a systemName is expected
                    g_systemName = doGetNewSystemName( );
                else    
                    GetNewSystemName(false);                        // only show new possiblity
            }
            else
                GetNewSystemName();
        }
        else
            bNeedSystemName = false;
    }
    
    // If a new systemName was selected, verify on the server
    // If the server already has a systemName associated with this FPR, cancel the operation.
    if(bNeedSystemName && !g_systemName.IsEmpty()){
        if( doUploadXFPR( false ) != 0){
            g_systemName.Clear();
            saveShopConfig();           // Record the blank systemName
            
            wxString sn = _("System Name:");
            m_staticTextSystemName->SetLabel( sn );
            m_staticTextSystemName->Refresh();

            setStatusText( _("Status: Ready"));
            return;
        }
    }

 
    RefreshSystemName();
    
    setStatusText( _("Status: Ready"));
    g_ipGauge->Stop();
    
    UpdateChartList();
    
    saveShopConfig();
}

int shopPanel::GetShopNameFromFPR()
{
    // Get a fresh fpr file
    bool bDongle = false;
    wxString stringFPR;
    wxString err;
    
    bool b_copyOK = false;
    
    wxString fpr_file = getFPR( false, b_copyOK, bDongle);              // No copy needed
    
    fpr_file = fpr_file.Trim(false);            // Trim leading spaces...
    
    wxFileName fnxpr(fpr_file);
    wxString fprName = fnxpr.GetFullName();

    if(fpr_file.Len()){
        
        //Read the file, convert to ASCII hex, and build a string
        if(::wxFileExists(fpr_file)){
            wxFileInputStream stream(fpr_file);
            while(stream.IsOk() && !stream.Eof() ){
                unsigned char c = stream.GetC();
                if(!stream.Eof()){
                    wxString sc;
                    sc.Printf(_T("%02X"), c);
                    stringFPR += sc;
                }
            }
        }
        else if(fpr_file.IsSameAs(_T("DONGLE_NOT_PRESENT")))
            err = _("  {USB Dongle not found.}");
            
        else
            err = _("  {fpr file not found.}");
    }
    else{
        err = _("  {fpr file not created.}");
    }
    
    if(err.Len()){
        wxString msg = _("ERROR Creating Fingerprint file") + _T("\n");
        msg += _("Check OpenCPN log file.") + _T("\n"); 
        msg += err;
        ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxOK);
        return 1;
    }
        
    wxString url = userURL;
    if(g_admin)
        url = adminURL;
    
    url +=_T("?fc=module&module=occharts&controller=apioesu");

    wxString loginParms;
    loginParms += _T("taskId=identifySystem");
    loginParms += _T("&username=") + g_loginUser;
    loginParms += _T("&key=") + g_loginKey;
    loginParms += _T("&xfpr=") + stringFPR;
    loginParms += _T("&xfprName=") + fprName;
    if(g_debugShop.Len())
        loginParms += _T("&debug=") + g_debugShop;
    loginParms += _T("&version=") + g_systemOS + g_versionString;
    
    int iResponseCode =0;
    TiXmlDocument *doc = 0;
    size_t res = 0;

#ifdef __OCPN_USE_CURL__    
    wxCurlHTTPNoZIP post;
    post.SetOpt(CURLOPT_TIMEOUT, g_timeout_secs);
    res = post.Post( loginParms.ToAscii(), loginParms.Len(), url );
    
    // get the response code of the server
    post.GetInfo(CURLINFO_RESPONSE_CODE, &iResponseCode);
    if(iResponseCode == 200){
        doc = new TiXmlDocument();
        doc->Parse( post.GetResponseBody().c_str());
    }

#else

    qDebug() << url.mb_str();
    qDebug() << loginParms.mb_str();
    
    wxString postresult;
    _OCPN_DLStatus stat = OCPN_postDataHttp( url, loginParms, postresult, 5 );

    qDebug() << "doLogin Post Stat: " << stat;
    
    if(stat != OCPN_DL_FAILED){
        wxCharBuffer buf = postresult.ToUTF8();
        std::string response(buf.data());
        
        qDebug() << response.c_str();
        doc = new TiXmlDocument();
        doc->Parse( response.c_str());
        iResponseCode = 200;
        res = 1;
    }

#endif
    
    if(iResponseCode == 200){
//        const char *rr = doc->Parse( post.GetResponseBody().c_str());
//         wxString p = wxString(post.GetResponseBody().c_str(), wxConvUTF8);
//         wxLogMessage(_T("doLogin results:"));
//         wxLogMessage(p);
        
        wxString queryResult;
        wxString tsystemName;
        
        if( res )
        {
            TiXmlElement * root = doc->RootElement();
            if(!root){
                wxString r = _T("50");
                checkResult(r);                              // undetermined error??
                return false;
            }
            
            wxString rootName = wxString::FromUTF8( root->Value() );
            TiXmlNode *child;
            for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){
                wxString s = wxString::FromUTF8(child->Value());
                
                if(!strcmp(child->Value(), "result")){
                    TiXmlNode *childResult = child->FirstChild();
                    queryResult =  wxString::FromUTF8(childResult->Value());
                }
                else if(!strcmp(child->Value(), "systemName")){
                    TiXmlNode *childResult = child->FirstChild();
                    tsystemName =  wxString::FromUTF8(childResult->Value());
                }
            }
        }
        
        if(queryResult == _T("1")){
            // is the detected systemName disabled?
            if(g_systemNameDisabledArray.Index(tsystemName) != wxNOT_FOUND){
                wxString msg = _("The detected System Name has been disabled\nPlease choose another System Name");
                ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxOK);
                return false;
            }
            else
                g_systemName = tsystemName;
        }
        else{
            checkResult(queryResult, true);
        }

        g_lastQueryResult = queryResult;
        
        long dresult;
        if(queryResult.ToLong(&dresult)){
            return dresult;
        }
        else{
            return 53;
        }
    }
    else
        return checkResponseCode(iResponseCode);
    

}

bool shopPanel::GetNewSystemName( bool bShowAll )
{
        bool sname_ok = false;
        int itry = 0;
        while(!sname_ok && itry < 4){
            bool bcont = doSystemNameWizard( bShowAll );
        
            if( !bcont ){                // user "Cancel"
                g_systemName.Clear();
                break;
            }
            
            if(!g_systemName.Len()){
                wxString msg = _("Invalid System Name");
                ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxOK);
                itry++;
            }
            
            else if(g_systemNameDisabledArray.Index(g_systemName) != wxNOT_FOUND){
                wxString msg = _("This System Name has been disabled\nPlease choose another SystemName");
                ShowOERNCMessageDialog(NULL, msg, _("o-charts_pi Message"), wxOK);
                itry++;
            }
            else{
                sname_ok = true;
            }
                
        }

    return sname_ok;
}


int shopPanel::ComputeUpdates(itemChart *chart, itemSlot *slot)
{
    int installedEdition = slot->GetInstalledEditionInt();
    int serverEdition = chart->GetServerEditionInt();
 
    // Debugging/testing?
    if(g_admin && chart->overrideChartEdition.size()){
        if(chart->overrideChartEdition.find("-0") != std::string::npos){
            chart->taskRequestedFile = _T("base");
            chart->taskRequestedEdition = chart->overrideChartEdition;
            chart->taskCurrentEdition = std::string();
            chart->taskAction = TASK_REPLACE;
        
            return 0;               // no error
        }
        else{
            chart->taskRequestedFile = _T("update");
            chart->taskRequestedEdition = chart->overrideChartEdition;
            chart->taskCurrentEdition = slot->installedEdition;
            chart->taskAction = TASK_UPDATE;
        
            return 0;               // no error
        }
    }
        
    // Is this a reload?
    // If so, just download and install the appropriate "base"
    if(serverEdition == installedEdition){
        chart->taskRequestedFile = _T("base");
        chart->taskRequestedEdition = chart->serverChartEdition;
        chart->taskCurrentEdition = slot->installedEdition;
        chart->taskAction = TASK_REPLACE;
        
        return 0;               // no error
    }
    
    // Is this a base edition update?
    //  If so, we need to simply download the latest new edition available
    if(serverEdition/100 > installedEdition / 100){
        chart->taskRequestedFile = _T("base");
        chart->taskRequestedEdition = chart->serverChartEdition;
        chart->taskCurrentEdition = slot->installedEdition;
        chart->taskAction = TASK_REPLACE;
        
        return 0;               // no error
    }
    else {                                      // must be a minor edition update
    
        chart->taskRequestedFile = _T("update");
        chart->taskRequestedEdition = chart->serverChartEdition;
        chart->taskCurrentEdition = slot->installedEdition;
        chart->taskAction = TASK_UPDATE;
        
        return 0;               // no error
    }

    
    return 1;           // General error
}

std::string GetNormalizedChartsetName( std::string rawName)
{
    //Get the basic chartset names
    if(rawName.find("SENC") != std::string::npos){
        wxFileName fn1(rawName);            // /home/dsr/.opencpn/o_charts_pi/DownloadCache/oeSENC-PL-2020/44-2-base.zip
        wxFileName fn2(fn1.GetPath());
        wxString rv = fn2.GetName();
        int nl = rv.Find('-', true);        // first from END
        if(nl != wxNOT_FOUND)
            rv=rv.Mid(0, nl);
            
        std::string rvs(rv.mb_str());
        return rvs;                         // oeuSENC-PL
    }
    else{
        wxFileName fn1(rawName);            // "/home/xxx/.opencpn/o_charts_pi/DownloadCache/oeRNC-CRBeast-2021/1-0-base.zip"
        wxFileName fn2(fn1.GetPath());
        wxString rv = fn2.GetName();
        int nl = rv.Find('-', true);        // first from END
        if(nl != wxNOT_FOUND)
            rv=rv.Mid(0, nl);
            
        std::string rvs(rv.mb_str());
        return rvs;                         // oeRNC-CRBeast
#if 0
        wxString tlDir = fn1.GetName();     // oeRNC-IMR-GR-2-0-base
        int nl = tlDir.Find( _T("-base"));
        if(nl == wxNOT_FOUND)
            nl = tlDir.Find( _T("-update"));
            
        if(nl != wxNOT_FOUND){
            nl--;
            int ndash_found = 0;
            while(nl){
                wxString ttt = tlDir.Mid(0, nl);
                if(tlDir[nl] == '-'){
                    ndash_found++;
                    if(ndash_found == 2)
                        break;
                }
                nl--;
            }
            return  std::string(tlDir.Mid(0, nl).mb_str());   // like "oeRNC-XXXXX"
        }
        else
            return std::string();
#endif        
    }
}
 

int shopPanel::processTask(itemSlot *slot, itemChart *chart, itemTaskFileInfo *task)
{
    //Get the basic chartset name, and store in the task for later reference
    task->chartsetNameNormalized = GetNormalizedChartsetName( task->cacheLinkLocn);
    
    
#if 0    
    if(chart->taskAction == TASK_REPLACE){
        
        if(!slot->installLocation.size())
            return false;

        // Check the SHA256 of both files in the task
        if(!validateSHA256(task->cacheLinkLocn, task->sha256)){
            wxLogError(_T("oernc_pi: Sha256 error on: ") + task->cacheLinkLocn );
            OCPNMessageBox_PlugIn(NULL, _("Validation error on zip file"), _("oeRNC_pi Message"), wxOK);
            return 6;
        }
        if(!validateSHA256(task->cacheKeysLocn, task->sha256Keys)){
            wxLogError(_T("oernc_pi: Sha256 error on: ") + task->cacheKeysLocn );
            OCPNMessageBox_PlugIn(NULL, _("Validation error on key file"), _("oeRNC_pi Message"), wxOK);
            return 7;
        }

            
        // Remove all .oernc files from the current installation directory
        
        if(chart->taskCurrentEdition.size()){
            // Craft the name of the previous chartset's top level directory
            wxString tlDirFull;
            wxFileName fnInstall(task->cacheLinkLocn);      // /home/dsr/.opencpn/oernc_pi/DownloadCache/oeRNC-IMR-GR-2-0-base.zip
            wxString tlDir = fnInstall.GetName();           // oeRNC-IMR-GR-2-0-base
            
            int nl = tlDir.Find( chart->taskRequestedEdition + _T("-base"));      // "2-0"
            if(nl != wxNOT_FOUND){                  // result is a base set
                wxString tlDirTest = tlDir;
                tlDirTest.Replace(chart->taskRequestedEdition + _T("-base"), chart->taskCurrentEdition +_T("-base"), false);
                tlDirTest.Prepend( wxString(slot->installLocation.c_str()) + wxFileName::GetPathSeparator());
                if(::wxDirExists(tlDirTest)){
                    tlDirFull = tlDirTest;
                }
                else{
                    wxString tlDirTest = tlDir;
                    tlDirTest.Replace(chart->taskRequestedEdition + _T("-base"), chart->taskCurrentEdition +_T("-update"), false);
                    tlDirTest.Prepend( wxString(slot->installLocation.c_str()) + wxFileName::GetPathSeparator());
                    if(::wxDirExists(tlDirTest)){
                        tlDirFull = tlDirTest;
                    }
                }
            }
            else{
                nl = tlDir.Find( chart->taskRequestedEdition + _T("-update"));      // "2-0"
                if(nl != wxNOT_FOUND){                  // result is an update set
                    wxString tlDirTest = tlDir;
                    tlDirTest.Replace(chart->taskRequestedEdition + _T("-update"), chart->taskCurrentEdition +_T("-base"), false);
                    tlDirTest.Prepend( wxString(slot->installLocation.c_str()) + wxFileName::GetPathSeparator());
                    if(::wxDirExists(tlDirTest)){
                        tlDirFull = tlDirTest;
                    }
                }
                else{
                    wxString tlDirTest = tlDir;
                    tlDirTest.Replace(chart->taskRequestedEdition + _T("-update"), chart->taskCurrentEdition +_T("-update"), false);
                    tlDirTest.Prepend( wxString(slot->installLocation.c_str()) + wxFileName::GetPathSeparator());
                    if(::wxDirExists(tlDirTest)){
                        tlDirFull = tlDirTest;
                    }
                }
            }
            
            if(tlDirFull.Length()){
                wxArrayString fileArray;
                size_t nFiles = wxDir::GetAllFiles( tlDirFull, &fileArray, _T("*.oernc"));
                for(unsigned int i=0 ; i < nFiles ;i++)
                    ::wxRemoveFile(fileArray.Item(i));
                
                // Remove the directory itself, if empty.
                wxDir oldDir(tlDirFull);
                if(!oldDir.HasFiles() && !oldDir.HasSubDirs())
                    wxRmdir(tlDirFull);

            }
        }
        
        
        int yyp = 4;
            
        
        // We can unzip the downloaded files directly to their final location
        g_shopPanel->setStatusText( _("Unzipping chart files..."));
        wxYield();
        
        ::wxBeginBusyCursor();
        bool ret = ExtractZipFiles( task->cacheLinkLocn, slot->installLocation, false, wxDateTime::Now(), false);
        ::wxEndBusyCursor();
        
        if(!ret){
            wxLogError(_T("oernc_pi: Unable to extract: ") + task->cacheLinkLocn );
            OCPNMessageBox_PlugIn(NULL, _("Error extracting zip file"), _("oeRNC_pi Message"), wxOK);
            ::wxRemoveFile(wxString(task->cacheLinkLocn.c_str()));
            return 2;
        }
    
    //  keyList.XML should be simply copied to the root
    // directory of the zip set, or, in other words, to the directory containing the .oernc files.
    // Find that directory...
        wxString containerDir;
        
        if(wxFileName::Exists(wxString(task->cacheKeysLocn.c_str()))){
            wxFile file(wxString(task->cacheKeysLocn.c_str()));
            if(file.IsOpened()){
                if(!file.Length()){
                    wxLogError(_T("oernc_pi: Found empty file: ") + task->cacheKeysLocn );
                    OCPNMessageBox_PlugIn(NULL, _("Error empty file "), _("oeRNC_pi Message"), wxOK);
                    ::wxRemoveFile(wxString(task->cacheKeysLocn.c_str()));
                    return 5;
                }
            }
            
            wxFileName fnz(task->cacheLinkLocn);
            containerDir = fnz.GetName();
    
            wxFileName fn(task->cacheKeysLocn);
            if(!::wxCopyFile( task->cacheKeysLocn, slot->installLocation +  wxFileName::GetPathSeparator() + containerDir + wxFileName::GetPathSeparator() + fn.GetFullName())){
                wxLogError(_T("oernc_pi: Unable to copy: ") + task->cacheKeysLocn );
                OCPNMessageBox_PlugIn(NULL, _("Error copying file"), _("oeRNC_pi Message"), wxOK);
                return 3;
            }
        }
        else{
            wxLogError(_T("oernc_pi: Unable to find: ") + task->cacheKeysLocn );
            OCPNMessageBox_PlugIn(NULL, _("Error finding file"), _("oeRNC_pi Message"), wxOK);
            return 4;
        }
            
        
        chart->lastInstalledtlDir = slot->installLocation +  wxFileName::GetPathSeparator() + containerDir;
        
    }
#endif    
    if(1/*chart->taskAction == TASK_UPDATE*/){
        
        if(!slot->installLocation.size())
            return 11;

        // Check the SHA256 of both files in the task
        if(!validateSHA256(task->cacheLinkLocn, task->sha256)){
            wxLogError(_T("o-charts_pi: Sha256 error on: ") + task->cacheLinkLocn );
            ShowOERNCMessageDialog(NULL, _("Validation error on zip file"), _("o-charts_pi Message"), wxOK);
            return 8;
        }
        if(!validateSHA256(task->cacheKeysLocn, task->sha256Keys)){
            wxLogError(_T("o-charts_pi: Sha256 error on: ") + task->cacheKeysLocn );
            ShowOERNCMessageDialog(NULL, _("Validation error on key file"), _("o-charts_pi Message"), wxOK);
            return 9;
        }

        // Extract the key type and name from the downloaded KeyList file
        wxFileName fn2(wxString(task->cacheKeysLocn.c_str()));
        wxString kfn = fn2.GetName();
        int nl = kfn.Find( _T("-base"));
        if(nl == wxNOT_FOUND)
            nl = kfn.Find( _T("-update"));

        wxString keySystem;
        nl++;
        if(nl != wxNOT_FOUND){
            bool dash_found = false;
            while( (size_t)nl < kfn.Length() - 1){
                if(kfn[nl++] == '-'){
                    dash_found = true;
                    break;
                }
            }
            if(dash_found)
                keySystem = kfn.Mid(nl);
        }
  
        if(!keySystem.Length()){
            wxLogError(_T("ChartKey list system name cannot be determined: ") + wxString(task->cacheKeysLocn.c_str()));
            return 16;
        }
        

        wxString pathSep(wxFileName::GetPathSeparator());  std::string ps(pathSep.mb_str());
        
        // If the task type is TASK_REPLACE, we will not get any "WithdrawnCharts" directives
        // So best to simply delete all the charts, and the keylist file from the current installation
        if(chart->taskAction == TASK_REPLACE){
            wxArrayString fileArray;
            wxString currentTLDIR = wxString((slot->installLocation +  ps + task->chartsetNameNormalized).c_str());
            if(wxDir::Exists(currentTLDIR)){
                size_t nFiles = wxDir::GetAllFiles( currentTLDIR, &fileArray);
                for(unsigned int i=0 ; i < nFiles ;i++)
                    ::wxRemoveFile(fileArray.Item(i));
            }
        }

        
        
        // Create chart list container class from currently installed base set   e.g. /home/dsr/Charts/oeRNC_GREECE/oeRNC-IMR/ChartList.XML
        std::string chartListXMLtarget = slot->installLocation +  ps + task->chartsetNameNormalized + ps + "ChartList.XML";
        ChartSetData csdata_target(chartListXMLtarget);

        // Create chart keylist container class from currently installed keylist   e.g. /home/dsr/Charts/oeRNC_GREECE/oeRNC-IMR-GR/oeRNC-IMR-GR-hp64linux.XML
        std::string chartListKeysXMLtarget = slot->installLocation +  ps + task->chartsetNameNormalized + ps + task->chartsetNameNormalized + "-" + std::string(keySystem.mb_str()) + ".XML";
        ChartSetKeys cskey_target(chartListKeysXMLtarget);
        
        // Extract the zip file to a temporary location, making the embedded files available for parsing
#ifdef __OCPN__ANDROID__
        wxString tmp_dir = AndroidGetCacheDir();
        tmp_dir += wxFileName::GetPathSeparator();
        tmp_dir += _T("zipTemp");
        tmp_dir += wxFileName::GetPathSeparator();
        
        // Check and make this (parent) directory if necessary
        wxFileName fnp(tmp_dir);
        if( !fnp.DirExists() ){
            if( !wxFileName::Mkdir(fnp.GetPath(), 0755, wxPATH_MKDIR_FULL) ){
                wxLogError(_T("Can not create tmp parent directory on TASK_UPDATE '") + fnp.GetPath() + _T("'."));
                return 10;
            }
        }
        tmp_dir += wxString( task->chartsetNameNormalized.c_str() );
        tmp_dir += wxFileName::GetPathSeparator();

#else
        wxString tmp_dir = wxFileName::CreateTempFileName( _T("") );                    // Be careful, this method actually create a file
        tmp_dir += _T("zipTemp");
        //tmp_dir += wxFileName::GetPathSeparator();
#endif
        
        wxFileName fn(tmp_dir);
        if( !fn.DirExists() ){
            if( !wxFileName::Mkdir(fn.GetPath(), 0755, wxPATH_MKDIR_FULL) ){
                wxLogError(_T("Can not create tmp directory on TASK_UPDATE '") + fn.GetPath() + _T("'."));
                return 10;
            }
        }
  
        bool zret = ExtractZipFiles( task->cacheLinkLocn, tmp_dir, false, wxDateTime::Now(), false);

        // Find any "ChartList.XML" file
        wxString actionChartList;
        wxArrayString fileArrayXML;
        wxDir::GetAllFiles(tmp_dir, &fileArrayXML, _T("*.XML"));
        
        for(unsigned int i=0 ; i < fileArrayXML.GetCount() ; i++){
            wxString candidate = fileArrayXML.Item(i);
            wxFileName fn(candidate);
            if(fn.GetName().IsSameAs(_T("ChartList"))){
                actionChartList = candidate;
                break;
            }
        }

        // Action vectors
        std::vector < std::string> actionWithdrawn;
        std::vector < itemChartData *> actionAddUpdate;

        // If there is a ChartList.XML, read it, parse it and prepare the indicated operations
        if(actionChartList.Length()){
            
                FILE *iFile = fopen(actionChartList.mb_str(), "rb");
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
                    const char *rr = doc->Parse( iText);
    
                    TiXmlElement * root = doc->RootElement();
                    if(root){
                        //wxString rootName = wxString::FromUTF8( root->Value() );
                        if(!strcmp(root->Value(), "chartList")){    //rootName.IsSameAs(_T("chartList"))){
            
                            TiXmlNode *child;
                            for ( child = root->FirstChild(); child != 0; child = child->NextSibling()){

                                if(!strcmp(child->Value(), "WithdrawnCharts")){
                                    TiXmlNode *childChart = child->FirstChild();
                                    for ( childChart = child->FirstChild(); childChart!= 0; childChart = childChart->NextSibling()){
                                        TiXmlNode *childChartName = childChart->FirstChild();
                                        const char *chartNameKAP =  childChartName->Value();
                                        actionWithdrawn.push_back(std::string(chartNameKAP));
                                    }
                                }
                                else if(!strcmp(child->Value(), "Edition")){            //  <Edition>2021/1-0</Edition>
                                    TiXmlNode *childKey = child->FirstChild();
                                        chart->editionTag = childKey->Value();
                                }
                                    
                                else if(!strcmp(child->Value(), "Chart")){
                                    TiXmlNode *childChart = child->FirstChild();
                                    itemChartData *cdata = new itemChartData;
                                    actionAddUpdate.push_back(cdata);
                                    
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
                }
        }

        ChartSetKeys workCSK;
        
        // If there is a Key list file (there had better be, even if empty), read it, parse it and prepare the indicated operations
        if(task->cacheKeysLocn.size()){
            workCSK.Load( task->cacheKeysLocn ); 
        }
        if( !workCSK.m_bOK){
            wxLogError(_T("New ChartKey list cannot be loaded: ") + wxString(task->cacheKeysLocn.c_str()));
            return 13;
        }

        //Ready to perform the actions indicated
        
        // Process withdrawn charts
        for(unsigned int i = 0 ; i < actionWithdrawn.size() ; i++){
            // Delete the oernc file
            wxFileName fn(actionWithdrawn[i].c_str());
            std::string chartFileToDelete = slot->installLocation +  ps + task->chartsetNameNormalized + ps + std::string(fn.GetName().c_str()) + ".oesu";
            if(::wxFileExists(wxString(chartFileToDelete.c_str()))){
                ::wxRemoveFile(wxString(chartFileToDelete.c_str()));
            }
            
            // Remove the entry from the working copy of ChartList.XML file
            csdata_target.RemoveChart( actionWithdrawn[i] );
 
            // Remove the entry from the working copy of keylist XML file
            cskey_target.RemoveKey( actionWithdrawn[i] );
 
        }
        
        wxDir unzipDir(tmp_dir);
        wxString chartTopLevelZip;
        if(chart->GetChartType() == (int)CHART_TYPE_OERNC){
            if(!unzipDir.GetFirst( &chartTopLevelZip, _T("oeuRNC*"), wxDIR_DIRS)){
                wxLogError(_T("Can not find oeRNC directory in zip file ") + task->cacheLinkLocn);
                return 11;
            }
        }
        else{
            if(!unzipDir.GetFirst( &chartTopLevelZip, _T("oeuSENC*"), wxDIR_DIRS)){
                wxLogError(_T("Can not find oeSENC directory in zip file ") + task->cacheLinkLocn);
                return 11;
            }
        }
 
 
        wxString destinationDir = wxString(slot->installLocation.c_str()) + wxFileName::GetPathSeparator() + task->chartsetNameNormalized + wxFileName::GetPathSeparator();
        wxFileName fndd(destinationDir);
        if( !fndd.DirExists() ){
            if( !wxFileName::Mkdir(fndd.GetPath(), 0755, wxPATH_MKDIR_FULL) ){
                wxLogError(_T("Can not create chart target directory on TASK_UPDATE '") + fndd.GetPath() );
                return 12;
            }
        }
  
        // Process added/modified charts
        for(unsigned int i = 0 ; i < actionAddUpdate.size() ; i++){
            wxString extension = _T(".oesu");
            if(chart->chartType == CHART_TYPE_OERNC)
                extension = _T(".oernc");
            
            wxString fileTarget = wxString( (actionAddUpdate[i]->ID).c_str()) + extension;
            // Copy the oernc chart from the temp unzip location to the target location
            wxString source = tmp_dir + wxFileName::GetPathSeparator() + chartTopLevelZip + wxFileName::GetPathSeparator() + fileTarget;
            if(!wxFileExists(source)){
                wxLogError(_T("Can not find file referenced in ChartList: ") + source);
                continue;
            }
            
            wxString destination = destinationDir + fileTarget;
            g_shopPanel->setStatusText( _("Relocating chart files...") + fileTarget);
            wxYield();
            
            if(wxFileExists(destination)){
                wxRemoveFile(destination);
            }
            
            bool bret;
#ifdef __OCPN__ANDROID__
            bret = AndroidSecureCopyFile( source, destination );
#else
            bret = wxCopyFile( source, destination);
#endif

            if(!bret){
                wxLogError(_T("Can not copy file referenced in ChartList...Source: ") + source + _T("   Destination: ") + destination);
            }
            
            // Revocer the temporary space used.
            wxRemoveFile( source );

            // Add the entry from the working copy of ChartList.XML file
            csdata_target.AddChart( actionAddUpdate[i] );
 
        }
        
        // Process the new Key list file, adding/editing new keys into the target set
        for(size_t i=0 ; i < workCSK.chartList.size() ; i++){
            cskey_target.AddKey(workCSK.chartList[i]);
        }
                
       
        // Arrange to add the tag <Edition> (from the downloaded zip file) to the newly create ChartList.XML file
        csdata_target.setEditionTag(chart->editionTag);
        
        // Write out the modified Target ChartList.XML file as the new result ChartList.XML
        wxString destinationCLXML = destinationDir + _T("ChartList.XML");
        if(! csdata_target.WriteFile( std::string(destinationCLXML.mb_str()) )){
            wxLogError(_T("Can not write target ChartList.XML on TASK_UPDATE '") + destinationCLXML );
            return 14;
        }
        

        // Write out the modified Target KeyList.XML file as the new result KeyList.XML
        wxString destinationKLXML = destinationDir  + wxString(task->chartsetNameNormalized.c_str()) + _T("-") + keySystem + _T(".XML");
        if(!cskey_target.WriteFile( std::string(destinationKLXML.mb_str()) )){
            wxLogError(_T("Can not write target KefList XML file on TASK_UPDATE '") + destinationKLXML );
            return 15;
        }
        
        chart->lastInstalledtlDir = destinationDir;

        // Find and store any EULAs found
        wxArrayString fileArrayEULA;
        wxDir::GetAllFiles(tmp_dir, &fileArrayEULA, _T("*.html"));
        for(unsigned int i=0 ; i < fileArrayEULA.GetCount() ; i++){
            wxFileName fn(fileArrayEULA.Item(i));
            wxString destination = destinationDir + fn.GetFullName();
            wxString source = fileArrayEULA.Item(i);

            bool bret;
#ifdef __OCPN__ANDROID__
            bret = AndroidSecureCopyFile( source, destination );
#else
            bret = wxCopyFile( source, destination);
#endif
            
            if(!bret)
                wxLogError(_T("Can not copy EULA file...Source: ") + source + _T("   Destination: ") + destination);
            else{
                g_lastEULAFile = destination;
               // Recover the temporary space used.
                wxRemoveFile( source );
            }
        }
        
        // Delete the temporary Chartlist file
        if(wxFileExists(actionChartList)){
            wxRemoveFile(actionChartList);
        }
    }    
        

    return 0;
}

bool shopPanel::validateSHA256(std::string fileName, std::string shaSum)
{
    //File...
    std::string sfile = fileName;
    
    // Does the file exist?
    if(!wxFileName::Exists(wxString(sfile.c_str())))
        return false;
    
    // Has the file any content?
    wxFile file(wxString(sfile.c_str()));
    if(!file.IsOpened())
        return false;
                
    if(!file.Length())
        return false;
    
    // Calculate the SHA256 Digest

    // Check for the file presence, and openability
    FILE *rFile = fopen(sfile.c_str(), "rb");
   
    if (rFile < (void *)0)
        return false;            // file error

    wxString previousStatus = getStatusText();
    setStatusText( _("Status: Validating download file..."));
    wxYield();

    // compute the file length    
    fseek(rFile, 0, SEEK_END);
    unsigned int rLength = ftell(rFile);
                        
    unsigned char buffer[1024 * 64];
    fseek(rFile, 0, SEEK_SET);
    
    SHA256_CTX ctx;
    unsigned char shasum[32];
    sha256_init(&ctx);
 
    size_t nread = 0;
    int ic = 0;
    while (nread < rLength){
        memset(buffer, 0, sizeof(buffer));
        int iread = fread(buffer, 1, sizeof(buffer), rFile);
        sha256_update(&ctx, buffer, iread);
        nread += iread;
        
        if((!(ic % 16)) && g_ipGauge){
            g_ipGauge->Pulse();
            wxYieldIfNeeded();
        }
        ic++;
    }

    fclose(rFile);
    
    sha256_final(&ctx, shasum);
    
    std::string ssum;
    for(int i=0 ; i < 32 ; i++){
        char t[3];
        sprintf(t, "%02x", shasum[i]);
        ssum += t;
    }
    
    int cval = ssum.compare(shaSum);
    
    setStatusText(previousStatus);
    wxYield();
    
    return (cval == 0);
    
}

void shopPanel::ValidateChartset( wxCommandEvent& event )
{

   
    if(m_ChartPanelSelected){
        m_shopLog->ClearLog();
        if(g_pi)
            g_pi->m_pOptionsPage->Scroll(0, GetSize().y / 2);
        
        if(m_validator)
            delete m_validator;
        
        m_buttonValidate->Disable();
        GetSizer()->Layout();
        wxYield();
        
        if(!g_shopLogFrame){
            wxSize shopSize = GetSize();
            wxSize valSize = wxSize( (shopSize.x * 9 / 10), (shopSize.y * 8 / 10));
            g_shopLogFrame = new oesu_piScreenLogContainer( this, _T("Validate Log"), valSize);
            g_shopLogFrame->Center();
        }

        g_shopLogFrame->ClearLog();
        m_validator = new ocValidator( m_ChartPanelSelected->GetSelectedChart(), g_shopLogFrame);
        m_validator->startValidation();
        
        m_buttonValidate->Enable();
        GetSizer()->Layout();
        wxYield();
        
    }
    else{
        ShowOERNCMessageDialog(NULL, _("No chartset selected."), _("o-charts_pi Message"), wxOK);
    }


}
    

void shopPanel::OnButtonInstallChain( wxCommandEvent& event )
{
    // Chained through from download end event

    if(m_bAbortingDownload){
        m_bAbortingDownload = false;
        ShowOERNCMessageDialog(NULL, _("Chart download cancelled."), _("o-charts_pi Message"), wxOK);
        UpdateActionControls();
        return;
    }

    //  Is the queue done?

    if(gtargetSlot->idlQueue < gtargetSlot->dlQueue.size()){
        
        // is the required file available in the cache?
        if(wxFileExists(gtargetSlot->dlQueue[gtargetSlot->idlQueue].localFile)){
            // Validate the existing file using SHA256
            if( validateSHA256(gtargetSlot->dlQueue[gtargetSlot->idlQueue].localFile, gtargetSlot->dlQueue[gtargetSlot->idlQueue].SHA256)){
                //  OK, Skip to next in queue
                gtargetSlot->idlQueue++;        // next
                
                //Send an event to continue the download chain
                wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED);
                event.SetId( ID_CMD_BUTTON_INSTALL_CHAIN );
                g_shopPanel->GetEventHandler()->AddPendingEvent(event);
                return;
            }
        }

        // Prepare to start the next download
        //Make the temp download directory if needed
        wxFileName fn(gtargetSlot->dlQueue[gtargetSlot->idlQueue].localFile);
        wxString dir = fn.GetPath();
        if( !wxFileName::DirExists(fn.GetPath()) ){
            if( !wxFileName::Mkdir(fn.GetPath(), 0755, wxPATH_MKDIR_FULL) ){
                wxLogError(_T("Can not create directory '") + fn.GetPath() + _T("'."));
                return;
            }
        }

#ifdef __OCPN_USE_CURL__        
        g_curlDownloadThread = new wxCurlDownloadThread(g_CurlEventHandler);
        downloadOutStream = new wxFFileOutputStream(gtargetSlot->dlQueue[gtargetSlot->idlQueue].localFile);
        g_curlDownloadThread->SetURL(gtargetSlot->dlQueue[gtargetSlot->idlQueue].url);
        g_curlDownloadThread->SetOutputStream(downloadOutStream);
        //wxLogMessage(_T("Downloading: ") + gtargetSlot->dlQueue[gtargetSlot->idlQueue].url);
        g_curlDownloadThread->Download();
#else
        if(!m_bconnected){
            Connect(wxEVT_DOWNLOAD_EVENT, (wxObjectEventFunction)(wxEventFunction)&shopPanel::onDLEvent);
            m_bconnected = true;
        }
 
        g_downloadChainIdentifier = ID_CMD_BUTTON_INSTALL_CHAIN;
        
        OCPN_downloadFileBackground( gtargetSlot->dlQueue[gtargetSlot->idlQueue].url,
                                     gtargetSlot->dlQueue[gtargetSlot->idlQueue].localFile,
                                     this, &g_FileDownloadHandle);
        
#endif
        gtargetSlot->idlQueue++;        // next
        m_buttonCancelOp->Show();
        GetSizer()->Layout();

        return;
    }


    // Chained through from download end event
    if(m_binstallChain){
        m_binstallChain = false;
        
        int rv = 0;
        
        //  Download is apparently done.
        g_statusOverride.Clear();

        // Parse the task definitions, decide what to do
    
        //Get the basic chartset name from the first task in the tasklist
        wxString ChartsetNormalName;
        if(gtargetSlot->taskFileList.size()){
            itemTaskFileInfo *pTask = gtargetSlot->taskFileList[0];
            if(pTask)
                ChartsetNormalName = GetNormalizedChartsetName( pTask->cacheLinkLocn);
        }

        // the simple case of initial load, or full base update
        if( (gtargetChart->taskAction == TASK_REPLACE) || (gtargetChart->taskAction == TASK_UPDATE) ){
            
            // Is there a known install directory?
            wxString installDir = gtargetSlot->installLocation;
            
            if(installDir.Length()){
                // Is there are ChartList.XML file in the config-stored install location?
                // If not, we assume that user moved the charts manually, so we ask for a new install directory
                wxString fTest = installDir;
                if(!fTest.EndsWith(wxFileName::GetPathSeparator()))
                    fTest += wxFileName::GetPathSeparator();
                if(ChartsetNormalName.Length())
                    fTest += ChartsetNormalName + wxFileName::GetPathSeparator();
                fTest +=_T("ChartList.XML");
                
                if(!wxFileExists(fTest))
                    installDir.Clear();
            }
            
            // Update, or initial load?
            if(!gtargetChart->taskCurrentEdition.Length() || !installDir.Length()){             // initial load
        
                wxString installLocn = g_PrivateDataDir;
                if(installDir.Length())
                    installLocn = installDir;
                else if(g_lastInstallDir.Length())
                    installLocn = g_lastInstallDir;
        
                //Need better default here, lik c:\Charts, if it exists.
                //wxDirDialog dirSelector( NULL, _("Choose chart install location."), installLocn, wxDD_DEFAULT_STYLE  );
                //int result = dirSelector.ShowModal();
        
                wxString dir_spec;
                int result = PlatformDirSelectorDialog( NULL, &dir_spec, _("Choose chart install location."), installLocn);
               
                if(result == wxID_OK)
                    gtargetSlot->installLocation = dir_spec.mb_str(); //dirSelector.GetPath().mb_str();
                else{
                    g_statusOverride.Clear();
                    setStatusText( _("Status: Ready"));
                    UpdateChartList();
                    UpdateActionControls();
                    return;
                }                    
            }
            
            //Presumably there is an install directory, and a current Edition, so this is an update
                
            // Process the array of itemTaskFileInfo
            for(unsigned int i=0 ; i < gtargetSlot->taskFileList.size() ; i++){
                itemTaskFileInfo *pTask = gtargetSlot->taskFileList[i];
                int rv = 0;
                rv = processTask(gtargetSlot, gtargetChart, pTask);
                if(rv){

                    g_statusOverride.Clear();
                    setStatusText( _("Status: Ready"));
                    ShowOERNCMessageDialog(NULL, _("Chart installation ERROR."), _("o-charts_pi Message"), wxOK);
                    UpdateChartList();
                    UpdateActionControls();
                    return;
                }
            }

            // If no error, finalize the contract
            gtargetSlot->installedEdition = gtargetChart->taskRequestedEdition;
        }
        

        //  We know that the unzip process puts all charts in a subdir whose name is the "downloadFile", without extension
        //  This is the dir that we want to add to database.
        wxString targetAddDir = gtargetChart->lastInstalledtlDir;
    
        // Remove trailing "/"
        if(targetAddDir.EndsWith(wxFileName::GetPathSeparator()))
            targetAddDir = targetAddDir.Truncate(targetAddDir.Length() - 1);
        
        //  If the currect core chart directories do not cover this new directory, then add it
        bool covered = false;
        for( size_t i = 0; i < GetChartDBDirArrayString().GetCount(); i++ ){
            if( targetAddDir.StartsWith((GetChartDBDirArrayString().Item(i))) ){
                covered = true;
                break;
            }
        }
        if( !covered ){
            AddChartDirectory( targetAddDir );
        }

        g_lastInstallDir = gtargetSlot->installLocation;
    
        // Clear out the global key hashes...
        keyMapDongle.clear();
        keyMapSystem.clear();

        ForceChartDBUpdate();
    
        saveShopConfig();
    
   
        g_statusOverride.Clear();
        setStatusText( _("Status: Ready"));
                
        ShowOERNCMessageDialog(NULL, _("Chart installation complete."), _("o-charts_pi Message"), wxOK);

        // Show any EULA here
        wxArrayString fileArrayEULA;
        wxDir::GetAllFiles(targetAddDir, &fileArrayEULA, _T("*.html"));
        for(unsigned int i=0 ; i < fileArrayEULA.GetCount() ; i++){
//             oeRNC_pi_about *pab = new oeRNC_pi_about( GetOCPNCanvasWindow(), fileArrayEULA.Item(i) );
//             pab->SetOKMode();
//             pab->ShowModal();
//             pab->Destroy();
        }
 
        
        
        UpdateChartList();

        UpdateActionControls();

        return;
    }
}

void shopPanel::OnButtonInstall( wxCommandEvent& event )
{
    itemChart *chart = m_ChartPanelSelected->GetSelectedChart();
    if(!chart)
        return;

    
    g_LastErrorMessage.Clear();
    SetErrorMessage();
    
    g_statusOverride = _T("Installing...");
    setStatusText( _("Preparing installation..."));

    wxYield();
    
    // Check the dongle
    g_dongleName.Clear();
    if(IsDongleAvailable()){
        g_dongleSN = GetDongleSN();
        char sName[20];
        snprintf(sName, 19, "sgl%08X", g_dongleSN);

        g_dongleName = wxString(sName);
    }

    m_buttonInstall->Disable();
    m_buttonValidate->Hide();
    m_buttonCancelOp->Hide();
    GetSizer()->Layout();
    
    wxYield();

    // Create and upload an XFPR and selected systemName to the server.
    // If the systemName is already known, and the XFPR matches no harm done.
    // Or, if the systemName is new, it will be registered with the shop.
    // Otherwise, an error is shown, and the operation cancels.
    
    // Prefer to use the dongle, if present
    if(g_dongleName.Len()){
        if( doUploadXFPR( true ) != 0){
            g_dongleName.Clear();
            g_statusOverride.Clear();
            setStatusText( _("Status: Dongle FPR upload error"));

            UpdateActionControls();
            return;
        }
    }
    else{
        if(!g_systemName.Length()){
            if(GetNewSystemName())
                RefreshSystemName();
        }
        
        if(!g_systemName.Length()){
                saveShopConfig();       // record blank system name.
                UpdateActionControls();
                RefreshSystemName();
                return;
        }

        if( doUploadXFPR( false ) != 0){
            g_systemName.Clear();
            g_statusOverride.Clear();
            setStatusText( _("Status: System FPR upload error"));
            saveShopConfig();       // record blank system name.
            RefreshSystemName();
                
            UpdateActionControls();

            return;
        }
    }

    int qtyIndex = -1;
    
    //  Check if I am already assigned to this chart
    bool bNeedAssign = false;
    if(g_dongleName.Len()){
        if(!chart->isChartsetAssignedToSystemKey(g_dongleName))
            bNeedAssign = true;
    }
    else{
        if(!chart->isChartsetAssignedToSystemKey(g_systemName))
            bNeedAssign = true;
    }
        
        
    if(bNeedAssign){
        
        // Need assignment
        // Choose the first available qty that has an available slot
        for(unsigned int i=0 ; i < chart->quantityList.size() ; i++){
            itemQuantity Qty = chart->quantityList[i];
            if(Qty.slotList.size() < chart->maxSlots){
                qtyIndex = i;
                break;
            }
        }
        
        if(qtyIndex < 0){
            wxLogMessage(_T("oeRNC Error: No available slot found for unassigned chart."));
            UpdateActionControls();
            return;
        }
            
        // Ready to assign.
        //Try to assign to dongle first..... 
        int assignResult;

        if(g_dongleName.Len())
            assignResult = doAssign(chart, qtyIndex, g_dongleName);
        else
            assignResult = doAssign(chart, qtyIndex, g_systemName);
        
        if(assignResult != 0){
            wxLogMessage(_T("oeRNC Error: Slot doAssign()."));
            g_statusOverride.Clear();
            UpdateActionControls();
            return;
        }
        
        // Find the active slot parameters
        chart->m_activeQtyID = chart->quantityList[qtyIndex].quantityId;
        // By definition, the new active slot index is the last one added
        chart->m_assignedSlotIndex = chart->quantityList[qtyIndex].slotList.size()-1;

    }
    
    // Known assigned to me, so maybe Ready for download
    
    // Get the slot target
    itemSlot *activeSlot = chart->GetActiveSlot();
    if(!activeSlot){
        wxLogMessage(_T("oeRNC Error: active slot not defined."));
        UpdateActionControls();
        return;
    }

    // Slot is know, now determine what edition to request
    ComputeUpdates(chart, activeSlot);

    bool bNeedRequestWait = true;
    
    int request_return;
    if(bNeedRequestWait){

        ::wxBeginBusyCursor();
        request_return = doPrepareGUI(activeSlot);
        ::wxEndBusyCursor();

        if(request_return != 0){
            if(g_ipGauge)
                g_ipGauge->Stop();

            m_buttonCancelOp->Hide();
            g_statusOverride.Clear();
            
            SetErrorMessage();
            UpdateChartList();
            UpdateActionControls();

            
            return;
        }
    }

    doDownloadGui(chart, activeSlot);
    
            

    return;
}


void shopPanel::OnButtonDownload( wxCommandEvent& event )
{
#if 0    
    
    if(!m_ChartSelected)                // No chart selected
        return;
   
    oitemChart *chart = m_ChartSelected->m_pChart;
    m_ChartSelectedID = chart->chartID;           // save a copy of the selected chart
    m_ChartSelectedOrder = chart->orderRef;
    m_ChartSelectedQty = chart->quantityId;
    
    // What slot?
    m_activeSlot = -1;
    if(chart->sysID0.IsSameAs(g_systemName))
        m_activeSlot = 0;
    else if(chart->sysID1.IsSameAs(g_systemName))
        m_activeSlot = 1;
    
    if(m_activeSlot < 0)
        return;
    
    // Is the selected chart ready for download?
    // If not, we do the "Request/Prepare" step
    bool bNeedRequestWait = false;    
    if(m_activeSlot == 0){
        if(!chart->statusID0.IsSameAs(_T("download")))
            bNeedRequestWait = true;
    }
    else if(m_activeSlot == 1){
        if(!chart->statusID1.IsSameAs(_T("download")))
            bNeedRequestWait = true;
    }

    if(bNeedRequestWait)
        int retval = doPrepareGUI();
    else
        doDownloadGui();
#endif    
}

int shopPanel::doPrepareGUI(itemSlot *targetSlot)
{
    m_buttonCancelOp->Hide();
    GetSizer()->Layout();
    wxYield();

    setStatusText( _("Requesting License Keys..."));
    
    m_prepareTimerCount = 8;            // First status query happens in 2 seconds
    m_prepareProgress = 0;
    m_prepareTimeout = 60;
    
//    m_prepareTimer.SetOwner( this, 4357 );

    wxYield();
    
    int err_code = doPrepare(m_ChartPanelSelected, targetSlot);
    if(err_code != 0){                  // Some error
//             wxString ec;
//             ec.Printf(_T(" { %d }"), err_code);     
//             setStatusText( _("Status: Communications error.") + ec);
            if(g_ipGauge)
                g_ipGauge->Stop();

            m_prepareTimer.Stop();
            g_statusOverride.Clear();

            SetErrorMessage();
            UpdateActionControls();

            return err_code;
    }
     return 0;
}

int shopPanel::doDownloadGui(itemChart *targetChart, itemSlot* targetSlot)
{
    setStatusText( _("Status: Downloading..."));
    //m_staticTextStatusProgress->Show();
    m_buttonCancelOp->Hide();
    GetButtonUpdate()->Disable();
    
    g_statusOverride = _("Downloading...");
    UpdateChartList();
    m_buttonValidate->Hide();
    m_buttonCancelOp->Hide();

    wxYield();
    
    m_binstallChain = true;
    m_bAbortingDownload = false;
    
    int err_code = doDownload(targetChart, targetSlot);
    return 0;
    
}

void shopPanel::OnButtonCancelOp( wxCommandEvent& event )
{
    if(m_prepareTimer.IsRunning()){
        m_prepareTimer.Stop();
        g_ipGauge->Stop();
    }

#ifdef __OCPN_USE_CURL__    
    if(g_curlDownloadThread){
        m_bAbortingDownload = true;
        g_curlDownloadThread->Abort();
        g_ipGauge->Stop();
        setStatusTextProgress(_T(""));
        m_binstallChain = true;
    }
#else
    OCPN_cancelDownloadFileBackground(g_FileDownloadHandle);
    m_bAbortingDownload = true;
    g_ipGauge->Stop();
    setStatusTextProgress(_T(""));
    m_binstallChain = true;
#endif    
    
    setStatusText( _("Status: OK"));
    m_buttonCancelOp->Hide();
    
    g_statusOverride.Clear();
    m_buttonInstall->Enable();
    m_buttonUpdate->Enable();
    GetSizer()->Layout();

    SetErrorMessage();

    UpdateChartList();
    
}

void shopPanel::OnPrepareTimer(wxTimerEvent &evt)
{
}    

bool compareName(itemChart * i1, itemChart * i2) 
{ 
    return (i1->chartName < i2->chartName); 
} 
void shopPanel::SortChartList()
{
    std::vector<itemChart *> VectorActive;
    std::vector<itemChart *> VectorInActive;
    std::vector<itemChart *> RasterActive;
    std::vector<itemChart *> RasterInActive;
    
    // Divide the entire list into 4 categories
    
    for(unsigned int i=0 ; i < ChartVector.size() ; i++){
        if(ChartVector[i]->GetChartType() == CHART_TYPE_OEUSENC){
            if( ChartVector[i]->isChartsetExpired() || ( ChartVector[i]->isChartsetFullyAssigned()  &&  !ChartVector[i]->isChartsetAssignedToMe()))
                VectorInActive.push_back( ChartVector[i] );
            else
                VectorActive.push_back( ChartVector[i] );
        }
        else {
            if( ChartVector[i]->isChartsetExpired() || ( ChartVector[i]->isChartsetFullyAssigned()  &&  !ChartVector[i]->isChartsetAssignedToMe()))
                RasterInActive.push_back( ChartVector[i] );
            else
                RasterActive.push_back( ChartVector[i] );
        }
    }
    
    // Sort the 4 lists alphabetically on chart name
    std::sort(VectorActive.begin(), VectorActive.end(), compareName); 
    std::sort(RasterActive.begin(), RasterActive.end(), compareName); 
    std::sort(VectorInActive.begin(), VectorInActive.end(), compareName); 
    std::sort(RasterInActive.begin(), RasterInActive.end(), compareName); 
    
    // And then recreate the master list
    ChartVector.clear();
    for(unsigned int i=0 ; i < VectorActive.size() ; i++){ ChartVector.push_back(VectorActive[i]); }
    for(unsigned int i=0 ; i < RasterActive.size() ; i++){ ChartVector.push_back(RasterActive[i]); }
    for(unsigned int i=0 ; i < VectorInActive.size() ; i++){ ChartVector.push_back(VectorInActive[i]); }
    for(unsigned int i=0 ; i < RasterInActive.size() ; i++){ ChartVector.push_back(RasterInActive[i]); }
    
}

void shopPanel::UpdateChartList( )
{
    if(g_ipGauge)
        g_ipGauge->Stop();

    // Capture the state of any selected chart
     if(m_ChartPanelSelected){
         itemChart *chart = m_ChartPanelSelected->GetSelectedChart();
         if(chart){
             m_ChartSelectedID = chart->chartID;           // save a copy of the selected chart
             m_ChartSelectedOrder = chart->orderRef;
         }
     }
    
    m_scrollWinChartList->ClearBackground();
    
    // Clear any existing panels
    for(unsigned int i = 0 ; i < panelVector.size() ; i++){
        delete panelVector[i];
    }
    panelVector.clear();
    m_ChartPanelSelected = NULL;

    
    // Add new panels
    for(unsigned int i=0 ; i < ChartVector.size() ; i++){
        if( ChartVector[i]->isChartsetShow() ){
            ChartVector[i]->GetChartThumbnail(100, true );              // attempt download if necessary
            oeXChartPanel *chartPanel = new oeXChartPanel( m_scrollWinChartList, wxID_ANY, wxDefaultPosition, wxSize(-1, -1), ChartVector[i], this);
            chartPanel->SetSelected(false);
        
            boxSizerCharts->Add( chartPanel, 0, wxEXPAND|wxALL, 0 );
//            boxSizerCharts->Layout();
            panelVector.push_back( chartPanel );
        } 
    }
    
    SelectChartByID(m_ChartSelectedID, m_ChartSelectedOrder);
    
    m_scrollWinChartList->ClearBackground();
    m_scrollWinChartList->GetSizer()->Layout();

    Layout();

    m_scrollWinChartList->ClearBackground();
    
    UpdateActionControls();
    
    saveShopConfig();
    
    Refresh( true );
}


void shopPanel::UpdateActionControls()
{
    //  Turn off all buttons.
    m_buttonInstall->Hide();
    m_buttonValidate->Hide();
    m_buttonCancelOp->Hide();

    
    if(!m_ChartPanelSelected){                // No chart selected
        m_buttonInstall->Enable();
        return;
    }
    
    if(!g_statusOverride.Length()){
        m_buttonInstall->Enable();
    }
    
    m_buttonValidate->Show();
    m_buttonValidate->Enable();
    
    wxString suffix = g_systemName;
    if(g_dongleName.Length())
        suffix = g_dongleName  + _T(" (") + _("USB Key Dongle") + _T(")");
    
    itemChart *chart = m_ChartPanelSelected->GetSelectedChart();

    wxString labelDownload = _("Download Selected Chart");
    wxString labelInstall = _("Install Selected Chart for ") + suffix;
    wxString labelReinstall = _("Reinstall Selected Chart for ") + suffix;
    wxString labelUpdate = _("Update Selected Chart for ") + suffix;
    
#ifdef __OCPN__ANDROID__
    labelDownload = _("Download Selection");
    labelInstall = _("Install Selection");
    labelReinstall = _("Reinstall Selection");
    labelUpdate = _("Update Selection");
#endif
    
    if(chart->getChartStatus() == STAT_REQUESTABLE){
        m_buttonInstall->SetLabel(labelDownload);
        m_buttonInstall->Show();
    }
    else if(chart->getChartStatus() == STAT_PURCHASED){
        m_buttonInstall->SetLabel(labelInstall);
        m_buttonInstall->Show();
    }
    else if(chart->getChartStatus() == STAT_CURRENT){
        m_buttonInstall->SetLabel(labelReinstall);
        m_buttonInstall->Show();
    }
    else if(chart->getChartStatus() == STAT_STALE){
        m_buttonInstall->SetLabel(labelUpdate);
        m_buttonInstall->Show();
    }
    //gridSizerActionButtons->Layout();
    
#if 0    
    else if(chart->getChartStatus() == STAT_READY_DOWNLOAD){
        m_buttonInstall->SetLabel(_("Download Selected Chart"));
        m_buttonInstall->Show();       
    }
    else if(chart->getChartStatus() == STAT_REQUESTABLE){
        m_buttonInstall->SetLabel(_("Download Selected Chart"));
        m_buttonInstall->Show();
    }
    else if(chart->getChartStatus() == STAT_PREPARING){
        m_buttonInstall->Hide();
    }
#endif    
    GetSizer()->Layout();

}

    
bool shopPanel::doSystemNameWizard( bool bShowAll )
{
    // Make sure the system name array is current
    
    if( g_systemName.Len() && (g_systemNameChoiceArray.Index(g_systemName) == wxNOT_FOUND))
        g_systemNameChoiceArray.Insert(g_systemName, 0);
    
    oeUniSystemNameSelector dlg( GetOCPNCanvasWindow(), bShowAll);
    
    wxSize dialogSize(500, -1);
    
    #ifdef __OCPN__ANDROID__
    wxSize ss = ::wxGetDisplaySize();
    dialogSize.x = ss.x * 8 / 10;
    #endif         
    dlg.SetSize(dialogSize);
    dlg.Centre();
    
    
    #ifdef __OCPN__ANDROID__
//    androidHideBusyIcon();
    #endif             
    int ret = dlg.ShowModal();
 
    if(dlg.GetReturnCode() == 0){               // OK
        wxString sName = dlg.getRBSelection();
        if(g_systemNameChoiceArray.Index(sName) == wxNOT_FOUND){
            // Is it the dongle selected?
            if(sName.Find(_T("Dongle")) != wxNOT_FOUND){
                wxString ssName = sName.Mid(0, 11);
                g_systemNameChoiceArray.Insert(ssName, 0);
                sName = ssName;
            }
            else{    
                sName = doGetNewSystemName();
                if(sName.Len())
                    g_systemNameChoiceArray.Insert(sName, 0);
                else
                    return false;
            }
        }
        if(sName.Len())
            g_systemName = sName;
    }
    else 
        return false;

    RefreshSystemName();
    
    saveShopConfig();
    
    return true;
}

wxString shopPanel::doGetNewSystemName( )
{
    oeUniGETSystemName dlg( GetOCPNCanvasWindow());
    
    wxSize dialogSize(500, -1);
    
    #ifdef __OCPN__ANDROID__
    wxSize ss = ::wxGetDisplaySize();
    dialogSize.x = ss.x * 8 / 10;
    #endif         
    dlg.SetSize(dialogSize);
    dlg.Centre();
    
    
    int ret = dlg.ShowModal();
    
    wxString sName;
    if(ret == 0){               // OK
        sName = dlg.GetNewName();
        
        // Check system name rules...
        const char *s = sName.c_str();
        if( (strlen(s) < 3) || (strlen(s) > 15))
            return wxEmptyString;
        
        char *t = (char *)s;
        for(unsigned int i = 0; i < strlen(s); i++, t++){
            bool bok = false;
            if( ((*t >= 'a') && (*t <= 'z')) ||
                ((*t >= 'A') && (*t <= 'Z')) ||
                ((*t >= '0') && (*t <= '9')) ){
                
                bok = true;
            }
            else{
                sName.Clear();
                break;
            }
        }
    }
    
    
    return sName;
}

void shopPanel::OnGetNewSystemName( wxCommandEvent& event )
{
    doGetNewSystemName();
}

// void shopPanel::OnChangeSystemName( wxCommandEvent& event )
// {
//     doSystemNameWizard();
// }


#ifdef __OCPN__ANDROID__

void shopPanel::onDLEvent(OCPN_downloadEvent &evt)
{
    qDebug() << "onDLEvent";
    
    wxDateTime now = wxDateTime::Now();
    
    switch(evt.getDLEventCondition()){
        case OCPN_DL_EVENT_TYPE_END:
        {
            m_bTransferComplete = true;
            m_bTransferSuccess = (evt.getDLEventStatus() == OCPN_DL_NO_ERROR) ? true : false;
            
            g_ipGauge->SetValue(100);
            setStatusTextProgress(_T(""));
            setStatusText( _("Status: OK"));
            m_buttonCancelOp->Hide();
            GetButtonUpdate()->Enable();

/*            
            if(!g_shopPanel->m_bAbortingDownload){
                if(g_curlDownloadThread){
                    g_curlDownloadThread->Wait();
                    delete g_curlDownloadThread;
                    g_curlDownloadThread = NULL;
                }
            }
            
            if(g_shopPanel->m_bAbortingDownload){
                if(g_shopPanel->GetSelectedChart()){
                    itemChart *chart = g_shopPanel->GetSelectedChart()->m_pChart;
                    if(chart){
                        chart->downloadingFile.Clear();
                    }
                }
            }
*/            
            //  Send an event to chain back to "Install" button
            wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED);
            if(g_downloadChainIdentifier){
                event.SetId( g_downloadChainIdentifier );
                GetEventHandler()->AddPendingEvent(event);
            }
            
            break;
        }   
        case OCPN_DL_EVENT_TYPE_PROGRESS:

            dl_now = evt.getTransferred();
            dl_total = evt.getTotal();
            
            // Calculate the gauge value
            if(dl_total > 0){
                float progress = dl_now/dl_total;
                
                g_ipGauge->SetValue(progress * 100);
                g_ipGauge->Refresh();
                
            }
            
            if(now.GetTicks() != g_progressTicks){
                
                //  Set text status
                wxString tProg;
                tProg = _("Downloaded:  ");
                wxString msg;
                msg.Printf( _T("(%6.1f MiB / %4.0f MiB)    "), (float)(evt.getTransferred() / 1e6), (float)(evt.getTotal() / 1e6));
                tProg += msg;
                
                setStatusTextProgress( tProg );
                
                g_progressTicks = now.GetTicks();
            }
            
            break;
            
        case OCPN_DL_EVENT_TYPE_START:
        case OCPN_DL_EVENT_TYPE_UNKNOWN:    
        default:
            break;
    }
}


#endif




IMPLEMENT_DYNAMIC_CLASS( oeUniGETSystemName, wxDialog )
BEGIN_EVENT_TABLE( oeUniGETSystemName, wxDialog )
   EVT_BUTTON( ID_GETIP_CANCEL, oeUniGETSystemName::OnCancelClick )
   EVT_BUTTON( ID_GETIP_OK, oeUniGETSystemName::OnOkClick )
END_EVENT_TABLE()
 
 
 oeUniGETSystemName::oeUniGETSystemName()
 {
 }
 
 oeUniGETSystemName::oeUniGETSystemName( wxWindow* parent, wxWindowID id, const wxString& caption,
                                             const wxPoint& pos, const wxSize& size, long style )
 {
     
     long wstyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER;
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
     SetFont( *qFont );
     
     CreateControls();
     GetSizer()->SetSizeHints( this );
     Centre();
     
 }
 
 oeUniGETSystemName::~oeUniGETSystemName()
 {
     delete m_SystemNameCtl;
 }
 
 /*!
  * oeRNCGETSystemName creator
  */
 
 bool oeUniGETSystemName::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                                    const wxPoint& pos, const wxSize& size, long style )
 {
     SetExtraStyle( GetExtraStyle() | wxWS_EX_BLOCK_EVENTS );
     
     long wstyle = style;
     #ifdef __WXMAC__
     wstyle |= wxSTAY_ON_TOP;
     #endif
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
     SetFont( *qFont );
     
     SetTitle( _("New OpenCPN oeRNC System Name"));
     
     CreateControls(  );
     Centre();
     return TRUE;
 }
 
 
 void oeUniGETSystemName::CreateControls(  )
 {
     int ref_len = GetCharHeight();
     
     oeUniGETSystemName* itemDialog1 = this;
     
     wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
     itemDialog1->SetSizer( itemBoxSizer2 );
     
     wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox( itemDialog1, wxID_ANY, _("Enter New System Name") );
     
     wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer( itemStaticBoxSizer4Static, wxVERTICAL );
     itemBoxSizer2->Add( itemStaticBoxSizer4, 0, wxEXPAND | wxALL, 5 );
     
     wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _T(""), wxDefaultPosition, wxDefaultSize, 0 );
     itemStaticBoxSizer4->Add( itemStaticText5, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 5 );
     
     m_SystemNameCtl = new wxTextCtrl( itemDialog1, ID_GETIP_IP, _T(""), wxDefaultPosition, wxSize( ref_len * 10, -1 ), 0 );
     itemStaticBoxSizer4->Add( m_SystemNameCtl, 0,  wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM , 5 );
     
     wxStaticText *itemStaticTextLegend = new wxStaticText( itemDialog1, wxID_STATIC,  _("A valid System Name is 3 to 15 characters in length."));
     itemBoxSizer2->Add( itemStaticTextLegend, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP, 5 );

//      wxStaticText *itemStaticTextLegend1 = new wxStaticText( itemDialog1, wxID_STATIC,  _("lower case letters and numbers only."));
//      itemBoxSizer2->Add( itemStaticTextLegend1, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP, 5 );
     
     wxStaticText *itemStaticTextLegend2 = new wxStaticText( itemDialog1, wxID_STATIC,  _("No symbols or spaces are allowed."));
     itemBoxSizer2->Add( itemStaticTextLegend2, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP, 5 );
     
     
     wxBoxSizer* itemBoxSizer16 = new wxBoxSizer( wxHORIZONTAL );
     itemBoxSizer2->Add( itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 5 );
     
     {
         m_CancelButton = new wxButton( itemDialog1, ID_GETIP_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
         itemBoxSizer16->Add( m_CancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
         m_CancelButton->SetDefault();
     }
     
     m_OKButton = new wxButton( itemDialog1, ID_GETIP_OK, _("OK"), wxDefaultPosition,
                                wxDefaultSize, 0 );
     itemBoxSizer16->Add( m_OKButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
     
     
 }
 
 
 bool oeUniGETSystemName::ShowToolTips()
 {
     return TRUE;
 }
 
 wxString oeUniGETSystemName::GetNewName()
 {
     return m_SystemNameCtl->GetValue();
 }
 
 
 void oeUniGETSystemName::OnCancelClick( wxCommandEvent& event )
 {
     EndModal(2);
 }
 
 void oeUniGETSystemName::OnOkClick( wxCommandEvent& event )
 {
     if( m_SystemNameCtl->GetValue().Length() == 0 )
         EndModal(1);
     else {
         //g_systemNameChoiceArray.Insert(m_SystemNameCtl->GetValue(), 0);   // Top of the list
         
         EndModal(0);
     }
 }
 

 
 IMPLEMENT_DYNAMIC_CLASS( oeUniSystemNameSelector, wxDialog )
 BEGIN_EVENT_TABLE( oeUniSystemNameSelector, wxDialog )
    EVT_BUTTON( ID_GETIP_CANCEL, oeUniSystemNameSelector::OnCancelClick )
    EVT_BUTTON( ID_GETIP_OK, oeUniSystemNameSelector::OnOkClick )
 END_EVENT_TABLE()
 
 
 oeUniSystemNameSelector::oeUniSystemNameSelector()
 {
 }
 
 oeUniSystemNameSelector::oeUniSystemNameSelector( wxWindow* parent, bool bshowAll, wxWindowID id, const wxString& caption,
                                           const wxPoint& pos, const wxSize& size, long style )
 {
     
     long wstyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER;
     wxDialog::Create( parent, id, caption, pos, size, wstyle );

#ifdef __OCPN__ANDROID__
    SetBackgroundColour(ANDROID_DIALOG_BACKGROUND_COLOR);
    SetForegroundColour(wxColour(200, 200, 200));
#endif
     
     wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
     SetFont( *qFont );
     
     CreateControls( bshowAll );
     GetSizer()->SetSizeHints( this );
     Centre();
     
 }
 
 oeUniSystemNameSelector::~oeUniSystemNameSelector()
 {
 }
 
  
 bool oeUniSystemNameSelector::Create( wxWindow* parent, bool bShowAll, wxWindowID id, const wxString& caption,
                                   const wxPoint& pos, const wxSize& size, long style )
 {
     SetExtraStyle( GetExtraStyle() | wxWS_EX_BLOCK_EVENTS );
     
     long wstyle = style;
     #ifdef __WXMAC__
     wstyle |= wxSTAY_ON_TOP;
     #endif
     wxDialog::Create( parent, id, caption, pos, size, wstyle );
     
     wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
     SetFont( *qFont );
     
     SetTitle( _("Select OpenCPN/oeRNC System Name"));
    
#ifdef __OCPN__ANDROID__
    SetBackgroundColour(ANDROID_DIALOG_BACKGROUND_COLOR);
    SetForegroundColour(wxColour(200, 200, 200));
#endif
    
     CreateControls( bShowAll );
     Centre();
     return TRUE;
 }
 
 
 void oeUniSystemNameSelector::CreateControls( bool bShowAll )
 {
     oeUniSystemNameSelector* itemDialog1 = this;
     
     wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
     itemDialog1->SetSizer( itemBoxSizer2 );
     
     
     wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _("Select your System Name from the following list, or "),
                                                       wxDefaultPosition, wxDefaultSize, 0 );
     itemStaticText5->Wrap(-1);
     itemBoxSizer2->Add( itemStaticText5, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP, 5 );
 
     wxStaticText* itemStaticText6 = new wxStaticText( itemDialog1, wxID_STATIC, _(" create a new System Name for this computer."),
                                                       wxDefaultPosition, wxDefaultSize, 0 );
     itemStaticText6->Wrap(-1);
                                                  
     itemBoxSizer2->Add( itemStaticText6, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP, 5 );
                                                       
     
     bool bDongleAdded = false;
     wxArrayString system_names;
     
     if(bShowAll){
        for(unsigned int i=0 ; i < g_systemNameChoiceArray.GetCount() ; i++){
            wxString candidate = g_systemNameChoiceArray.Item(i);
            if(candidate.StartsWith("sgl")){
                if(g_systemNameDisabledArray.Index(candidate) == wxNOT_FOUND){
                    system_names.Add(candidate + _T(" (") + _("USB Key Dongle") + _T(")"));
                    bDongleAdded = true;
                }
            }

            else if(g_systemNameDisabledArray.Index(candidate) == wxNOT_FOUND)
            system_names.Add(candidate);
        }
     }
     
     // Add USB dongle if present, and not already added
     
     if(!bDongleAdded && IsDongleAvailable()){
        system_names.Add( g_dongleName  + _T(" (") + _("USB Key Dongle") + _T(")"));
     }

     system_names.Add(_("new..."));

    wxPanel *selectorPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxBG_STYLE_ERASE );
    itemBoxSizer2->Add(selectorPanel, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
#ifdef __OCPN__ANDROID__
    selectorPanel->SetForegroundColour(wxColour(200, 200, 200));
    selectorPanel->SetBackgroundColour(ANDROID_DIALOG_BODY_COLOR);
#endif    

    wxBoxSizer *boxSizercPanel = new wxBoxSizer(wxVERTICAL);
    selectorPanel->SetSizer(boxSizercPanel);

    wxScrolledWindow *rbScroller = new wxScrolledWindow(selectorPanel, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxBORDER_RAISED | wxVSCROLL | wxBG_STYLE_ERASE );
    rbScroller->SetScrollRate(5, 5);
    boxSizercPanel->Add(rbScroller, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    wxBoxSizer *boxSizerrB = new wxBoxSizer(wxVERTICAL);
    rbScroller->SetSizer(boxSizerrB);

    rbScroller->SetMinSize(wxSize(-1,10 * GetCharHeight()));
 
     m_rbSystemNames = new wxRadioBox(rbScroller, wxID_ANY, _("System Names"), wxDefaultPosition, wxDefaultSize, system_names, 0, wxRA_SPECIFY_ROWS);
     
     boxSizerrB->Add( m_rbSystemNames, 0, wxALIGN_CENTER | wxALL, 25 );

     wxStaticLine* itemStaticLine = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
     itemBoxSizer2->Add( itemStaticLine, 0, wxEXPAND|wxALL, 0 );
     
     
     wxBoxSizer* itemBoxSizer16 = new wxBoxSizer( wxHORIZONTAL );
     itemBoxSizer2->Add( itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 5 );
                                              
     m_CancelButton = new wxButton( itemDialog1, ID_GETIP_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
     itemBoxSizer16->Add( m_CancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
                                              
     m_OKButton = new wxButton( itemDialog1, ID_GETIP_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
     m_OKButton->SetDefault();
     itemBoxSizer16->Add( m_OKButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
}
 
 
bool oeUniSystemNameSelector::ShowToolTips()
{
    return TRUE;
}
 
wxString oeUniSystemNameSelector::getRBSelection(  )
{
     return m_rbSystemNames->GetStringSelection();
}
 
 void oeUniSystemNameSelector::OnCancelClick( wxCommandEvent& event )
 {
     EndModal(2);
 }
 
 void oeUniSystemNameSelector::OnOkClick( wxCommandEvent& event )
 {
     EndModal(0);
 }
 
 
 //------------------------------------------------------------------
 
 BEGIN_EVENT_TABLE( InProgressIndicator, wxGauge )
 EVT_TIMER( 4356, InProgressIndicator::OnTimer )
 END_EVENT_TABLE()
 
 InProgressIndicator::InProgressIndicator()
 {
 }
 
 InProgressIndicator::InProgressIndicator(wxWindow* parent, wxWindowID id, int range,
                     const wxPoint& pos, const wxSize& size,
                     long style, const wxValidator& validator, const wxString& name)
{
    wxGauge::Create(parent, id, range, pos, size, style, validator, name);
    
//    m_timer.Connect(wxEVT_TIMER, wxTimerEventHandler( InProgressIndicator::OnTimer ), NULL, this);
    m_timer.SetOwner( this, 4356 );
    m_timer.Start( 50 );
    
    m_bAlive = false;
    
}

InProgressIndicator::~InProgressIndicator()
{
}
 
void InProgressIndicator::OnTimer(wxTimerEvent &evt)
{
    if(m_bAlive)
        Pulse();
}
 
 
void InProgressIndicator::Start() 
{
     m_bAlive = true;
}
 
void InProgressIndicator::Stop() 
{
     m_bAlive = false;
     SetValue(0);
}

#ifdef __OCPN_USE_CURL__

//-------------------------------------------------------------------------------------------
OESENC_CURL_EvtHandler::OESENC_CURL_EvtHandler()
{
    Connect(wxCURL_BEGIN_PERFORM_EVENT, (wxObjectEventFunction)(wxEventFunction)&OESENC_CURL_EvtHandler::onBeginEvent);
    Connect(wxCURL_END_PERFORM_EVENT, (wxObjectEventFunction)(wxEventFunction)&OESENC_CURL_EvtHandler::onEndEvent);
    Connect(wxCURL_DOWNLOAD_EVENT, (wxObjectEventFunction)(wxEventFunction)&OESENC_CURL_EvtHandler::onProgressEvent);
    
}

OESENC_CURL_EvtHandler::~OESENC_CURL_EvtHandler()
{
}

void OESENC_CURL_EvtHandler::onBeginEvent(wxCurlBeginPerformEvent &evt)
{
 //   ShowOERNCMessageDialog(NULL, _("DLSTART."), _("oeSENC_PI Message"), wxOK);
    g_shopPanel->m_startedDownload = true;
    g_shopPanel->m_buttonCancelOp->Show();

}

void OESENC_CURL_EvtHandler::onEndEvent(wxCurlEndPerformEvent &evt)
{
 //   ShowOERNCMessageDialog(NULL, _("DLEnd."), _("oeSENC_PI Message"), wxOK);
    
    g_ipGauge->Stop();
    g_shopPanel->setStatusTextProgress(_T(""));
    g_shopPanel->setStatusText( _("Status: OK"));
    g_shopPanel->m_buttonCancelOp->Hide();
    //g_shopPanel->GetButtonDownload()->Hide();
    g_shopPanel->GetButtonUpdate()->Enable();
    
    if(downloadOutStream){
        downloadOutStream->Close();
        downloadOutStream = NULL;
    }
    
    g_curlDownloadThread = NULL;

    if(g_shopPanel->m_bAbortingDownload){
        if(g_shopPanel->GetSelectedChartPanel()){
//             oitemChart *chart = g_shopPanel->GetSelectedChart()->m_pChart;
//             if(chart){
//                 chart->downloadingFile.Clear();
//             }
        }
    }
            
    //  Send an event to chain back to "Install" button
    wxCommandEvent event(wxEVT_COMMAND_BUTTON_CLICKED);
    event.SetId( ID_CMD_BUTTON_INSTALL_CHAIN );
    g_shopPanel->GetEventHandler()->AddPendingEvent(event);
    
}


void OESENC_CURL_EvtHandler::onProgressEvent(wxCurlDownloadEvent &evt)
{
    dl_now = evt.GetDownloadedBytes();
    dl_total = evt.GetTotalBytes();
    
    // Calculate the gauge value
    if(evt.GetTotalBytes() > 0){
        float progress = evt.GetDownloadedBytes()/evt.GetTotalBytes();
        g_ipGauge->SetValue(progress * 100);
    }
    
    wxDateTime now = wxDateTime::Now();
    if(now.GetTicks() != g_progressTicks){
        std::string speedString = evt.GetHumanReadableSpeed(" ", 0);
    
    //  Set text status
        wxString tProg;
        tProg = _("Downloaded:  ");
        wxString msg;
        msg.Printf( _T("%6.1f MiB / %4.0f MiB    "), (float)(evt.GetDownloadedBytes() / 1e6), (float)(evt.GetTotalBytes() / 1e6));
        msg += wxString( speedString.c_str(), wxConvUTF8);
        tProg += msg;

        g_shopPanel->setStatusTextProgress( tProg );
        
        g_progressTicks = now.GetTicks();
    }
    
}
#endif   //__OCPN_USE_CURL__



IMPLEMENT_DYNAMIC_CLASS( oeUniLogin, wxDialog )
BEGIN_EVENT_TABLE( oeUniLogin, wxDialog )
EVT_BUTTON( ID_GETIP_CANCEL, oeUniLogin::OnCancelClick )
EVT_BUTTON( ID_GETIP_OK, oeUniLogin::OnOkClick )
END_EVENT_TABLE()


oeUniLogin::oeUniLogin()
{
}

oeUniLogin::oeUniLogin( wxWindow* parent, wxWindowID id, const wxString& caption,
                                          const wxPoint& pos, const wxSize& size, long style )
{
    
    long wstyle = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER;

    m_bCompact = false;
    wxSize sz = ::wxGetDisplaySize();
    if((sz.x < 500) | (sz.y < 500))
        m_bCompact = true;
    
    wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
    
#ifndef __OCPN__ANDROID__
    SetFont( *qFont );
#else
    if(m_bCompact){
        SetFont( *qFont );
    }
    else{
        double font_size = qFont->GetPointSize() * 1.20;
        wxFont *dFont = wxTheFontList->FindOrCreateFont( font_size, qFont->GetFamily(), qFont->GetStyle(),  qFont->GetWeight());
        SetFont( *dFont );
    }
#endif
    
    wxDialog::Create( parent, id, caption, pos, size, wstyle );
    
    CreateControls();
    GetSizer()->SetSizeHints( this );
    Centre();
    Move(-1, 2 * GetCharHeight());
    
}

oeUniLogin::~oeUniLogin()
{
}

/*!
 * oeRNCLogin creator
 */

bool oeUniLogin::Create( wxWindow* parent, wxWindowID id, const wxString& caption,
                                  const wxPoint& pos, const wxSize& size, long style )
{
    SetExtraStyle( GetExtraStyle() | wxWS_EX_BLOCK_EVENTS );
    
    long wstyle = style;
    #ifdef __WXMAC__
    wstyle |= wxSTAY_ON_TOP;
    #endif
    wxDialog::Create( parent, id, caption, pos, size, wstyle );
    
//     wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
//     SetFont( *qFont );

//     wxFont *qFont = GetOCPNScaledFont_PlugIn(_("Dialog"));
//     double font_size = qFont->GetPointSize() * 3 / 2;
//     wxFont *dFont = wxTheFontList->FindOrCreateFont( font_size, qFont->GetFamily(), qFont->GetStyle(),  qFont->GetWeight());
//     SetFont( *dFont );
    
    
    CreateControls(  );
    Centre();

    return TRUE;
}


void oeUniLogin::CreateControls(  )
{
    int ref_len = GetCharHeight();
    
    oeUniLogin* itemDialog1 = this;

#ifndef __OCPN__ANDROID__    
    
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
    itemDialog1->SetSizer( itemBoxSizer2 );
    
    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox( itemDialog1, wxID_ANY, _("Login to o-charts.org") );
    
    wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer( itemStaticBoxSizer4Static, wxVERTICAL );
    itemBoxSizer2->Add( itemStaticBoxSizer4, 0, wxEXPAND | wxALL, 5 );
    
    itemStaticBoxSizer4->AddSpacer(10);
    
    wxStaticLine *staticLine121 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxLI_HORIZONTAL);
    itemStaticBoxSizer4->Add(staticLine121, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    wxFlexGridSizer* flexGridSizerActionStatus = new wxFlexGridSizer(0, 2, 0, 0);
    flexGridSizerActionStatus->SetFlexibleDirection( wxBOTH );
    flexGridSizerActionStatus->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizerActionStatus->AddGrowableCol(0);
    
    itemStaticBoxSizer4->Add(flexGridSizerActionStatus, 1, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    wxStaticText* itemStaticText5 = new wxStaticText( itemDialog1, wxID_STATIC, _("email address:"), wxDefaultPosition, wxDefaultSize, 0 );
    flexGridSizerActionStatus->Add( itemStaticText5, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 5 );
    
    m_UserNameCtl = new wxTextCtrl( itemDialog1, ID_GETIP_IP, _T(""), wxDefaultPosition, wxSize( ref_len * 10, -1 ), 0 );
    flexGridSizerActionStatus->Add( m_UserNameCtl, 0,  wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM , 5 );
    
 
    wxStaticText* itemStaticText6 = new wxStaticText( itemDialog1, wxID_STATIC, _("Password:"), wxDefaultPosition, wxDefaultSize, 0 );
    flexGridSizerActionStatus->Add( itemStaticText6, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP, 5 );
    
    m_PasswordCtl = new wxTextCtrl( itemDialog1, ID_GETIP_IP, _T(""), wxDefaultPosition, wxSize( ref_len * 10, -1 ), wxTE_PASSWORD );
    flexGridSizerActionStatus->Add( m_PasswordCtl, 0,  wxALIGN_CENTER | wxLEFT | wxRIGHT | wxBOTTOM , 5 );
    
    
    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer( wxHORIZONTAL );
    itemBoxSizer2->Add( itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 5 );
    
    m_CancelButton = new wxButton( itemDialog1, ID_GETIP_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add( m_CancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
    
    m_OKButton = new wxButton( itemDialog1, ID_GETIP_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    m_OKButton->SetDefault();
    
    itemBoxSizer16->Add( m_OKButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );
#else
    
    SetBackgroundColour(ANDROID_DIALOG_BACKGROUND_COLOR);
    SetForegroundColour(wxColour(200, 200, 200));
    
    wxBoxSizer* itemBoxSizer2 = new wxBoxSizer( wxVERTICAL );
    itemDialog1->SetSizer( itemBoxSizer2 );
    
    wxStaticBox* itemStaticBoxSizer4Static = new wxStaticBox( itemDialog1, wxID_ANY, _("Login to o-charts.org") );
    
    wxStaticBoxSizer* itemStaticBoxSizer4 = new wxStaticBoxSizer( itemStaticBoxSizer4Static, wxVERTICAL );
    itemBoxSizer2->Add( itemStaticBoxSizer4, 0, wxEXPAND | wxALL, 5 );
    
    itemStaticBoxSizer4->AddSpacer(10);
    
    wxStaticLine *staticLine121 = new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxLI_HORIZONTAL);
    itemStaticBoxSizer4->Add(staticLine121, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    
    
        
    wxPanel *loginPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDLG_UNIT(this, wxSize(-1,-1)), wxBG_STYLE_ERASE );
    itemStaticBoxSizer4->Add(loginPanel, 0, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    loginPanel->SetForegroundColour(wxColour(200, 200, 200));
    
    wxBoxSizer *boxSizercPanel = new wxBoxSizer(wxVERTICAL);
    loginPanel->SetSizer(boxSizercPanel);

    loginPanel->SetBackgroundColour(ANDROID_DIALOG_BODY_COLOR);
    
    wxFlexGridSizer* flexGridSizerActionStatus = new wxFlexGridSizer(0, 2, 0, 0);
    flexGridSizerActionStatus->SetFlexibleDirection( wxBOTH );
    flexGridSizerActionStatus->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
    flexGridSizerActionStatus->AddGrowableCol(0);
    
    boxSizercPanel->Add(flexGridSizerActionStatus, 1, wxALL|wxEXPAND, WXC_FROM_DIP(5));
    
    int item_space = 50;
    if(m_bCompact)
        item_space = 10;
    
    wxStaticText* itemStaticText5 = new wxStaticText( loginPanel, wxID_STATIC, _("email:"), wxDefaultPosition, wxDefaultSize, 0 );
    flexGridSizerActionStatus->Add( itemStaticText5, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, item_space );
    
    m_UserNameCtl = new wxTextCtrl( loginPanel, ID_GETIP_IP, _T(""), wxDefaultPosition, wxSize( ref_len * 10, -1 ), 0 );
    flexGridSizerActionStatus->Add( m_UserNameCtl, 0,  wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP , item_space );
    
    wxStaticText* itemStaticText6 = new wxStaticText( loginPanel, wxID_STATIC, _("pass:"), wxDefaultPosition, wxDefaultSize, 0 );
    flexGridSizerActionStatus->Add( itemStaticText6, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxTOP | wxADJUST_MINSIZE, item_space );
    
    m_PasswordCtl = new wxTextCtrl( loginPanel, ID_GETIP_IP, _T(""), wxDefaultPosition, wxSize( ref_len * 10, -1 ), wxTE_PASSWORD );
    m_PasswordCtl->SetBackgroundColour(wxColour(0, 192, 192));
    flexGridSizerActionStatus->Add( m_PasswordCtl, 0,  wxALIGN_CENTER | wxLEFT | wxRIGHT | wxTOP , item_space );
    
 
    int button_space = 100;
    if(m_bCompact)
        button_space = 20;
    
    wxBoxSizer* itemBoxSizer16 = new wxBoxSizer( wxHORIZONTAL );
    boxSizercPanel->Add( itemBoxSizer16, 0, wxALIGN_RIGHT | wxALL, 5 );
    
    m_CancelButton = new wxButton( loginPanel, ID_GETIP_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
    itemBoxSizer16->Add( m_CancelButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, button_space );
    
    m_OKButton = new wxButton( loginPanel, ID_GETIP_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
    m_OKButton->SetDefault();
    
    itemBoxSizer16->Add( m_OKButton, 0, wxALIGN_CENTER_VERTICAL | wxALL, button_space );
#endif    
    
}


bool oeUniLogin::ShowToolTips()
{
    return TRUE;
}



void oeUniLogin::OnCancelClick( wxCommandEvent& event )
{
    EndModal(2);
}

void oeUniLogin::OnOkClick( wxCommandEvent& event )
{
    if( (m_UserNameCtl->GetValue().Length() == 0 ) || (m_PasswordCtl->GetValue().Length() == 0 ) ){
        SetReturnCode(1);
        EndModal(1);
    }
    else {
        SetReturnCode(0);
        EndModal(0);
    }
}

void oeUniLogin::OnClose( wxCloseEvent& event )
{
    SetReturnCode(2);
}

