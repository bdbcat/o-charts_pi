/***************************************************************************
 * 
 * Project:  OpenCPN
 * Purpose:  XTR1_INSTREAM Object
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.         *
 **************************************************************************/

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
#include "wx/wx.h"
#endif //precompiled headers

#include "oernc_inStream.h"

#include <wx/wfstream.h>
#include <wx/filename.h>

//#define LOCAL_FILE 1

extern int g_debugLevel;
extern wxString g_pipeParm;

//--------------------------------------------------------------------------
//      Global Static storage
//      WARNING:  NOT THREAD-SAFE
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
//      Utility functions
//--------------------------------------------------------------------------

#if !defined(__WXMSW__) && !defined( __OCPN__ANDROID__)   // i.e. linux and Mac

int makeAddr(const char* name, struct sockaddr_un* pAddr, socklen_t* pSockLen)
{
    // consider this:
    //http://stackoverflow.com/questions/11640826/can-not-connect-to-linux-abstract-unix-socket
    
    int nameLen = strlen(name);
    if (nameLen >= (int) sizeof(pAddr->sun_path) -1)  /* too long? */
        return -1;
    memset(pAddr, 'x', sizeof(pAddr));
    pAddr->sun_path[0] = '\0';  /* abstract namespace */
    strncpy(pAddr->sun_path+1, name, nameLen);
    pAddr->sun_family = AF_LOCAL;
    *pSockLen = 1 + nameLen + offsetof(struct sockaddr_un, sun_path);
    return 0;
}

#endif

#ifdef __OCPN__ANDROID__
#include "qdebug.h"

//--------------------------------------------------------------------------
//      xtr1_inStream implementation as Kernel Socket
//--------------------------------------------------------------------------

int makeAddr(const char* name, struct sockaddr_un* pAddr, socklen_t* pSockLen)
{
    // consider this:
    //http://stackoverflow.com/questions/11640826/can-not-connect-to-linux-abstract-unix-socket
    
    int nameLen = strlen(name);
    if (nameLen >= (int) sizeof(pAddr->sun_path) -1)  /* too long? */
        return -1;
    //memset(pAddr, 'x', sizeof(pAddr));
    pAddr->sun_path[0] = '\0';  /* abstract namespace */
    strncpy(pAddr->sun_path+1, name, nameLen);
    pAddr->sun_family = AF_LOCAL;
    *pSockLen = 1 + nameLen + offsetof(struct sockaddr_un, sun_path);
    //qDebug() << "makeAddr::sockName: [" << pAddr->sun_path[1] << "]"; 

    return 0;
}

oernc_inStream::oernc_inStream()
{
    Init();
    
}

oernc_inStream::oernc_inStream( const wxString &file_name, const wxString &crypto_key, bool bHeaderOnly )
{
    //qDebug() << "oernc_inStream::ctor()";
    
    Init();
    
    m_fileName = file_name;
    m_cryptoKey = crypto_key;
    
    m_OK = Open( );
    if(m_OK){
        if(!Load(bHeaderOnly)){
            printf("%s\n", err);
            m_OK = false;
        }
    }
    
    
//     if(-1 != publicSocket){
//         //qDebug() << "Close() Close Socket" << publicSocket;
//         close( publicSocket );
//         publicSocket = -1;
//     }
    
    privatefifo = -1;
    m_lastBytesRead = 0;
    m_lastBytesReq = 0;
    m_uncrypt_stream = 0;
    
    
   
}

oernc_inStream::~oernc_inStream()
{
    Close();
    if(publicSocket > 0){
        // qDebug() << "dtor Close Socket" << publicSocket;
        close( publicSocket );
        publicSocket = -1;
    }
    
}

void oernc_inStream::Init()
{
    //qDebug() << "oernc_inStream::Init()";
    
    publicSocket = -1;
    
    privatefifo = -1;
    publicfifo = -1;
    m_OK = false;
    m_lastBytesRead = 0;
    m_lastBytesReq = 0;
    m_lenIDat = 0;
    m_uncrypt_stream = 0;

    strcpy(publicsocket_name,"com.opencpn.oernc_pi");
    
    if (makeAddr(publicsocket_name, &sockAddr, &sockLen) < 0){
        wxLogMessage(_T("oernc_pi: Could not makeAddr for PUBLIC socket"));
    }
    
    publicSocket = socket(AF_LOCAL, SOCK_STREAM, PF_UNIX);
    if (publicSocket < 0) {
        wxLogMessage(_T("oernc_pi: Could not make PUBLIC socket"));
    }
    
}


void oernc_inStream::Close()
{
    wxLogMessage(_T("oernc_inStream::Close()"));
    //qDebug() << "oernc_inStream::Close()";
    
    if(-1 != privatefifo){
        if(g_debugLevel)printf("   Close private fifo: %s \n", privatefifo_name);
        close(privatefifo);
        if(g_debugLevel)printf("   unlink private fifo: %s \n", privatefifo_name);
        unlink(privatefifo_name);
    }
    
    if(-1 != publicfifo)
        close(publicfifo);
    
    if(m_uncrypt_stream){
        delete m_uncrypt_stream;
    }
    
    if(-1 != publicSocket){
        //qDebug() << "Close() Close Socket" << publicSocket;
        close( publicSocket );
        publicSocket = -1;
    }
    

    Init();             // In case it want to be used again
    
}


bool oernc_inStream::Open( )
{
    wxLogMessage(_T("oernc_inStream::Open()"));
    //qDebug() << "oernc_inStream::Open()";
    //qDebug() << "sockLen: " << sockLen;
    //qDebug() << "sockName: [" << (const char *)sockAddr.sun_path+1 << "]"; 
    
    if (connect(publicSocket, (const struct sockaddr*) &sockAddr, sockLen) < 0) {
        wxLogMessage(_T("oernc_pi: Could not connect to PUBLIC socket"));
        return false;
    }
    
    return true;
}

bool oernc_inStream::readPayload( unsigned char *p )
{
    if(!Read(p, m_lenIDat).IsOk()){
        strncpy(err, "Load:  READ error Payload1", sizeof(err));
        return false;
    }
    return true;
}

bool oernc_inStream::Load( bool bHeaderOnly )
{ 
    //printf("LOAD()\n");
    //wxLogMessage(_T("ofc_pi: LOAD"));
    //qDebug() << "oernc_pi: LOAD";
    
    if(m_cryptoKey.Length() && m_fileName.length()){
        
        fifo_msg msg;
        //  Build a message for the public pipe
        
        wxCharBuffer buf = m_fileName.ToUTF8();
        if(buf.data()) 
            strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
        
        strncpy(msg.fifo_name, privatefifo_name, sizeof(privatefifo_name));
        
        buf = m_cryptoKey.ToUTF8();
        if(buf.data()) 
            strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
        
        msg.cmd = CMD_OPEN_RNC_FULL;
        if(bHeaderOnly)
            msg.cmd =CMD_OPEN_RNC;
                
        write(publicSocket, (char*) &msg, sizeof(msg));
 
                // Read the function return code
        char frcbuf[4];
        if(!Read(frcbuf, 1).IsOk()){
            strncpy(err, "Load:  READ error PFC", sizeof(err));
            qDebug() << err;
            return false;
        }
        if(frcbuf[0] == '1'){
            strncpy(err, "Load:  READ error PFCDC", sizeof(err));
            qDebug() << err;
            return false;
        }

        // Read response, by steps...
        // 1.  The composite length string
        char lbuf[100];
        
        if(!Read(lbuf, 41).IsOk()){
            strncpy(err, "Load:  READ error PL", sizeof(err));
             qDebug() << err;
           return false;
        }
        int lp1, lp2, lp3, lp4, lp5, lpl;
        sscanf(lbuf, "%d;%d;%d;%d;%d;%d;", &lp1, &lp2, &lp3, &lp4, &lp5, &lpl);
        m_lenIDat = lpl;
        
        //qDebug() << "read41 " << lp1 << lp2 << lp3 << lp4 << lp5 << lpl;
        
        int maxLen = wxMax(lp1, lp2); 
        maxLen = wxMax(maxLen, lp3);
        maxLen = wxMax(maxLen, lp4);
        maxLen = wxMax(maxLen, lp5);
        char *work = (char *)calloc(maxLen+1, sizeof(char));
        
        // 5 strings
        // 1
        if(!Read(work, lp1).IsOk()){
            strncpy(err, "Load:  READ error P1", sizeof(err));
            qDebug() << err;
            return false;
        }
        work[lp1] = 0;
        m_ep1 =std::string(work);
        //qDebug() << "ep1: " << m_ep1.c_str();

        // 2
        if(!Read(work, lp2).IsOk()){
            strncpy(err, "Load:  READ error P2", sizeof(err));
             qDebug() << err;
           return false;
        }
        work[lp2] = 0;
        m_ep2 =std::string(work);
        //qDebug() << "ep2: " << m_ep2.c_str();
        
        // 3
        if(!Read(work, lp3).IsOk()){
            strncpy(err, "Load:  READ error P3", sizeof(err));
            qDebug() << err;
            return false;
        }
        work[lp3] = 0;
        m_ep3 =std::string(work);
        
        // 4
        if(!Read(work, lp4).IsOk()){
            strncpy(err, "Load:  READ error P4", sizeof(err));
            qDebug() << err;
            return false;
        }
        work[lp4] = 0;
        m_ep4 =std::string(work);
        
        // 5
        if(!Read(work, lp5).IsOk()){
            strncpy(err, "Load:  READ error P5", sizeof(err));
            qDebug() << err;
            return false;
        }
        work[lp5] = 0;
        m_ep5 =std::string(work);
        
        free(work);

        return true;
    }
    
    return false;
}

bool oernc_inStream::SendServerCommand( unsigned char cmd )
{
    fifo_msg msg;
    
    strncpy(msg.fifo_name, privatefifo_name, sizeof(msg.fifo_name));
    
    wxCharBuffer buf = m_fileName.ToUTF8();
    if(buf.data()) 
        strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
    else
        strncpy(msg.file_name, "?", sizeof(msg.file_name));
    
    
    buf = m_cryptoKey.ToUTF8();
    if(buf.data()) 
        strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
    else
        strncpy(msg.crypto_key, "??", sizeof(msg.crypto_key));
    
    msg.cmd = cmd;
    
    write(publicSocket, (char*) &msg, sizeof(msg));
    
     return true;
}


bool oernc_inStream::IsOk()
{
    return m_OK;
}

bool oernc_inStream::Ok()
{
    return m_OK;
}


bool oernc_inStream::isAvailable(wxString user_key)
{
    //qDebug() << "\nTestAvail";
                        
                        if(m_uncrypt_stream){
                            return m_uncrypt_stream->IsOk();
                        }
                        else{
                            if(!Open()){
                                //qDebug() << "TestAvail Open FAILED\n";
                                return false;
                            }
                            
                      if( SendServerCommand(CMD_TEST_AVAIL) ){
                        //qDebug() << "TestAvail SendServerCommand OK" ;
                        char response[8];
                        memset( response, 0, 8);
                        int nTry = 5;
                        do{
                            if( Read(response, 2).IsOk() ){
                                //qDebug() << "TestAvail Response Got" << response ;
                                return( !strncmp(response, "OK", 2) );
                            }
                            
                            //qDebug() << "Sleep on TestAvail: %d", nTry;
                        wxMilliSleep(100);
                        nTry--;
                        }while(nTry);
                        
                        return false;
                            }
                            else{
                                //qDebug() << "TestAvail Open Error" ;
                                return false;
                            }
                        }
                        
                        return false;
}

wxString oernc_inStream::getHK()
{
    //wxLogMessage(_T("hk0"));
    qDebug() << "\ngetHK";
    
    if(!Open()){
        qDebug() << "getHK Open FAILED";
        wxLogMessage(_T("hk1"));
        return wxEmptyString;
    }
    
    if( SendServerCommand(CMD_GET_HK) ){
        char response[9];
        memset( response, 0, 9);
        strcpy( response, "testresp"); 
        int nTry = 5;
        do{
            if( Read(response, 8).IsOk() ){
                if(g_debugLevel)wxLogMessage(_T("hks") + wxString( response, wxConvUTF8 ));
                return( wxString( response, wxConvUTF8 ));
            }
            
            if(g_debugLevel)printf("Sleep on getHK: %d\n", nTry);
               wxLogMessage(_T("hk2"));
            
            wxMilliSleep(100);
            nTry--;
        }while(nTry);
        
        if(g_debugLevel)printf("getHK Response Timeout nTry\n");
                               wxLogMessage(_T("hk3"));
        
        return wxEmptyString;
    }
    else{
        if(g_debugLevel)printf("getHK SendServer Error\n");
                            wxLogMessage(_T("hk4"));
    }
    return wxEmptyString;
}


void oernc_inStream::Shutdown()
{
    if(!Open()){
        if(g_debugLevel)printf("Shutdown Open FAILED\n");
        return;
    }
    
    if(SendServerCommand(CMD_EXIT)) {
        char response[8];
        memset( response, 0, 8);
        Read(response, 3);
    }
}


oernc_inStream &oernc_inStream::Read(void *buffer, size_t size)
{
    #define READ_SIZE 64000;
    #define MAX_TRIES 100;
    if(!m_uncrypt_stream){
        size_t max_read = READ_SIZE;
        
        if( -1 != publicSocket){
            
            int remains = size;
            char *bufRun = (char *)buffer;
            int totalBytesRead = 0;
            int nLoop = MAX_TRIES;
            do{
                int bytes_to_read = wxMin(remains, max_read);
                if(bytes_to_read > 10000)
                    int yyp = 2;
                
                int bytesRead;
                
                #if 1
                struct pollfd fd;
                int ret;
                
                fd.fd = publicSocket; // your socket handler 
                fd.events = POLLIN;
                ret = poll(&fd, 1, 100); // 1 second for timeout
                switch (ret) {
                    case -1:
                        // Error
                        bytesRead = 0;
                        break;
                    case 0:
                        // Timeout 
                        bytesRead = 0;
                        break;
                    default:
                        bytesRead = read(publicSocket, bufRun, bytes_to_read );
                        break;
                }
                
                #else                
                bytesRead = read(publicSocket, bufRun, bytes_to_read );
                #endif                
                
                //qDebug() << "Bytes Read " << bytesRead;
                
                // Server may not have opened the Write end of the FIFO yet
                if(bytesRead == 0){
                    nLoop --;
                    wxMilliSleep(2);
                }
//                 else if(bytesRead == -1){
//                     nLoop = 0;                          // Breaks the loop on error
//                 }
                else
                    nLoop = MAX_TRIES;
                
                if(bytesRead > 0){
                    remains -= bytesRead;
                    bufRun += bytesRead;
                    totalBytesRead += bytesRead;
                }
            } while( (remains > 0) && (nLoop) );
            
            m_OK = ((size_t)totalBytesRead == size);

            m_lastBytesRead = totalBytesRead;
            m_lastBytesReq = size;
        }
        else{
            //qDebug() << "socket gone";
            m_OK=false;
        }
        
        return *this;
    }
    else{
        if(m_uncrypt_stream->IsOk())
            m_uncrypt_stream->Read(buffer, size);
        m_OK = m_uncrypt_stream->IsOk();
        return *this;
    }
}








#endif  //__OCPN__ANDROID__

#if !defined(__WXMSW__) && !defined( __OCPN__ANDROID__)   // i.e. linux and Mac

//--------------------------------------------------------------------------
//      oernc_inStream implementation
//--------------------------------------------------------------------------

oernc_inStream::oernc_inStream()
{
    Init();
    
}

oernc_inStream::oernc_inStream( const wxString &file_name, const wxString &crypto_key, bool bHeaderOnly )
{
    Init();
    
#ifdef LOCAL_FILE    
    m_uncrypt_stream = new wxFileInputStream(file_name);          // open the Header file as a read-only stream
    m_OK = m_uncrypt_stream->IsOk();
    
    strncpy(BAPfileName, file_name.mb_str(), sizeof(BAPfileName));
    m_OK = processHeader();
    if(!m_OK)
        return;
    
    m_OK = processOffsetTable();
  
#else
    m_fileName = file_name;
    m_cryptoKey = crypto_key;
    
    m_OK = Open( );
    if(m_OK){
        if(!Load( bHeaderOnly )){
            printf("%s\n", err);
            m_OK = false;
        }
    }
    
    
    // Done with the private FIFO

    if(bHeaderOnly){
        if(-1 != privatefifo){
            if(g_debugLevel)printf("   Close private fifo: %s \n", privatefifo_name);
            close(privatefifo);
            if(g_debugLevel)printf("   unlink private fifo: %s \n", privatefifo_name);
            unlink(privatefifo_name);
            privatefifo = -1;
        }
    }

    m_lastBytesRead = 0;
    m_lastBytesReq = 0;
    m_uncrypt_stream = 0;
    
    
#endif
    
    
}

oernc_inStream::~oernc_inStream()
{
    Close();
}

void oernc_inStream::Init()
{
    privatefifo = -1;
    publicfifo = -1;
    m_OK = false;
    m_lastBytesRead = 0;
    m_lastBytesReq = 0;
    m_lenIDat = 0;
    m_uncrypt_stream = 0;
    
   
}


void oernc_inStream::Close()
{
    if(-1 != privatefifo){
        if(g_debugLevel)printf("   Close private fifo: %s \n", privatefifo_name);
        close(privatefifo);
        if(g_debugLevel)printf("   unlink private fifo: %s \n", privatefifo_name);
        unlink(privatefifo_name);
    }
    
    if(-1 != publicfifo)
        close(publicfifo);
    
    if(m_uncrypt_stream){
        delete m_uncrypt_stream;
    }

   
    Init();             // In case it want to be used again
    
}

bool oernc_inStream::Open( )
{
        
    //printf("OPEN()\n");
    //wxLogMessage(_T("ofc_pi: OPEN"));

    // Open the well known public FIFO for writing
    if( (publicfifo = open(PUBLIC, O_WRONLY | O_NDELAY) ) == -1) {
        wxLogMessage(_T("oernc_pi: Could not open PUBLIC pipe"));
        
        return false;
    }
    
             
    wxString tmp_file = wxFileName::CreateTempFileName( _T("") );
    unlink(tmp_file);
        
    wxCharBuffer bufn = tmp_file.ToUTF8();
    if(bufn.data()) 
        strncpy(privatefifo_name, bufn.data(), sizeof(privatefifo_name));
            
            // Create the private FIFO
    if(-1 == mkfifo(privatefifo_name, 0666)){
        if(g_debugLevel)printf("   mkfifo private failed: %s\n", privatefifo_name);
        return false;
    }
    else{
        if(g_debugLevel)printf("   mkfifo OK: %s\n", privatefifo_name);
    }
                
    return true;
}

bool oernc_inStream::readPayload( unsigned char *p )
{
    if(!Read(p, m_lenIDat).IsOk()){
        strncpy(err, "Load:  READ error Payload1", sizeof(err));
        return false;
    }
    return true;
}


bool oernc_inStream::Load( bool bHeaderOnly )
{ 
    //printf("LOAD()\n");
    //wxLogMessage(_T("ofc_pi: LOAD"));
    
    if(m_cryptoKey.Length() && m_fileName.length()){
     
        fifo_msg msg;
        //  Build a message for the public pipe
        
        wxCharBuffer buf = m_fileName.ToUTF8();
        if(buf.data()) 
            strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
        
        strncpy(msg.fifo_name, privatefifo_name, sizeof(privatefifo_name));
                
        buf = m_cryptoKey.ToUTF8();
        int lenc = strlen(buf.data());
        if(buf.data()) 
            strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
                
        msg.cmd = CMD_OPEN_RNC_FULL;
        if(bHeaderOnly)
            msg.cmd =CMD_OPEN_RNC;
                
        write(publicfifo, (char*) &msg, sizeof(msg));
                
                // Open the private FIFO for reading to get output of command
                // from the server.
        if((privatefifo = open(privatefifo_name, O_RDONLY /*| O_NONBLOCK*/) ) == -1) {
              wxLogMessage(_T("oernc_pi: Could not open private pipe"));
              return false;
        }

        // Read the function return code
        char frcbuf[4];
        if(!Read(frcbuf, 1).IsOk()){
            strncpy(err, "Load:  READ error PFC", sizeof(err));
            return false;
        }
        if(frcbuf[0] == '1'){
            strncpy(err, "Load:  READ error PFCDC", sizeof(err));
            return false;
        }

        // Read response, by steps...
        // 1.  The composite length string
        char lbuf[100];
        
        if(!Read(lbuf, 41).IsOk()){
            strncpy(err, "Load:  READ error PL", sizeof(err));
            return false;
        }
        int lp1, lp2, lp3, lp4, lp5, lpl;
        sscanf(lbuf, "%d;%d;%d;%d;%d;%d;", &lp1, &lp2, &lp3, &lp4, &lp5, &lpl);
        m_lenIDat = lpl;
        
        int maxLen = wxMax(lp1, lp2); 
        maxLen = wxMax(maxLen, lp3);
        maxLen = wxMax(maxLen, lp4);
        maxLen = wxMax(maxLen, lp5);
        char *work = (char *)calloc(maxLen+1, sizeof(char));
        
        // 5 strings
        // 1
        if(!Read(work, lp1).IsOk()){
            strncpy(err, "Load:  READ error P1", sizeof(err));
            return false;
        }
        work[lp1] = 0;
        m_ep1 =std::string(work);
        
        // 2
        if(!Read(work, lp2).IsOk()){
            strncpy(err, "Load:  READ error P2", sizeof(err));
            return false;
        }
        work[lp2] = 0;
        m_ep2 =std::string(work);
        
        // 3
        if(!Read(work, lp3).IsOk()){
            strncpy(err, "Load:  READ error P3", sizeof(err));
            return false;
        }
        work[lp3] = 0;
        m_ep3 =std::string(work);
        
        // 4
        if(!Read(work, lp4).IsOk()){
            strncpy(err, "Load:  READ error P4", sizeof(err));
            return false;
        }
        work[lp4] = 0;
        m_ep4 =std::string(work);
        
        // 5
        if(!Read(work, lp5).IsOk()){
            strncpy(err, "Load:  READ error P5", sizeof(err));
            return false;
        }
        work[lp5] = 0;
        m_ep5 =std::string(work);
        
        free(work);
        
#if 0
        // 1.  Compressed Header
        if(NULL == phdr){
            phdr = (compressedHeader *)calloc(1, sizeof(compressedHeader));
            if(NULL == phdr){
                strncpy(err, "Load:  cannot allocate memory", sizeof(err));
                return false;
            }
        }
        if(!Read(phdr, sizeof(compressedHeader)).IsOk()){
            strncpy(err, "Load:  READ error chdr", sizeof(err));
            return false;
        }
        
        // 2.  Palette Block
        if(phdr->nPalleteLines){
            pPalleteBlock = (char *)calloc( phdr->nPalleteLines,  PALLETE_LINE_SIZE);
            if(!Read(pPalleteBlock, phdr->nPalleteLines * PALLETE_LINE_SIZE).IsOk()){
                strncpy(err, "Load:  READ error pal", sizeof(err));
                return false;
            }
        }

        for(int i=0 ; i < phdr->nPalleteLines ; i++){
            char *pl = &pPalleteBlock[i * PALLETE_LINE_SIZE];
            int yyp = 4;
        //              printf("Offset %d:   %d\n", i, pline_table[i]);
        }
        
        // 3.  Ref Block
        if(phdr->nRefLines){
            pRefBlock = (char *)calloc( phdr->nRefLines,  REF_LINE_SIZE);
            if(!Read(pRefBlock, phdr->nRefLines * REF_LINE_SIZE).IsOk()){
                strncpy(err, "Load:  READ error ref", sizeof(err));
                return false;
            }
        }
        
        // 4.  Ply Block
        if(phdr->nPlyLines){
            pPlyBlock = (char *)calloc( phdr->nPlyLines,  PLY_LINE_SIZE);
            if(!Read(pPlyBlock, phdr->nPlyLines * PLY_LINE_SIZE).IsOk()){
                strncpy(err, "Load:  READ error ply", sizeof(err));
                return false;
            }
        }
        
        // 5.  Line offset Block
        pline_table = NULL;
        pline_table = (int *)malloc((phdr->Size_Y+1) * sizeof(int) );               
        if(!pline_table){
            strncpy(err, "Load:  cannot allocate memory, pline", sizeof(err));
            return false;
        }
        if(!Read(pline_table, (phdr->Size_Y+1) * sizeof(int)).IsOk()){
            strncpy(err, "Load:  READ error off", sizeof(err));
            return false;
        }
        
//          for(int i=0 ; i < phdr->Size_Y+1 ; i++)
//              printf("Offset %d:   %d\n", i, pline_table[i]);
        
#endif

        return true;
    }
    
    return false;
}


bool oernc_inStream::SendServerCommand( unsigned char cmd )
{
        fifo_msg msg;
        
        strncpy(msg.fifo_name, privatefifo_name, sizeof(msg.fifo_name));
        
        wxCharBuffer buf = m_fileName.ToUTF8();
        if(buf.data()) 
            strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
        else
            strncpy(msg.file_name, "?", sizeof(msg.file_name));
        
        
        buf = m_cryptoKey.ToUTF8();
        if(buf.data()) 
            strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
        else
            strncpy(msg.crypto_key, "??", sizeof(msg.crypto_key));
        
        msg.cmd = cmd;
        
        write(publicfifo, (char*) &msg, sizeof(msg));
        
        // Open the private FIFO for reading to get output of command
        // from the server.
        if((privatefifo = open(privatefifo_name, O_RDONLY) ) == -1) {
            wxLogMessage(_T("ofc_pi SendServerCommand: Could not open private pipe"));
            return false;
        }
        
        return true;
}


bool oernc_inStream::IsOk()
{
    return m_OK;
}

bool oernc_inStream::Ok()
{
    return m_OK;
}


bool oernc_inStream::isAvailable(wxString user_key)
{
    if(g_debugLevel)printf("TestAvail\n");

    if(m_uncrypt_stream){
        return m_uncrypt_stream->IsOk();
    }
    else{
        if(!Open()){
            if(g_debugLevel)printf("TestAvail Open FAILED\n");
            return false;
        }

        if( SendServerCommand(CMD_TEST_AVAIL) ){
            if(g_debugLevel)printf("TestAvail Open OK\n");
            char response[8];
            memset( response, 0, 8);
            int nTry = 5;
            do{
                if( Read(response, 2).IsOk() ){
                    if(g_debugLevel)printf("TestAvail Response OK\n");
                    return( !strncmp(response, "OK", 2) );
                }
                
                if(g_debugLevel)printf("Sleep on TestAvail: %d\n", nTry);
                wxMilliSleep(100);
                nTry--;
            }while(nTry);
            
            return false;
        }
        else{
            if(g_debugLevel)printf("TestAvail Open Error\n");
            return false;
        }
    }
    
    return false;
}

wxString oernc_inStream::getHK()
{
    wxLogMessage(_T("hk0"));
    
    if(!Open()){
            if(g_debugLevel)printf("getHK Open FAILED\n");
            wxLogMessage(_T("hk1"));
            return wxEmptyString;
    }
        
    if( SendServerCommand(CMD_GET_HK) ){
            char response[9];
            memset( response, 0, 9);
            strcpy( response, "testresp"); 
            int nTry = 5;
            do{
                if( Read(response, 8).IsOk() ){
                    wxLogMessage(_T("hks") + wxString( response, wxConvUTF8 ));
                    return( wxString( response, wxConvUTF8 ));
                }
                
                if(g_debugLevel)printf("Sleep on getHK: %d\n", nTry);
                wxLogMessage(_T("hk2"));
                
                wxMilliSleep(100);
                nTry--;
            }while(nTry);
            
            if(g_debugLevel)printf("getHK Response Timeout nTry\n");
                             wxLogMessage(_T("hk3"));
            
            return wxEmptyString;
    }
    else{
        if(g_debugLevel)printf("getHK SendServer Error\n");
         wxLogMessage(_T("hk4"));
    }
    return wxEmptyString;
}



void oernc_inStream::Shutdown()
{
    if(!Open()){
        if(g_debugLevel)printf("Shutdown Open FAILED\n");
        return;
    }
    
    if(SendServerCommand(CMD_EXIT)) {
        char response[8];
        memset( response, 0, 8);
        Read(response, 3);
    }
}


oernc_inStream &oernc_inStream::Read(void *buffer, size_t size)
{
    #define READ_SIZE 64000;
    #define MAX_TRIES 5;
    if(!m_uncrypt_stream){
        size_t max_read = READ_SIZE;
        //    bool blk = fcntl(privatefifo, F_GETFL) & O_NONBLOCK;
        
        if( -1 != privatefifo){
            //            printf("           Read private fifo: %s, %d bytes\n", privatefifo_name, size);
            
            size_t remains = size;
            char *bufRun = (char *)buffer;
            size_t totalBytesRead = 0;
            int nLoop = MAX_TRIES;
            do{
                size_t bytes_to_read = wxMin(remains, max_read);
                
                size_t bytesRead = read(privatefifo, bufRun, bytes_to_read );
                
                // Server may not have opened the Write end of the FIFO yet
                if(bytesRead == 0){
                    //                    printf("miss %d %d %d\n", nLoop, bytes_to_read, size);
                    nLoop --;
                    wxMilliSleep(20);
                }
                else
                    nLoop = MAX_TRIES;
                
                remains -= bytesRead;
                bufRun += bytesRead;
                totalBytesRead += bytesRead;
            } while( (remains > 0) && (nLoop) );
            
            m_OK = (totalBytesRead == size);
//             if(!m_OK)
//                 int yyp = 4;
            m_lastBytesRead = totalBytesRead;
            m_lastBytesReq = size;
        }
        
        return *this;
    }
    else{
        if(m_uncrypt_stream->IsOk())
            m_uncrypt_stream->Read(buffer, size);
        m_OK = m_uncrypt_stream->IsOk();
        return *this;
    }
}

#endif   // linux/Mac

#if defined(__WXMSW__)   // Windows

//--------------------------------------------------------------------------
//      oernc_inStream implementation
//--------------------------------------------------------------------------

oernc_inStream::oernc_inStream()
{
    Init();
    
}

oernc_inStream::oernc_inStream( const wxString &file_name, const wxString &crypto_key, bool bHeaderOnly )
{
    Init();
    
    m_fileName = file_name;
    m_cryptoKey = crypto_key;
    
    m_OK = Open( );
    if(m_OK){
        if(!Load( bHeaderOnly )){
            printf("%s\n", err);
            m_OK = false;
        }
    }
}

oernc_inStream::~oernc_inStream()
{
    Close();
}

void oernc_inStream::Init()
{
    privatefifo = -1;
    publicfifo = -1;
    m_OK = false;
    m_lastBytesRead = 0;
    m_lastBytesReq = 0;
    m_uncrypt_stream = 0;
}


void oernc_inStream::Close()
{
    Init();             // In case it want to be used again
}

bool oernc_inStream::Open( )
{
    
    //printf("OPEN()\n");
    //wxLogMessage(_T("xtr1_pi: OPEN"));
    
#if 0    
    
    
    wxString tmp_file = wxFileName::CreateTempFileName( _T("") );
    wxCharBuffer bufn = tmp_file.ToUTF8();
    if(bufn.data()) 
        strncpy(privatefifo_name, bufn.data(), sizeof(privatefifo_name));
    
    // Create the private FIFO
        if(-1 == mkfifo(privatefifo_name, 0666)){
            if(g_debugLevel)printf("   mkfifo private failed: %s\n", privatefifo_name);
        }
        else{
            if(g_debugLevel)printf("   mkfifo OK: %s\n", privatefifo_name);
        }
        
        
        
        // Open the well known public FIFO for writing
        if( (publicfifo = open(PUBLIC, O_WRONLY | O_NDELAY) ) == -1) {
            wxLogMessage(_T("xtr1_pi: Could not open PUBLIC pipe"));
            return false;
            //if( errno == ENXIO )
        }
        
#endif 

    LPTSTR lpvMessage=TEXT("Default message from client."); 
    BOOL   fSuccess = FALSE; 
     
    wxString pipeName;
    if(g_pipeParm.Length())
        pipeName = _T("\\\\.\\pipe\\") + g_pipeParm;
    else
        pipeName = _T("\\\\.\\pipe\\myoerncpipe");
    
    LPCWSTR lpszPipename = pipeName.wc_str();

    while (1) 
    { 
            hPipe = CreateFile( 
            lpszPipename,   // pipe name 
            GENERIC_READ |  // read and write access 
            GENERIC_WRITE, 
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 
            
            // Break if the pipe handle is valid. 
            
            if (hPipe != INVALID_HANDLE_VALUE) 
                break; 
            
            // Exit if an error other than ERROR_PIPE_BUSY occurs. 

            int err = GetLastError();
                
            if (err != ERROR_PIPE_BUSY) {
                _tprintf( TEXT("Could not open pipe. GLE=%d\n"), err ); 
                return false;
            }
                
                // All pipe instances are busy, so wait for 20 seconds. 
                
            if ( ! WaitNamedPipe(lpszPipename, 20000)) 
            { 
                _tprintf( TEXT("Could not open pipe: 20 second wait timed out.") ); 
                //printf("Could not open pipe: 20 second wait timed out."); 
                return false;
            } 
    }
 
    return true;
}

bool oernc_inStream::readPayload( unsigned char *p )
{
    if(!Read(p, m_lenIDat).IsOk()){
        strncpy(err, "Load:  READ error Payload1", sizeof(err));
        return false;
    }
    return true;
}

bool oernc_inStream::Load( bool bHeaderOnly )
{ 
    //printf("LOAD()\n");
    //wxLogMessage(_T("ofc_pi: LOAD"));
    
    if(m_cryptoKey.Length() && m_fileName.length()){
     
        fifo_msg msg;
        //  Build a message for the public pipe
        
        wxCharBuffer buf = m_fileName.ToUTF8();
        if(buf.data()) 
            strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
        
        strncpy(msg.fifo_name, privatefifo_name, sizeof(privatefifo_name));
                
        buf = m_cryptoKey.ToUTF8();
        int lenc = strlen(buf.data());
        if(buf.data()) 
            strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
                
        msg.cmd = CMD_OPEN_RNC_FULL;
        if(bHeaderOnly)
            msg.cmd =CMD_OPEN_RNC;
        
        SendServerCommand(&msg);

        // Read the function return code
        char frcbuf[4];
        if(!Read(frcbuf, 1).IsOk()){
            strncpy(err, "Load:  READ error PFC", sizeof(err));
            return false;
        }
        if(frcbuf[0] == '1'){
            strncpy(err, "Load:  READ error PFCDC", sizeof(err));
            return false;
        }

        // Read response, by steps...
        // 1.  The composite length string
        char lbuf[100];
        
        if(!Read(lbuf, 41).IsOk()){
            strncpy(err, "Load:  READ error PL", sizeof(err));
            return false;
        }
        int lp1, lp2, lp3, lp4, lp5, lpl;
        sscanf(lbuf, "%d;%d;%d;%d;%d;%d;", &lp1, &lp2, &lp3, &lp4, &lp5, &lpl);
        m_lenIDat = lpl;
        
        int maxLen = wxMax(lp1, lp2); 
        maxLen = wxMax(maxLen, lp3);
        maxLen = wxMax(maxLen, lp4);
        maxLen = wxMax(maxLen, lp5);
        char *work = (char *)calloc(maxLen+1, sizeof(char));
        
        // 5 strings
        // 1
        if(!Read(work, lp1).IsOk()){
            strncpy(err, "Load:  READ error P1", sizeof(err));
            return false;
        }
        work[lp1] = 0;
        m_ep1 =std::string(work);
        
        // 2
        if(!Read(work, lp2).IsOk()){
            strncpy(err, "Load:  READ error P2", sizeof(err));
            return false;
        }
        work[lp2] = 0;
        m_ep2 =std::string(work);
        
        // 3
        if(!Read(work, lp3).IsOk()){
            strncpy(err, "Load:  READ error P3", sizeof(err));
            return false;
        }
        work[lp3] = 0;
        m_ep3 =std::string(work);
        
        // 4
        if(!Read(work, lp4).IsOk()){
            strncpy(err, "Load:  READ error P4", sizeof(err));
            return false;
        }
        work[lp4] = 0;
        m_ep4 =std::string(work);
        
        // 5
        if(!Read(work, lp5).IsOk()){
            strncpy(err, "Load:  READ error P5", sizeof(err));
            return false;
        }
        work[lp5] = 0;
        m_ep5 =std::string(work);
        
        free(work);
        
        return true;
    }
    
    return false;
}



#if 0
bool oernc_inStream::Load( bool bHeaderOnly )
{ 
    //printf("LOAD()\n");
    //wxLogMessage(_T("ofc_pi: LOAD"));
    

        //  Build a message 
        fifo_msg msg;
        
        wxCharBuffer buf = m_fileName.ToUTF8();
        if(buf.data()) 
            strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
        
        strncpy(msg.fifo_name, privatefifo_name, sizeof(privatefifo_name));
        
        buf = m_cryptoKey.ToUTF8();
        if(buf.data()) 
            strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
        
        //msg.cmd = CMD_OPEN;

        SendServerCommand(&msg);
        
        // Read response, by steps...
 
#if 0        
        // 1.  Compressed Header
        if(NULL == phdr){
            phdr = (compressedHeader *)calloc(1, sizeof(compressedHeader));
            if(NULL == phdr){
                strncpy(err, "Load:  cannot allocate memory", sizeof(err));
                return false;
            }
        }
        if(!Read(phdr, sizeof(compressedHeader)).IsOk()){
            strncpy(err, "Load:  READ error chdr", sizeof(err));
            return false;
        }
        
        // 2.  Palette Block
        if(phdr->nPalleteLines){
            pPalleteBlock = (char *)calloc( phdr->nPalleteLines,  PALLETE_LINE_SIZE);
            if(!Read(pPalleteBlock, phdr->nPalleteLines * PALLETE_LINE_SIZE).IsOk()){
                strncpy(err, "Load:  READ error pal", sizeof(err));
                return false;
            }
        }
        
        for(int i=0 ; i < phdr->nPalleteLines ; i++){
            char *pl = &pPalleteBlock[i * PALLETE_LINE_SIZE];
            int yyp = 4;
            //              printf("Offset %d:   %d\n", i, pline_table[i]);
        }
        
        // 3.  Ref Block
        if(phdr->nRefLines){
            pRefBlock = (char *)calloc( phdr->nRefLines,  REF_LINE_SIZE);
            if(!Read(pRefBlock, phdr->nRefLines * REF_LINE_SIZE).IsOk()){
                strncpy(err, "Load:  READ error ref", sizeof(err));
                return false;
            }
        }
        
        // 4.  Ply Block
        if(phdr->nPlyLines){
            pPlyBlock = (char *)calloc( phdr->nPlyLines,  PLY_LINE_SIZE);
            if(!Read(pPlyBlock, phdr->nPlyLines * PLY_LINE_SIZE).IsOk()){
                strncpy(err, "Load:  READ error ply", sizeof(err));
                return false;
            }
        }
        
        // 5.  Line offset Block
        pline_table = NULL;
        pline_table = (int *)malloc((phdr->Size_Y+1) * sizeof(int) );               
        if(!pline_table){
            strncpy(err, "Load:  cannot allocate memory, pline", sizeof(err));
            return false;
        }
        if(!Read(pline_table, (phdr->Size_Y+1) * sizeof(int)).IsOk()){
            strncpy(err, "Load:  READ error off", sizeof(err));
            return false;
        }
        
        //          for(int i=0 ; i < phdr->Size_Y+1 ; i++)
        //              printf("Offset %d:   %d\n", i, pline_table[i]);
        
#endif        
        return true;
}
#endif

bool oernc_inStream::SendServerCommand( unsigned char cmd )
{
    
    fifo_msg msg;
    
    strncpy(msg.fifo_name, privatefifo_name, sizeof(msg.fifo_name));
    
    wxCharBuffer buf = m_fileName.ToUTF8();
    if(buf.data()) 
        strncpy(msg.file_name, buf.data(), sizeof(msg.file_name));
    else
        strncpy(msg.file_name, "?", sizeof(msg.file_name));
    
    
    buf = m_cryptoKey.ToUTF8();
    if(buf.data()) 
        strncpy(msg.crypto_key, buf.data(), sizeof(msg.crypto_key));
    else
        strncpy(msg.crypto_key, "??", sizeof(msg.crypto_key));
    
    msg.cmd = cmd;

#if 0    
    write(publicfifo, (char*) &msg, sizeof(msg));
    
    // Open the private FIFO for reading to get output of command
    // from the server.
    if((privatefifo = open(privatefifo_name, O_RDONLY) ) == -1) {
        wxLogMessage(_T("xtr1_pi SendServerCommand: Could not open private pipe"));
        return false;
    }
#endif 

    
    // Send the command message to the pipe server. 
    DWORD  cbToWrite, cbWritten; 
    BOOL   fSuccess = FALSE; 
    
    cbToWrite = sizeof(msg);
    _tprintf( TEXT("Sending %d byte message \n"), cbToWrite); 
    
    fSuccess = WriteFile( 
        hPipe,                  // pipe handle 
        &msg,                   // message 
        cbToWrite,              // message length 
        &cbWritten,             // bytes written 
        NULL);                  // not overlapped 
    
    if ( ! fSuccess) 
    {
        _tprintf( TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError() ); 
        return false;
    }
    
    printf("\nMessage sent to server.\n");
    
               
    return true;
}

bool oernc_inStream::SendServerCommand( fifo_msg *pmsg )
{
    
#if 0    
    write(publicfifo, (char*) &msg, sizeof(msg));
    
    // Open the private FIFO for reading to get output of command
    // from the server.
    if((privatefifo = open(privatefifo_name, O_RDONLY) ) == -1) {
        wxLogMessage(_T("xtr1_pi SendServerCommand: Could not open private pipe"));
        return false;
    }
#endif 

    
    // Send the command message to the pipe server. 
    DWORD  cbToWrite, cbWritten; 
    BOOL   fSuccess = FALSE; 
    
    cbToWrite = sizeof(*pmsg);
    _tprintf( TEXT("Sending %d byte message \n"), cbToWrite); 
    
    fSuccess = WriteFile( 
        hPipe,                  // pipe handle 
        pmsg,                   // message 
        cbToWrite,              // message length 
        &cbWritten,             // bytes written 
        NULL);                  // not overlapped 
    
    if ( ! fSuccess) 
    {
        _tprintf( TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError() ); 
        return false;
    }
    
    printf("\nMessage sent to server.\n");
    
               
    return true;
}



bool oernc_inStream::IsOk()
{
    return m_OK;
}

bool oernc_inStream::Ok()
{
    return m_OK;
}


bool oernc_inStream::isAvailable(wxString user_key)
{
#if 0    
    if(g_debugLevel)printf("TestAvail\n");
                        
                        if(m_uncrypt_stream){
                            return m_uncrypt_stream->IsOk();
                        }
                        else{
                            if(!Open()){
                                if(g_debugLevel)printf("TestAvail Open FAILED\n");
                        return false;
                            }
                            
                            if( SendServerCommand(CMD_TEST_AVAIL) ){
                                if(g_debugLevel)printf("TestAvail Open OK\n");
                        char response[8];
                        memset( response, 0, 8);
                        int nTry = 5;
                        do{
                            if( Read(response, 2).IsOk() ){
                                if(g_debugLevel)printf("TestAvail Response OK\n");
                        return( !strncmp(response, "OK", 2) );
                            }
                            
                            if(g_debugLevel)printf("Sleep on TestAvail: %d\n", nTry);
                        wxMilliSleep(100);
                        nTry--;
                        }while(nTry);
                        
                        return false;
                            }
                            else{
                                if(g_debugLevel)printf("TestAvail Open Error\n");
                        return false;
                            }
                        }
#endif 

                        if (Open()){

                            if (SendServerCommand(CMD_TEST_AVAIL)){
                                char response[8];
                                memset(response, 0, 8);
                                int nTry = 5;

                                do{
                                    if (Read(response, 2).IsOk()){
                                        if (strncmp(response, "OK", 2))
                                            return false;
                                        else
                                            return true;
                                    }

                                    wxMilliSleep(100);
                                    nTry--;
                                } while (nTry);

                                return false;
                            }
                        }
    else{
        if(g_debugLevel) wxLogMessage(_T("instream OPEN() failed"));
        return false;
    }
    


    return false;
                        
}

wxString oernc_inStream::getHK()
{
    wxLogMessage(_T("hk0"));
    
    if(!Open()){
        if(g_debugLevel)printf("getHK Open FAILED\n");
                               wxLogMessage(_T("hk1"));
        return wxEmptyString;
    }
#if 1    
    
    if( SendServerCommand(CMD_GET_HK) ){
        char response[9];
        memset( response, 0, 9);
        strcpy( response, "testresp"); 
        int nTry = 5;
        do{
            if( Read(response, 8).IsOk() ){
                //wxLogMessage(_T("hks") + wxString( response, wxConvUTF8 ));
                return( wxString( response, wxConvUTF8 ));
            }
            
            if(g_debugLevel)printf("Sleep on getHK: %d\n", nTry);
                             //wxLogMessage(_T("hk2"));
            
            wxMilliSleep(100);
            nTry--;
        }while(nTry);
        
        if(g_debugLevel)printf("getHK Response Timeout nTry\n");
                               //wxLogMessage(_T("hk3"));
        
        return wxEmptyString;
    }
    else{
        if(g_debugLevel)printf("getHK SendServer Error\n");
                             //wxLogMessage(_T("hk4"));
    }
#endif    
    return wxEmptyString;
}



void oernc_inStream::Shutdown()
{
    if(!Open()){
        if(g_debugLevel)printf("Shutdown Open FAILED\n");
        return;
    }
 
    if(SendServerCommand(CMD_EXIT)) {
        char response[8];
        memset( response, 0, 8);
        Read(response, 3);
    }
}


oernc_inStream &oernc_inStream::Read(void *buffer, size_t size)
{
#if 0    
    #define READ_SIZE 64000;
    #define MAX_TRIES 100;
    if(!m_uncrypt_stream){
        size_t max_read = READ_SIZE;
        //    bool blk = fcntl(privatefifo, F_GETFL) & O_NONBLOCK;
        
        if( -1 != privatefifo){
            //            printf("           Read private fifo: %s, %d bytes\n", privatefifo_name, size);
            
            size_t remains = size;
            char *bufRun = (char *)buffer;
            size_t totalBytesRead = 0;
            int nLoop = MAX_TRIES;
            do{
                size_t bytes_to_read = wxMin(remains, max_read);
                
                size_t bytesRead = read(privatefifo, bufRun, bytes_to_read );
                
                // Server may not have opened the Write end of the FIFO yet
                if(bytesRead == 0){
                    //                    printf("miss %d %d %d\n", nLoop, bytes_to_read, size);
                    nLoop --;
                    wxMilliSleep(100);
                }
                else
                    nLoop = MAX_TRIES;
                
                remains -= bytesRead;
                bufRun += bytesRead;
                totalBytesRead += bytesRead;
            } while( (remains > 0) && (nLoop) );
            
            m_OK = (totalBytesRead == size);
            //             if(!m_OK)
            //                 int yyp = 4;
            m_lastBytesRead = totalBytesRead;
            m_lastBytesReq = size;
        }
        
        return *this;
    }
    else{
        if(m_uncrypt_stream->IsOk())
            m_uncrypt_stream->Read(buffer, size);
        m_OK = m_uncrypt_stream->IsOk();
        return *this;
    }
#endif

    #define READ_SIZE 64000;
    size_t max_read = READ_SIZE;
    
    if(size > max_read)
        int yyp = 4;
    
    if( (HANDLE)-1 != hPipe){
        size_t remains = size;
        char *bufRun = (char *)buffer;
        int totalBytesRead = 0;
        DWORD bytesRead;
        BOOL fSuccess = FALSE;
        
        do{
            int bytes_to_read = wxMin(remains, max_read);
            
            // Read from the pipe. 
            
            fSuccess = ReadFile( 
                hPipe,                  // pipe handle 
                bufRun,                 // buffer to receive reply 
                bytes_to_read,          // size of buffer 
                &bytesRead,             // number of bytes read 
                NULL);                  // not overlapped 
            
            if ( ! fSuccess && GetLastError() != ERROR_MORE_DATA )
                break; 
            
            //_tprintf( TEXT("\"%s\"\n"), chBuf ); 
            
            if(bytesRead == 0){
                int yyp = 3;
                remains = 0;    // break it
            }
            
            remains -= bytesRead;
            bufRun += bytesRead;
            totalBytesRead += bytesRead;
        } while(remains > 0);

        
        m_OK = (totalBytesRead == size) && fSuccess;
        m_lastBytesRead = totalBytesRead;
        m_lastBytesReq = size;
        
    }
    else
        m_OK = false;

    return *this;
}


#endif          // MSW

            







