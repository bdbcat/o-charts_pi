
#ifndef _OISCREENLOG_H_
#define _OISCREENLOG_H_

#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers


#include "wx/socket.h"

class oesu_piScreenLog : public wxWindow
{
public:
    oesu_piScreenLog(wxWindow *parent);
    ~oesu_piScreenLog();
    
    void Init();
    
    void LogMessage(wxString s);
    void ClearLog(void);
    void ClearLogSeq(void){ m_nseq = 0; }
    
    void OnServerEvent(wxSocketEvent& event);
    void OnSocketEvent(wxSocketEvent& event);
    void OnSize( wxSizeEvent& event);
    void StartServer( unsigned int port);
    void StopServer();
    
private:    
    wxTextCtrl          *m_plogtc;
    unsigned int        m_nseq;
    
    wxSocketServer      *m_server;
    unsigned int        m_backchannel_port;
    bool                m_bsuppress_log;
    
    DECLARE_EVENT_TABLE()
    
};

class oesu_piScreenLogContainer : public wxDialog
{
    DECLARE_DYNAMIC_CLASS( oesu_piScreenLogContainer )
    DECLARE_EVENT_TABLE()

public:
    oesu_piScreenLogContainer();
    oesu_piScreenLogContainer(wxWindow *parent, wxString title, wxSize size = wxDefaultSize);
    ~oesu_piScreenLogContainer();
 
    void OnCloseClick(wxCommandEvent& event);

    void LogMessage(wxString s);
    void ClearLog(void);
    oesu_piScreenLog        *m_slog;
    
private:    
    
};


#endif