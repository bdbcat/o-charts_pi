/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  oesenc Plugin
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

#ifndef _OCHARTSHOP_H_
#define _OCHARTSHOP_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#include <wx/statline.h>
//#include <../../wxWidgets/wxWidgets-3.0.2/wxWidgets-3.0.2/include/wx/gtk/gauge.h>

#include "ocpn_plugin.h"

#ifndef __OCPN__ANDROID__
 #include "wx/curl/http.h"
#endif

#ifdef WXC_FROM_DIP
#undef WXC_FROM_DIP
#endif

#ifdef __OCPN__ANDROID__
    #define WXC_FROM_DIP(x) x
#else    
    #if wxVERSION_NUMBER >= 3100
        #define WXC_FROM_DIP(x) wxWindow::FromDIP(x, NULL)
    #else
        #define WXC_FROM_DIP(x) x
    #endif
#endif    

#include <vector>

wxString ProcessResponse(std::string, bool bsubAmpersand = false);
int GetEditionInt(std::string edition);
int ShowOERNCMessageDialog(wxWindow *parent, const wxString& message,  const wxString& caption, long style = wxOK);
int doLogin( wxWindow *parent );

class shopPanel;
class InProgressIndicator;
class oesu_piScreenLog;
class oesu_piScreenLogContainer;
class oeUniLogin;
class ocValidator;

enum{
        STAT_UNKNOWN = 0,
        STAT_PURCHASED,
        STAT_CURRENT,
        STAT_STALE,
        STAT_EXPIRED,
        STAT_EXPIRED_MINE,
        STAT_PREPARING,
        STAT_READY_DOWNLOAD,
        STAT_REQUESTABLE,
        STAT_NEED_REFRESH,
        STAT_PURCHASED_NOSLOT
};

enum{
        TASK_NULL = 0,
        TASK_REPLACE,
        TASK_UPDATE
};

enum{
        CHART_TYPE_OEUSENC = 0,
        CHART_TYPE_OERNC
};

class itemChartDataKeys
{
public:
    itemChartDataKeys(){}
    ~itemChartDataKeys(){}
    
    std::string  Name;
    std::string  ID;
    std::string  fileName;
    std::string  RIK;

};

class ChartSetKeys
{
public:
    ChartSetKeys(){ m_bOK = false; }
    ChartSetKeys(std::string fileXML);
    ~ChartSetKeys(){}
    
    bool Load( std::string fileNameKap );
    bool RemoveKey( std::string fileNameKap );
    bool AddKey(itemChartDataKeys *kdata);
    bool WriteFile( std::string fileName);
    void CopyAuxData( ChartSetKeys &source);

    std::vector < itemChartDataKeys *> chartList;
    std::string m_chartInfo,  m_chartInfoEdition, m_chartInfoExpirationDate;
    std::string m_chartInfoShow, m_chartInfoEULAShow, m_chartInfoDisappearingDate;

    bool m_bOK;
};

class itemChartData
{
public:
    itemChartData(){}
    ~itemChartData(){}
    
    std::string  Name;
    std::string  ID;
    std::string  SE;
    std::string  RE;
    std::string  ED;
    std::string  Scale;

};

class ChartSetData
{
public:
    ChartSetData(std::string fileXML);
    ~ChartSetData(){}
    
    bool RemoveChart( std::string fileNameKap );
    bool AddChart(itemChartData *cdata);
    bool WriteFile( std::string fileName);
    void setEditionTag( std::string tag ){ editionTag = tag; }

    std::vector < itemChartData *> chartList;
    std::string editionTag;

    
};

    
class itemDLTask
{
public:
    itemDLTask(){}
    ~itemDLTask(){}
    
    std::string uuidParentSlot;
    std::string url;
    std::string localFile;
    long        currentOffset;
    long        totalSize;
    std::string SHA256;
};
    
    
class itemTaskFileInfo
{
public:
    itemTaskFileInfo(){ bsubBase = false; }
    ~itemTaskFileInfo(){}
    
    std::string targetEdition;
    std::string resultEdition;
    std::string link;
    std::string size;
    std::string sha256;
    std::string linkKeys;
    std::string sha256Keys;
    std::string cacheKeysLocn;
    std::string cacheLinkLocn;
    std::string chartsetNameNormalized;          // like "oeRNC-XXXXX"
    bool bsubBase;
};




class itemSlot
{
public:
    itemSlot(){ slotID = -1; }
    ~itemSlot(){}
    
    int GetInstalledEditionInt(){ return GetEditionInt(installedEdition); }

    int slotID;
    std::string slotUuid;
    std::string assignedSystemName;
    std::string lastRequested;
    std::string installLocation;
    std::string installedEdition;
    
    std::vector<itemDLTask> dlQueue;
    std::vector<itemTaskFileInfo *>taskFileList;
    
    unsigned int idlQueue;

};
                                                

class itemQuantity
{
public:
    itemQuantity(){ quantityId = -1; }
    ~itemQuantity(){}
    
    int quantityId;
    std::vector<itemSlot *> slotList;
};


//      A single chart(set) container
class itemChart
{
public:    
    itemChart() { m_downloading = false; m_bEnabled = true; m_assignedSlotIndex = -1; m_activeQtyID = -1; maxSlots = 0; bshopValidated=false; bExpired = false;}
    ~itemChart() {};

    void setDownloadPath(int slot, wxString path);
    wxString getDownloadPath(int slot); 
    bool isChartsetExpired();
    bool isChartsetDontShow();
    bool isChartsetShow();

    bool isChartsetFullyAssigned();
    bool isChartsetAssignedToSystemKey(wxString key);
    bool isChartsetAssignedToAnyDongle();
    bool isChartsetAssignedToMe();

    int GetSlotAssignedToInstalledDongle( int &qId );
    int GetSlotAssignedToSystem( int &qId );
    int FindQuantityIndex( int nqty);
    int getChartAssignmentCount();
    itemSlot *GetActiveSlot();
    bool isUUIDAssigned( wxString UUID);
    wxString GetDisplayedChartEdition();
    
    itemSlot *GetSlotPtr( wxString UUID );
    itemSlot *GetSlotPtr(int slot, int qId);
    
    void Update(itemChart *other);
    
public:    
    bool isEnabled(){ return m_bEnabled; }
    wxString getStatusString();
    int getChartStatus();
    bool isThumbnailReady();
    wxBitmap& GetChartThumbnail(int size, bool bDL_If_Needed = false);
    wxString getKeytypeString( std::string slotUUID );
    int GetServerEditionInt();
    int GetChartType(){ return chartType; }
    
    std::string orderRef;
    std::string purchaseDate;
    std::string expDate;
    std::string chartName;
    std::string chartID;
    std::string serverChartEdition;
    std::string editionTag;
    std::string editionDate;
    std::string thumbLink;
    std::string overrideChartEdition;
    int chartType;
    
    unsigned int maxSlots;
    bool bExpired;
    int m_assignedSlotIndex;
    int m_activeQtyID;
    
    wxArrayString baseChartListArray;
    wxArrayString updateChartListArray;
    
    std::vector<itemQuantity> quantityList;

    // Computed update task parameters
    wxString  taskRequestedFile;
    wxString  taskRequestedEdition;
    wxString  taskCurrentEdition;
    int       taskAction;
    
    bool m_downloading;
    wxString downloadingFile;
    
    long downloadReference;
    bool m_bEnabled;
    wxImage m_ChartImage;
    wxBitmap m_bm;
    
    wxString lastInstall;          // For updates, the full path of installed chartset
    int m_status;
    wxString lastInstalledtlDir;
    bool bshopValidated;     
};



WX_DECLARE_OBJARRAY(itemChart *,      ArrayOfCharts);    

//  The main entry point for ocharts Shop interface
int doShop();
    
class oeXChartPanel: public wxPanel
{
public:
    oeXChartPanel( wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, itemChart *p_itemChart, shopPanel *pContainer );
    ~oeXChartPanel();
    
    void OnChartSelected( wxMouseEvent &event );
    void SetSelected( bool selected );
    void OnPaint( wxPaintEvent &event );
    void OnEraseBackground( wxEraseEvent &event );
    
    bool GetSelected(){ return m_bSelected; }
    int GetUnselectedHeight(){ return m_unselectedHeight; }
    itemChart *GetSelectedChart() { return m_pChart; }

    
private:
    shopPanel *m_pContainer;
    bool m_bSelected;
    wxStaticText *m_pName;
    wxColour m_boxColour;
    int m_unselectedHeight;
    itemChart *m_pChart;
    
    DECLARE_EVENT_TABLE()
};

#if 0
class oeSencChartPanel: public wxPanel
{
public:
    oeSencChartPanel( wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size, oitemChart *p_itemChart, shopPanel *pContainer );
    ~oeSencChartPanel();
    
    void OnChartSelected( wxMouseEvent &event );
    void SetSelected( bool selected );
    void OnPaint( wxPaintEvent &event );
    void OnEraseBackground( wxEraseEvent &event );
    
    bool GetSelected(){ return m_bSelected; }
    int GetUnselectedHeight(){ return m_unselectedHeight; }
    oitemChart *GetSelectedChart() { return m_pChart; }
    
    oitemChart *m_pChart;
    
private:
    shopPanel *m_pContainer;
    bool m_bSelected;
    wxStaticText *m_pName;
    wxColour m_boxColour;
    int m_unselectedHeight;
    
    DECLARE_EVENT_TABLE()
};



WX_DECLARE_OBJARRAY(oeSencChartPanel *,      ArrayOfChartPanels);    

#endif

WX_DECLARE_OBJARRAY(oeXChartPanel *,      ArrayOfChartPanels);    

class chartScroller : public wxScrolledWindow
{
    DECLARE_EVENT_TABLE()

public:
    chartScroller(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style);
    void OnEraseBackground(wxEraseEvent& event);
    void DoPaint(wxDC& dc);
    void OnPaint( wxPaintEvent &event );
    
    
};


class shopPanel : public wxPanel
{
    DECLARE_EVENT_TABLE()
    
protected:
    wxScrolledWindow* m_scrollWinChartList;
    wxButton* m_button37;
    wxButton* m_button39;
    wxButton* m_button41;
    wxButton* m_button43;
    wxButton* m_button45;
    wxButton* m_button47;
    wxStaticText* m_staticTextSystemName;
    wxStaticText* m_staticText111;
    wxStaticText* m_staticText113;
    wxStaticText* m_staticText115;
    wxTextCtrl* m_textCtrl117;
    wxTextCtrl* m_textCtrl119;
    wxStaticLine* m_staticLine121;
    wxButton* m_buttonAssign;
    wxButton* m_buttonDownload;
    wxButton* m_buttonInstall;
    wxButton* m_buttonUpdate;
    wxButton* m_buttonValidate;
    wxBoxSizer* boxSizerCharts;
    wxBoxSizer* gridSizerActionButtons;
    
    std::vector<oeXChartPanel *> panelVector;
    
    oeXChartPanel *m_ChartPanelSelected;
    
    wxChoice* m_choiceSystemName;
    wxButton* m_buttonNewSystemName;
    wxTextCtrl*  m_sysName;
    wxButton* m_buttonChangeSystemName;
    wxStaticText *m_staticTextStatus;
    wxStaticText *m_staticTextStatusProgress;
    wxStaticText *m_staticTextLEM;
    

protected:
    void SortChartList();

    
public:
    wxButton* GetButton37() { return m_button37; }
    wxButton* GetButton39() { return m_button39; }
    wxButton* GetButton41() { return m_button41; }
    wxButton* GetButton43() { return m_button43; }
    wxButton* GetButton45() { return m_button45; }
    wxButton* GetButton47() { return m_button47; }
    wxScrolledWindow* GetScrollWinChartList() { return m_scrollWinChartList; }
    wxStaticText* GetStaticTextSystemName() { return m_staticTextSystemName; }
    wxStaticText* GetStaticText111() { return m_staticText111; }
    wxStaticText* GetStaticText113() { return m_staticText113; }
    wxStaticText* GetStaticText115() { return m_staticText115; }
    wxTextCtrl* GetTextCtrl117() { return m_textCtrl117; }
    wxTextCtrl* GetTextCtrl119() { return m_textCtrl119; }
    wxStaticLine* GetStaticLine121() { return m_staticLine121; }
    //wxButton* GetButtonAssign() { return m_buttonAssign; }
    //wxButton* GetButtonDownload() { return m_buttonDownload; }
    wxButton* GetButtonInstall() { return m_buttonInstall; }
    wxButton* GetButtonUpdate() { return m_buttonUpdate; }
    
    oesu_piScreenLog *m_shopLog;
    
    oeUniLogin *m_login;
    
    void RefreshSystemName();
    void SetErrorMessage();
    
    shopPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(500,600), long style = wxTAB_TRAVERSAL);
    virtual ~shopPanel();
    
    //void SelectChart( oeSencChartPanel *chart );
    void SelectChart( oeXChartPanel *chart );
    void SelectChartByID( std::string id, std::string order);
    
    oeXChartPanel *GetSelectedChartPanel(){ return m_ChartPanelSelected; }
    
    void OnButtonUpdate( wxCommandEvent& event );
    void OnButtonDownload( wxCommandEvent& event );
    void OnButtonCancelOp( wxCommandEvent& event );
    void OnButtonInstall( wxCommandEvent& event );
    void OnButtonInstallChain( wxCommandEvent& event );
    void ValidateChartset( wxCommandEvent& event );

    void OnPrepareTimer(wxTimerEvent &evt);
    int doPrepareGUI(itemSlot *activeSlot);
    int doDownloadGui( itemChart *targetChart, itemSlot *targetSlot);
    
    void UpdateChartList();
    void OnGetNewSystemName( wxCommandEvent& event );
    void OnChangeSystemName( wxCommandEvent& event );
    bool doSystemNameWizard( bool bshowAll);
    wxString doGetNewSystemName( );
    void UpdateActionControls();
    void setStatusText( const wxString &text ){ m_staticTextStatus->SetLabel( text );  m_staticTextStatus->Refresh(); }
    wxString getStatusText(){ return m_staticTextStatus->GetLabel(); }
    void setStatusTextProgress( const wxString &text ){ m_staticTextStatus/*m_staticTextStatusProgress*/->SetLabel( text );  /*m_staticTextStatusProgress->Refresh();*/ }
    void MakeChartVisible(oeXChartPanel *chart);
    int ComputeUpdates(itemChart *chart, itemSlot *slot);
    bool GetNewSystemName( bool bShowAll = true);
    int processTask(itemSlot *slot, itemChart *chart, itemTaskFileInfo *task);
    bool validateSHA256(std::string fileName, std::string shaSum);
    int GetShopNameFromFPR();

    void onDLEvent(OCPN_downloadEvent &evt);

    int m_prepareTimerCount;
    int m_prepareTimeout;
    int m_prepareProgress;
    wxTimer m_prepareTimer;
    int m_activeSlot;
    std::string m_ChartSelectedID;
    std::string m_ChartSelectedOrder;
    wxButton* m_buttonCancelOp;
    bool m_binstallChain;
    bool m_bAbortingDownload;
    bool m_startedDownload;
    bool m_bTransferComplete;
    bool m_bTransferSuccess;
    bool m_bconnected;
    
    ocValidator *m_validator;
};


#define ID_GETIP 8200
#define SYMBOL_GETIP_STYLE wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX
#define SYMBOL_GETIP_IDNAME ID_GETIP
#define SYMBOL_GETIP_SIZE wxSize(500, 200)
#define SYMBOL_GETIP_POSITION wxDefaultPosition
#define ID_GETIP_CANCEL 8201
#define ID_GETIP_OK 8202
#define ID_GETIP_IP 8203

class oeUniGETSystemName: public wxDialog
{
    DECLARE_DYNAMIC_CLASS( oeUniGETSystemName )
    DECLARE_EVENT_TABLE()
    
public:
    oeUniGETSystemName( );
    oeUniGETSystemName( wxWindow* parent, wxWindowID id = SYMBOL_GETIP_IDNAME,
                         const wxString& caption =  _("OpenCPN o-charts System Name"),
                          const wxPoint& pos = SYMBOL_GETIP_POSITION,
                          const wxSize& size = SYMBOL_GETIP_SIZE,
                          long style = SYMBOL_GETIP_STYLE );
    
    ~oeUniGETSystemName();
    
    bool Create( wxWindow* parent, wxWindowID id = SYMBOL_GETIP_IDNAME,
                 const wxString& caption =  _("OpenCPN o-charts System Name"),
                 const wxPoint& pos = SYMBOL_GETIP_POSITION,
                 const wxSize& size = SYMBOL_GETIP_SIZE, long style = SYMBOL_GETIP_STYLE );
    
    
    void CreateControls(  );
    
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    wxString GetNewName();
    
    static bool ShowToolTips();
    
    wxTextCtrl*   m_SystemNameCtl;
    wxButton*     m_CancelButton;
    wxButton*     m_OKButton;
    
    
};


class oeUniSystemNameSelector: public wxDialog
{
    DECLARE_DYNAMIC_CLASS( oeUniSystemNameSelector )
    DECLARE_EVENT_TABLE()
    
public:
    oeUniSystemNameSelector( );
    oeUniSystemNameSelector( wxWindow* parent, bool bShowAll = true, wxWindowID id = SYMBOL_GETIP_IDNAME,
                         const wxString& caption =  _("Select OpenCPN/o-charts System Name"),
                         const wxPoint& pos = wxDefaultPosition,
                         const wxSize& size = wxSize(500, 200),
                         long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );
    
    ~oeUniSystemNameSelector();
    
    bool Create( wxWindow* parent, bool bshowAll = true, wxWindowID id = SYMBOL_GETIP_IDNAME,
                 const wxString& caption =  _("Select OpenCPN/o-charts System Name"),
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxSize(500, 200), long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );
    
    
    void CreateControls( bool bShowAll );
    
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    wxString getRBSelection();
    
    static bool ShowToolTips();
    
    wxButton*     m_CancelButton;
    wxButton*     m_OKButton;
    wxRadioBox*   m_rbSystemNames;
    
};


class InProgressIndicator: public wxGauge
{
    DECLARE_EVENT_TABLE()
    
public:    
    InProgressIndicator();
    InProgressIndicator(wxWindow* parent, wxWindowID id, int range,
                        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                        long style = wxGA_HORIZONTAL, const wxValidator& validator = wxDefaultValidator, const wxString& name = "inprogress");
    
    ~InProgressIndicator();
    
    void OnTimer(wxTimerEvent &evt);
    void Start();
    void Stop();
    
    
    wxTimer m_timer;
    int msec;
    bool m_bAlive;
};


#ifndef __OCPN__ANDROID__
class OESENC_CURL_EvtHandler : public wxEvtHandler
{
public:
    OESENC_CURL_EvtHandler();
    ~OESENC_CURL_EvtHandler();
    
    void onBeginEvent(wxCurlBeginPerformEvent &evt);
    void onEndEvent(wxCurlEndPerformEvent &evt);
    void onProgressEvent(wxCurlDownloadEvent &evt);
    
    
};
#endif

class oeUniLogin: public wxDialog
{
    DECLARE_DYNAMIC_CLASS( oeUniLogin )
    DECLARE_EVENT_TABLE()
    
public:
    oeUniLogin( );
    oeUniLogin( wxWindow* parent, wxWindowID id = wxID_ANY,
                         const wxString& caption =  _("OpenCPN Login"),
                        const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxSize(500, 200),
                        long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );
    
    ~oeUniLogin();
    
    bool Create( wxWindow* parent, wxWindowID id = wxID_ANY,
                 const wxString& caption =  _("OpenCPN Login"),
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxSize(500, 200), long style = wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX );
    
    
    void CreateControls(  );
    
    void OnCancelClick( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnClose( wxCloseEvent& event );
    
    static bool ShowToolTips();
    
    wxTextCtrl*   m_UserNameCtl;
    wxTextCtrl*   m_PasswordCtl;
    wxButton*     m_CancelButton;
    wxButton*     m_OKButton;
    bool          m_bCompact;
    
    
};

#endif          //_OCHARTSHOP_H_
