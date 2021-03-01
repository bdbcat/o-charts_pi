/******************************************************************************
 *
 * Project:
 * Purpose:
 * Author:   David Register
 *
*/
// ============================================================================
// declarations
// ============================================================================


// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers


//  Why are these not in wx/prec.h?
#include "wx/dir.h"
#include "wx/stream.h"
#include "wx/wfstream.h"
#include "wx/tokenzr.h"
#include "wx/filename.h"
#include <wx/image.h>
#include <wx/dynlib.h>
#include <wx/textfile.h>

#include <sys/stat.h>

//#include "oernc_pi.h"
#include "chart.h"
#include "oernc_inStream.h"
#include <zlib.h>

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif

#ifdef __OCPN__ANDROID__
#include "qdebug.h"
#endif

extern bool processChartinfo(const wxString &oesenc_file);
extern void showChartinfoDialog( void );

// ----------------------------------------------------------------------------
// Random Prototypes
// ----------------------------------------------------------------------------
extern bool validate_SENC_server( void );


typedef struct  {
      float y;
      float x;
} MyFlPoint;

float hex2float( std::string h)
{
    int val;
    sscanf(h.c_str(), "%x", &val);
    
    float *pf = (float *)&val;
    return *pf;
}


// Static methods belonging to upstream plugin
wxString getPrimaryKey(wxString file);
wxString getAlternateKey(wxString file);
void SwapKeyHashes();


#ifdef __WXGTK__
class OCPNStopWatch
{
public:
    OCPNStopWatch() { Reset(); }
    void Reset() { clock_gettime(CLOCK_REALTIME, &tp); }
    
    double GetTime() {
        timespec tp_end;
        clock_gettime(CLOCK_REALTIME, &tp_end);
        return (tp_end.tv_sec - tp.tv_sec) * 1.e3 + (tp_end.tv_nsec - tp.tv_nsec) / 1.e6;
    }
    
private:
    timespec tp;
};
#endif

//  Private version of PolyPt testing using floats instead of doubles
bool Intersect(MyFlPoint p1, MyFlPoint p2, MyFlPoint p3, MyFlPoint p4) ;
int CCW(MyFlPoint p0, MyFlPoint p1, MyFlPoint p2) ;

/*************************************************************************


 * FUNCTION:   G_FloatPtInPolygon
 *
 * PURPOSE
 * This routine determines if the point passed is in the polygon. It uses

 * the classical polygon hit-testing algorithm: a horizontal ray starting

 * at the point is extended infinitely rightwards and the number of
 * polygon edges that intersect the ray are counted. If the number is odd,
 * the point is inside the polygon.
 *
 * RETURN VALUE
 * (bool) TRUE if the point is inside the polygon, FALSE if not.
 *************************************************************************/


bool G_FloatPtInPolygon(MyFlPoint *rgpts, int wnumpts, float x, float y)

{

   MyFlPoint  *ppt, *ppt1 ;
   int   i ;
   MyFlPoint  pt1, pt2, pt0 ;
   int   wnumintsct = 0 ;

   pt0.x = x;
   pt0.y = y;

   pt1 = pt2 = pt0 ;
   pt2.x = 1.e6;

   // Now go through each of the lines in the polygon and see if it
   // intersects
   for (i = 0, ppt = rgpts ; i < wnumpts-1 ; i++, ppt++)
   {
      ppt1 = ppt;
        ppt1++;
        if (Intersect(pt0, pt2, *ppt, *(ppt1)))
         wnumintsct++ ;
   }

   // And the last line
   if (Intersect(pt0, pt2, *ppt, *rgpts))
      wnumintsct++ ;

//   return(wnumintsct&1);

   //       If result is false, check the degenerate case where test point lies on a polygon endpoint
   if(!(wnumintsct&1))
   {
         for (i = 0, ppt = rgpts ; i < wnumpts ; i++, ppt++)
         {
               if(((*ppt).x == x) && ((*ppt).y == y))
                     return true;
         }
   }
   else
       return true;

   return false;
}


/*************************************************************************


 * FUNCTION:   Intersect
 *
 * PURPOSE
 * Given two line segments, determine if they intersect.
 *
 * RETURN VALUE
 * TRUE if they intersect, FALSE if not.
 *************************************************************************/


inline bool Intersect(MyFlPoint p1, MyFlPoint p2, MyFlPoint p3, MyFlPoint p4) {
   return ((( CCW(p1, p2, p3) * CCW(p1, p2, p4)) <= 0)
        && (( CCW(p3, p4, p1) * CCW(p3, p4, p2)  <= 0) )) ;

}
/*************************************************************************


 * FUNCTION:   CCW (CounterClockWise)
 *
 * PURPOSE
 * Determines, given three points, if when travelling from the first to
 * the second to the third, we travel in a counterclockwise direction.
 *
 * RETURN VALUE
 * (int) 1 if the movement is in a counterclockwise direction, -1 if
 * not.
 *************************************************************************/


inline int CCW(MyFlPoint p0, MyFlPoint p1, MyFlPoint p2) {
   float dx1, dx2 ;
   float dy1, dy2 ;

   dx1 = p1.x - p0.x ; dx2 = p2.x - p0.x ;
   dy1 = p1.y - p0.y ; dy2 = p2.y - p0.y ;

   /* This is basically a slope comparison: we don't do divisions because
    * of divide by zero possibilities with pure horizontal and pure
    * vertical lines.
    */
   return ((dx1 * dy2 > dy1 * dx2) ? 1 : -1) ;

}




// ============================================================================
// PixelCache Implementation
// ============================================================================
PIPixelCache::PIPixelCache(int width, int height, int depth)
{
    m_width = width;
    m_height = height;
    m_depth = depth;
    m_pbm = NULL;
    m_rgbo = RGB;                        // default value;
    pData = NULL;

    line_pitch_bytes =
            bytes_per_pixel = BPP / 8;
    line_pitch_bytes = bytes_per_pixel * width;


#ifdef ocpnUSE_ocpnBitmap
      m_rgbo = BGR;
#endif

#ifdef __PIX_CACHE_PIXBUF__
      m_rgbo = RGB;
#endif

#ifdef __PIX_CACHE_DIBSECTION__
      m_pDS = new wxDIB(width, -height, BPP);
      pData = m_pDS->GetData();
#endif


#ifdef __PIX_CACHE_WXIMAGE__
      m_pimage = new wxImage(m_width, m_height, (bool)FALSE);
      pData = m_pimage->GetData();
#endif

#ifdef __PIX_CACHE_X11IMAGE__
      m_pocpnXI = new ocpnXImage(width, height);
      pData = (unsigned char *)m_pocpnXI->m_img->data;
#endif            //__PIX_CACHE_X11IMAGE__

#ifdef __PIX_CACHE_PIXBUF__
//      m_pbm = new ocpnBitmap((unsigned char *)NULL, m_width, m_height, m_depth);

///      m_pbm = new ocpnBitmap();
///      m_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB,
///                                           1,
///                                           8, m_width, m_height);

///      m_pixbuf = m_pbm->GetPixbuf();
///      pData = gdk_pixbuf_get_pixels(m_pixbuf);
///      m_pbm->SetPixbuf(m_pixbuf, 32);

      pData = (unsigned char *)malloc(m_width * m_height * 4);
///      memset(pData, 255, m_width * m_height * 4);       // set alpha channel to 1
#endif
}

PIPixelCache::~PIPixelCache()
{
#ifdef __PIX_CACHE_WXIMAGE__
      delete m_pimage;
      delete m_pbm;
#endif


#ifdef __PIX_CACHE_DIBSECTION__
      delete m_pDS;
#endif

#ifdef __PIX_CACHE_X11IMAGE__
      delete m_pbm;
      delete m_pocpnXI;
#endif

#ifdef __PIX_CACHE_PIXBUF__
      free(pData);
      delete m_pbm;
#endif

}

void PIPixelCache::BuildBM()
{
      if(!m_pbm)
            m_pbm = new wxBitmap (*m_pimage, -1);
}



void PIPixelCache::Update(void)
{
#ifdef __PIX_CACHE_WXIMAGE__
    delete m_pbm;                       // kill the old one
    m_pbm = NULL;
#endif
}


void PIPixelCache::SelectIntoDC(wxMemoryDC &dc)
{

#ifdef __PIX_CACHE_DIBSECTION__
      PIocpnMemDC *pmdc = dynamic_cast<PIocpnMemDC*>(&dc);
      pmdc->SelectObject (*m_pDS);

#endif      //__PIX_CACHE_DIBSECTION__


#ifdef __PIX_CACHE_WXIMAGE__
//    delete m_pbm;                       // kill the old one

      //    Convert image to bitmap
#ifdef ocpnUSE_ocpnBitmap
      if(!m_pbm)
            m_pbm = new PIocpnBitmap(*m_pimage, m_depth);
#else
      if(!m_pbm)
            m_pbm = new wxBitmap (*m_pimage, -1);
#endif

      if(m_pbm)
            dc.SelectObject(*m_pbm);
#endif            // __PIX_CACHE_WXIMAGE__


#ifdef __PIX_CACHE_X11IMAGE__
      if(!m_pbm)
        m_pbm = new PIocpnBitmap(m_pocpnXI, m_width, m_height, m_depth);
      dc.SelectObject(*m_pbm);
#endif            //__PIX_CACHE_X11IMAGE__

#ifdef __PIX_CACHE_PIXBUF__
      if(!m_pbm)
            m_pbm = new PIocpnBitmap(pData, m_width, m_height, m_depth);
      if(m_pbm)
      {
          dc.SelectObject(*m_pbm);
      }
#endif          //__PIX_CACHE_PIXBUF__


}


unsigned char *PIPixelCache::GetpData(void) const
{
    return pData;
}





// ============================================================================
// ThumbData implementation
// ============================================================================

ThumbData::ThumbData()
{
    pDIBThumb = NULL;
}

ThumbData::~ThumbData()
{
    delete pDIBThumb;
}

// ============================================================================
// Palette implementation
// ============================================================================
opncpnPalette::opncpnPalette()
{
    // Index into palette is 1-based, so predefine the first entry as null
    nFwd = 1;
    nRev = 1;
    FwdPalette =(int *)malloc( sizeof(int));
    RevPalette =(int *)malloc( sizeof(int));
    FwdPalette[0] = 0;
    RevPalette[0] = 0;
}

opncpnPalette::~opncpnPalette()
{
    if(NULL != FwdPalette)
        free( FwdPalette );
    if(NULL != RevPalette)
        free( RevPalette ) ;
}


// callback function to authorize your software key.

void nvc_dll_authcheck(unsigned long *Data)
{
  // replace this entry with the provided key
//  unsigned long Key[]=  { 0,0,0,0x77034680 };
  unsigned long Key[] = { 0x0cd9469e,0x657f194c,0x1d952eaa,0x5a9b7e38 };

  // xtea cypher
  unsigned long delta, sum;
  short    cnt;
  sum   = 0;
  delta = 0x9E3779B9;
  cnt   = 32;
  while(cnt-- > 0) {
    Data[0] += ((Data[1]<<4 ^ Data[1]>>5) + Data[1]) ^ (sum + Key[sum&3]);
    sum += delta;
    Data[1] += ((Data[0]<<4 ^ Data[0]>>5) + Data[0]) ^ (sum + Key[sum>>11 & 3]);
  }
}



// ----------------------------------------------------------------------------
// Chart_oeuRNC Implementation
// ----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(Chart_oeuRNC, PlugInChartBase)


Chart_oeuRNC::Chart_oeuRNC()
{
      ChartBaseBSBCTOR();

      m_depth_unit_id = PI_DEPTH_UNIT_UNKNOWN;

      m_global_color_scheme = PI_GLOBAL_COLOR_SCHEME_RGB;

      m_bReadyToRender = false;

      m_Chart_Error_Factor = 0.;

      m_Chart_Scale = 10000;              // a benign value

      m_nCOVREntries = 0;
      m_pCOVRTable = NULL;
      m_pCOVRTablePoints = NULL;

      m_EdDate.Set(1, wxDateTime::Jan, 2000);

      m_lon_datum_adjust = 0.;
      m_lat_datum_adjust = 0.;

      m_projection = PI_PROJECTION_MERCATOR;             // default

      m_ChartType = PI_CHART_TYPE_PLUGIN;
      m_ChartFamily = PI_CHART_FAMILY_RASTER;

      m_pBMPThumb = NULL;
      m_nColors = 0;
      m_imageMap = NULL;
      m_bImageReady = false;
 
}

Chart_oeuRNC::~Chart_oeuRNC()
{

      //    Free the COVR tables

      for(unsigned int j=0 ; j<(unsigned int)m_nCOVREntries ; j++)
            free( m_pCOVRTable[j] );

      free( m_pCOVRTable );
      free( m_pCOVRTablePoints );

      delete m_pBMPThumb;

      free( m_imageMap );
      
      ChartBaseBSBDTOR();
}

void Chart_oeuRNC::FreeLineCacheRows(int start, int end)
{
    if(pLineCache)
    {
        if(end < 0)
            end = Size_Y;
        else
            end = wxMin(end, Size_Y);
        for(int ylc = start ; ylc < end ; ylc++) {
            CachedLine *pt = &pLineCache[ylc];
            if(pt->bValid) {
                free (pt->pTileOffset);
                free (pt->pPix);
                pt->bValid = false;
            }
        }
    }
}


#define BUF_LEN_MAX 4000

bool gs_binit_msg_shown;

int Chart_oeuRNC::Init( const wxString& name, int init_flags )
{
    
      int nPlypoint = 0;
      Plypoint *pPlyTable = (Plypoint *)malloc(sizeof(Plypoint));

      Size_X = Size_Y = 0;
      
      PreInit(name, init_flags, PI_GLOBAL_COLOR_SCHEME_DAY);

      m_FullPath = name;
      m_Description = m_FullPath;
      
      if(!::wxFileExists(name)){
          wxString msg(_T("   OERNC_PI: chart file not found: "));
          msg.Append(m_FullPath);
          wxLogMessage(msg);
          
          return INIT_FAIL_REMOVE;
      }
      
      if(!processChartinfo( name )){
        return PI_INIT_FAIL_REMOVE;
      }
      
      if(init_flags != HEADER_ONLY)
        showChartinfoDialog();

      validate_SENC_server();
      

      wxString key = wxString(getPrimaryKey(name));
      if(!key.Len()){
          key = wxString(getAlternateKey(name));
          if(!key.Len()){
            wxString msg(_T("   OERNC_PI: chart RInstallKey not found: "));
            msg.Append(m_FullPath);
            wxLogMessage(msg);
          
            return INIT_FAIL_REMOVE;
          }
          SwapKeyHashes();          // interchanges hashes, so next time will be faster
      }

      bool bHeaderOnly = false;
      if(init_flags == HEADER_ONLY)
          bHeaderOnly = true;

      ifs_hdr = new oernc_inStream(name, key, bHeaderOnly);          // open the file server

      if(!ifs_hdr->Ok()){
          wxString msg(_T("   OERNC_PI: chart local server error, retrying: "));
          msg.Append(m_FullPath);
          wxLogMessage(msg);

          SwapKeyHashes();          // interchanges hashes, so next time will be faster

          key = wxString(getPrimaryKey(name));

          validate_SENC_server();
 
          ifs_hdr = new oernc_inStream(name, key, bHeaderOnly);          // open the file server
          if(!ifs_hdr->Ok()){
              wxString msg(_T("   OERNC_PI: chart local server error, final: "));
              msg.Append(m_FullPath);
              wxLogMessage(msg);
              return INIT_FAIL_REMOVE;
          }
      }


      //  Basic parameters (hd)
      long tmp;
      wxString hd(ifs_hdr->m_ep2.c_str());
      wxStringTokenizer tkz(hd, _T(";"));
      
      wxString token = tkz.GetNextToken();      //Size_X
      token.ToLong( &tmp);
      Size_X = tmp;
      token = tkz.GetNextToken();      //Size_Y
      token.ToLong( &tmp);
      Size_Y= tmp;
      
      token = tkz.GetNextToken();      //ID
      m_ID = token;
      
      token = tkz.GetNextToken();      //Projection
      token.ToLong( &tmp);
      m_projection = (OcpnProjTypePI)tmp;
      
      token = tkz.GetNextToken();      //DU
      token.ToLong( &tmp);
      m_Chart_DU = tmp;
      
      token = tkz.GetNextToken();      //Scale
      token.ToLong( &tmp);
      m_Chart_Scale = tmp; 
      
      double dtmp;
      token = tkz.GetNextToken();      //Skew
      token.ToDouble( &dtmp);
      m_Chart_Skew = dtmp; 
      
      token = tkz.GetNextToken();      //projection parameter
      token.ToDouble( &dtmp);
      m_proj_parameter = dtmp; 
      
      token = tkz.GetNextToken();      //m_dx
      token.ToDouble( &dtmp);
      m_dx = dtmp; 
      
      token = tkz.GetNextToken();      //m_dy
      token.ToDouble( &dtmp);
      m_dy = dtmp; 
      
      token = tkz.GetNextToken();      //Depth units
      m_DepthUnits = token;

      token = tkz.GetNextToken();      //Sounding datum
      m_SoundingsDatum = token;

      token = tkz.GetNextToken();      //Geo Datum
      m_datum_str = token;

      token = tkz.GetNextToken();      //Date
      char date_buf[16];
      date_buf[0] = 0;

      wxString date_wxstr  = token;
      wxDateTime dt;
      if(dt.ParseDate(date_wxstr)){       // successful parse?
        int iyear = dt.GetYear(); 
        
        if(iyear < 50){
            iyear += 2000;
            dt.SetYear(iyear);
        }
        else if((iyear >= 50) && (iyear < 100)){
            iyear += 1900;
            dt.SetYear(iyear);
        }
        sprintf(date_buf, "%d", iyear);

        //    Initialize the wxDateTime menber for Edition Date
        m_EdDate = dt;
      }
      else{
        m_EdDate.Set(1, wxDateTime::Jan, 2000);                    //Todo this could be smarter
      }

      m_PubYear = wxString(date_buf,  wxConvUTF8);

      token = tkz.GetNextToken();      //SE
      m_SE = token;


      //Check for bogus data
      
      if((Size_X <= 0) || (Size_Y <= 0))
      {
          wxString msg(_T("   OERNC_PI: chart header content error 2: "));
          msg.Append(m_FullPath);
          wxLogMessage(msg);
          
          return INIT_FAIL_REMOVE;
      }
      if(0 == m_Chart_Scale)
          m_Chart_Scale = 100000000;
      
      // Name, UTF8 aware
      wxString nas(ifs_hdr->m_ep3.c_str());
      unsigned char nab[200];
      int nl = nas.Length() / 2;
      long ntmp;
      for(int in=0 ; in < nl ; in++){
          wxString s = nas.Mid(in * 2, 2);
          s.ToLong(&ntmp, 16);
          nab[in] = ntmp & 0xFF;
      }
      nab[nl] = 0;
      m_Name = wxString(nab, wxConvUTF8);
      
    
      // Ply Points
      wxString plys(ifs_hdr->m_ep4.c_str());
      nPlypoint = plys.Length() / 16;
      pPlyTable = (Plypoint *)realloc(pPlyTable, sizeof(Plypoint) * (nPlypoint+1));
      for(int ip = 0 ; ip < nPlypoint ; ip++){
          wxString v1 = plys.Mid( ip*16, 8);
          float ltp = hex2float( std::string(v1.c_str()));
          wxString v2 = plys.Mid( (ip*16) + 8, 8);
          float lnp = hex2float( std::string(v2.c_str()));
          
          pPlyTable[ip].ltp = ltp;
          pPlyTable[ip].lnp = lnp;
      }
      
      // Ref Points
      wxString refs(ifs_hdr->m_ep1.c_str());
      nRefpoint =refs.Length() / 32;
      pRefTable = (Refpoint *)realloc(pRefTable, sizeof(Refpoint) * (nRefpoint+1));
      for(int ip = 0 ; ip < nRefpoint ; ip++){
          wxString v1 = refs.Mid( ip*32, 8);
          float xr = hex2float( std::string(v1.c_str()));
          wxString v2 = refs.Mid( (ip*32) + 8, 8);
          float yr = hex2float( std::string(v2.c_str()));
          wxString v3 = refs.Mid( (ip*32) + 16, 8);
          float ltr = hex2float( std::string(v3.c_str()));
          wxString v4 = refs.Mid( (ip*32) + 24, 8);
          float lnr = hex2float( std::string(v4.c_str()));

          pRefTable[ip].xr = xr;
          pRefTable[ip].yr = yr;
          pRefTable[ip].latr = ltr;
          pRefTable[ip].lonr = lnr;
          pRefTable[ip].bXValid = 1;
          pRefTable[ip].bYValid = 1;
      }
      
      // Palettes
      wxString pals(ifs_hdr->m_ep5.c_str());
      ///wxLogMessage(pals);
      wxStringTokenizer tkpz(pals, _T(";"));
      long ptmp;

      for(int i=0 ; i < N_BSB_COLORS ; i++){
        if(!pPalettes[i])
            pPalettes[i] = new opncpnPalette;
  
        opncpnPalette *pp = pPalettes[i];

        wxString token = tkpz.GetNextToken();      
        token.ToLong(&ptmp);            //  counter == i
        token = tkpz.GetNextToken();      
        token.ToLong(&ptmp);            //  m_nColors
        m_nColors = ptmp;
        pp->nFwd = m_nColors;
        pp->nRev = m_nColors;
        pp->FwdPalette = (int *)realloc(pp->FwdPalette, (pp->nFwd + 1) * sizeof(int));
        pp->RevPalette = (int *)realloc(pp->RevPalette, (pp->nRev + 1) * sizeof(int));
        
        for(int j=0 ; j < m_nColors ; j++){
            token = tkpz.GetNextToken();      
            token.ToLong(&ptmp, 16);            //  a color
            //wxString msg;
            //msg.Printf(_T("Pallette %d: nColors %d: Color %d: "), i, m_nColors, j);
            //if(i == 0) wxLogMessage(msg + token);
            pp->RevPalette[j] = ptmp;
            pp->FwdPalette[j] = ptmp;
        }
      }
     
     
        
          
 #if 0      
      
      
//      m_cph = pHeader->CPH;
//      m_dtm_lat = pHeader->DTMlat;
//      m_dtm_lon = pHeader->DTMlon;
      
#endif

    nColorSize = 3;
    
    
    
    //    Clear georeferencing coefficients
    for(int icl=0 ; icl< 12 ; icl++)
    {
        wpx[icl] = 0;
        wpy[icl] = 0;
        pwx[icl] = 0;
        pwy[icl] = 0;
    }
    
//     //    Set georeferencing coefficients
//     for(int icl=0 ; icl< 12 ; icl++)
//     {
//         wpx[icl] = pHeader->WPX[icl];
//         wpy[icl] = pHeader->WPY[icl];
//         pwx[icl] = pHeader->PWX[icl];
//         pwy[icl] = pHeader->PWY[icl];
//     }
    
 
   
    
 
//       ifss_bitmap = new wxFileInputStream(name); // Open again, as the bitmap
//       ifs_bitmap = new wxBufferedInputStream(*ifss_bitmap);


      //    If imbedded coefficients are found,
      //    then use the polynomial georeferencing algorithms
      if(pwx[0] && pwy[0] && wpx[0] && pwy[0])
          bHaveEmbeddedGeoref = true;


      //    Set up the projection point according to the projection parameter
      if(m_projection == PI_PROJECTION_MERCATOR)
            m_proj_lat = m_proj_parameter;
      else if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
            m_proj_lon = m_proj_parameter;
      else if(m_projection == PI_PROJECTION_POLYCONIC)
            m_proj_lon = m_proj_parameter;


//    Validate some of the header data

      if(nPlypoint < 3)
      {
            wxString msg(_T("   Chart File contains less than 3 PLY points: "));
            msg.Append(m_FullPath);
            wxLogMessage(msg);

            return INIT_FAIL_REMOVE;
      }


//    Convert captured plypoint information into chart COVR structures
      m_nCOVREntries = 1;
      m_pCOVRTablePoints = (int *)malloc(sizeof(int));
      *m_pCOVRTablePoints = nPlypoint;
      m_pCOVRTable = (float **)malloc(sizeof(float *));
      *m_pCOVRTable = (float *)malloc(nPlypoint * 2 * sizeof(float));
      memcpy(*m_pCOVRTable, pPlyTable, nPlypoint * 2 * sizeof(float));
      free(pPlyTable);


      //    Adjust the PLY points to WGS84 datum
      Plypoint *ppp = (Plypoint *)GetCOVRTableHead(0);
      int cnPlypoint = GetCOVRTablenPoints(0);

      for(int u=0 ; u<cnPlypoint ; u++)
      {
            ppp->lnp += m_dtm_lon / 3600;
            ppp->ltp += m_dtm_lat / 3600;
            ppp++;
      }


      if(!SetMinMax())
            return INIT_FAIL_REMOVE;          // have to bail here

      if(init_flags == HEADER_ONLY)
            return INIT_OK;

#ifdef __WXGTK__
      OCPNStopWatch sw;
#endif
      // Get the image payload
      if(ifs_hdr->m_lenIDat){
        m_imageComp = (unsigned char *)malloc(ifs_hdr->m_lenIDat);
        ifs_hdr->readPayload(m_imageComp);

        if(!ifs_hdr->Ok()){
            wxString msg(_T("   OERNC_PI: chart local server payload error, final: "));
            msg.Append(m_FullPath);
            wxLogMessage(msg);
            free (m_imageComp);
            return INIT_FAIL_REMOVE;
        }
        
//         int inflate_err = DecodeImage();
 
//         if(inflate_err){
//             wxString msg(_T("   OERNC_PI: chart local server inflate error, final: "));
//             msg.Append(m_FullPath);
//             wxLogMessage(msg);
//             free (m_imageComp);
//             return INIT_FAIL_REMOVE;
//         }
        
      }

//    Perform common post-init actions in ChartBaseBSB
      InitReturn pi_ret = PostInit();


      if( pi_ret  != INIT_OK)
            return pi_ret;
      else
            return INIT_OK;
}

int Chart_oeuRNC::DecodeImage( void )
{
        int inflate_err = 0;
        m_imageMap = (unsigned char *)malloc(Size_X * Size_Y);             
        m_lenImageMap = Size_X * Size_Y;

        inflate_err = ocpn_decode_image( m_imageComp, m_imageMap, ifs_hdr->m_lenIDat, m_lenImageMap, Size_X, Size_Y, m_nColors);

        free(m_imageComp);              // Done with compressed buffer

//#ifdef __WXGTK__
//        printf("Inflate: %g\n", sw.GetTime());
//#endif
        if(inflate_err){
            wxString msg(_T("   OERNC_PI: chart local server inflate error, final: "));
            msg.Append(m_FullPath);
            wxLogMessage(msg);
        }
        
        m_bImageReady = true;
        
        return inflate_err;
}



wxString Chart_oeuRNC::GetFileSearchMask(void)
{
      return _T("*.oernc");
}


wxString Chart_oeuRNC::getKeyAsciiHex(const wxString& name)
{
    wxString key;
    wxString strhex;
    
    // Extract the directory name
    wxFileName fn(name);
    wxString infoFile = fn.GetPath(wxPATH_GET_SEPARATOR + wxPATH_GET_VOLUME) + _T("chartInfo.txt");
    
    wxTextFile info;
    if(info.Open( infoFile)){
        wxString str;
        for ( str = info.GetFirstLine(); !info.Eof(); str = info.GetNextLine() ){
            if(str.StartsWith(_T("productKey"))){
                strhex = str.AfterFirst(_T('='));
                key = strhex.BeforeFirst(_T(':'));
            }
        }
        if(str.StartsWith(_T("productKey"))){
            strhex = str.AfterFirst(_T('='));
            key = strhex.BeforeFirst(_T(':'));
        }
    }
    
    return key;
}


void Chart_oeuRNC::ChartBaseBSBCTOR()
{
      //    Init some private data

      pBitmapFilePath = NULL;

      //pline_table = NULL;
      ifs_buf = NULL;
      ifs_buf_decrypt = NULL;

      cached_image_ok = 0;

      pRefTable = (Refpoint *)malloc(sizeof(Refpoint));
      nRefpoint = 0;
      cPoints.status = 0;
      bHaveEmbeddedGeoref = false;

      bUseLineCache = true;
      m_Chart_Skew = 0.0;

      pPixCache = NULL;

      pLineCache = NULL;

      m_bilinear_limit = 8;         // bilinear scaling only up to n

      ifs_bitmap = NULL;
      ifss_bitmap = NULL;
      ifs_hdr = NULL;

      for(int i = 0 ; i < N_BSB_COLORS ; i++)
            pPalettes[i] = NULL;

      bGeoErrorSent = false;
      m_Chart_DU = 0;
      m_cph = 0.;

      m_mapped_color_index = COLOR_RGB_DEFAULT;

      m_datum_str = _T("WGS84");                // assume until proven otherwise

      m_dtm_lat = 0.;
      m_dtm_lon = 0.;

      m_bIDLcross = false;

      m_dx = 0.;
      m_dy = 0.;
      m_proj_lat = 0.;
      m_proj_lon = 0.;
      m_proj_parameter = 0.;

      m_b_cdebug = 0;

#ifdef OCPN_USE_CONFIG
      wxFileConfig *pfc = pConfig;
      pfc->SetPath ( _T ( "/Settings" ) );
      pfc->Read ( _T ( "DebugBSBImg" ),  &m_b_cdebug, 0 );
#endif

}

void Chart_oeuRNC::ChartBaseBSBDTOR()
{
      if(m_FullPath.Len())
      {
            wxString msg(_T("OFC_PI:  Closing chart "));
            msg += m_FullPath;
            //wxLogMessage(msg);
      }

      if(pBitmapFilePath)
            delete pBitmapFilePath;

      //if(pline_table)
       //     free(pline_table);

      if(ifs_buf)
            free(ifs_buf);

      if(ifs_buf_decrypt)
            free(ifs_buf_decrypt);

      free(pRefTable);
//      free(pPlyTable);

      delete ifs_bitmap;
      delete ifs_hdr;
      delete ifss_bitmap;

      if(cPoints.status)
      {
          free(cPoints.tx );
          free(cPoints.ty );
          free(cPoints.lon );
          free(cPoints.lat );

          free(cPoints.pwx );
          free(cPoints.wpx );
          free(cPoints.pwy );
          free(cPoints.wpy );
      }

//    Free the line cache

      FreeLineCacheRows();
      free (pLineCache);
      

      delete pPixCache;

//      delete pPixCacheBackground;
//      free(background_work_buffer);


      for(int i = 0 ; i < N_BSB_COLORS ; i++)
            delete pPalettes[i];

}

//    Report recommended minimum and maximum scale values for which use of this chart is valid

double Chart_oeuRNC::GetNormalScaleMin(double canvas_scale_factor, bool b_allow_overzoom)
{
      if(b_allow_overzoom)
          return (canvas_scale_factor / m_pi_ppm_avg) / 32;         // allow wide range overzoom overscale
      else
          return (canvas_scale_factor / m_pi_ppm_avg) / 2;         // don't suggest too much overscale

}

double Chart_oeuRNC::GetNormalScaleMax(double canvas_scale_factor, int canvas_width)
{
    return (canvas_scale_factor / m_pi_ppm_avg) * 4.0;        // excessive underscale is slow, and unreadable
}


double Chart_oeuRNC::GetNearestPreferredScalePPM(double target_scale_ppm)
{
      return GetClosestValidNaturalScalePPM(target_scale_ppm, .01, 64.);            // changed from 32 to 64 to allow super small
                                                                                    // scale BSB charts as quilt base
}



double Chart_oeuRNC::GetClosestValidNaturalScalePPM(double target_scale, double scale_factor_min, double scale_factor_max)
{
      double chart_1x_scale = GetPPM();

      double binary_scale_factor = 1.;



      //    Overzoom....
      if(chart_1x_scale > target_scale)
      {
            double binary_scale_factor_max = 1 / scale_factor_min;

            while(binary_scale_factor < binary_scale_factor_max)
            {
                  if(fabs((chart_1x_scale / binary_scale_factor ) - target_scale) < (target_scale * 0.05))
                        break;
                  if((chart_1x_scale / binary_scale_factor ) < target_scale)
                        break;
                  else
                        binary_scale_factor *= 2.;
            }
      }


      //    Underzoom.....
      else
      {
            int ibsf = 1;
            int isf_max = (int)scale_factor_max;
            while(ibsf < isf_max)
            {
                  if(fabs((chart_1x_scale * ibsf ) - target_scale) < (target_scale * 0.05))
                        break;

                  else if((chart_1x_scale * ibsf ) > target_scale)
                  {
                        if(ibsf > 1)
                              ibsf /= 2;
                        break;
                  }
                  else
                        ibsf *= 2;
            }

            binary_scale_factor = 1. / ibsf;
      }

      return  chart_1x_scale / binary_scale_factor;
}



InitReturn Chart_oeuRNC::PreInit( const wxString& name, int init_flags, int cs )
{
      m_global_color_scheme = cs;
      return INIT_OK;
}

void Chart_oeuRNC::CreatePaletteEntry(char *buffer, int palette_index)
{
    if(palette_index < N_BSB_COLORS)
    {
      if(!pPalettes[palette_index])
            pPalettes[palette_index] = new opncpnPalette;
      opncpnPalette *pp = pPalettes[palette_index];

      pp->FwdPalette = (int *)realloc(pp->FwdPalette, (pp->nFwd + 1) * sizeof(int));
      pp->RevPalette = (int *)realloc(pp->RevPalette, (pp->nRev + 1) * sizeof(int));
      pp->nFwd++;
      pp->nRev++;

      int i;
      int n,r,g,b;
      sscanf(&buffer[4], "%d,%d,%d,%d", &n, &r, &g, &b);

      i=n;

      int fcolor, rcolor;
      fcolor = (b << 16) + (g << 8) + r;
      rcolor = (r << 16) + (g << 8) + b;

      pp->RevPalette[i] = rcolor;
      pp->FwdPalette[i] = fcolor;
    }
}



InitReturn Chart_oeuRNC::PostInit(void)
{
     //    Validate the palette array, substituting DEFAULT for missing entries
      for(int i = 0 ; i < N_BSB_COLORS ; i++)
      {
            if(pPalettes[i] == NULL)
            {
                opncpnPalette *pNullSubPal = new opncpnPalette;

                pNullSubPal->nFwd = pPalettes[COLOR_RGB_DEFAULT]->nFwd;        // copy the palette count
                pNullSubPal->nRev = pPalettes[COLOR_RGB_DEFAULT]->nRev;        // copy the palette count
                //  Deep copy the palette rgb tables
                free( pNullSubPal->FwdPalette );
                pNullSubPal->FwdPalette = (int *)malloc(pNullSubPal->nFwd * sizeof(int));
                memcpy(pNullSubPal->FwdPalette, pPalettes[COLOR_RGB_DEFAULT]->FwdPalette, pNullSubPal->nFwd * sizeof(int));

                free( pNullSubPal->RevPalette );
                pNullSubPal->RevPalette = (int *)malloc(pNullSubPal->nRev * sizeof(int));
                memcpy(pNullSubPal->RevPalette, pPalettes[COLOR_RGB_DEFAULT]->RevPalette, pNullSubPal->nRev * sizeof(int));

                pPalettes[i] = pNullSubPal;
            }
      }

      // Establish the palette type and default palette
      palette_direction = GetPaletteDir();

      SetColorScheme(m_global_color_scheme, false);

      //    Allocate memory for ifs file buffering
      ifs_bufsize = Size_X * 4;
      ifs_buf = (unsigned char *)malloc(ifs_bufsize);
      ifs_buf_decrypt = (unsigned char *)malloc(ifs_bufsize);
      if(!ifs_buf)
            return INIT_FAIL_REMOVE;

      ifs_bufend = ifs_buf + ifs_bufsize;
      ifs_lp = ifs_bufend;
      ifs_file_offset = -ifs_bufsize;

#if 0
      //    Create and load the line offset index table
      pline_table = NULL;
      pline_table = (int *)malloc((Size_Y+1) * sizeof(int) );               //Ugly....
      if(!pline_table)
            return INIT_FAIL_REMOVE;

//      int a = ifs_bitmap->TellI();
      ifs_bitmap->SeekI((Size_Y+1) * -4, wxFromEnd);                 // go to Beginning of offset table
//      int b = ifs_bitmap->TellI();
      pline_table[Size_Y] = ifs_bitmap->TellI();                     // fill in useful last table entry

      int offset;
      for(int ifplt=0 ; ifplt<Size_Y ; ifplt++)
      {
          unsigned char c4[4];
          ifs_bitmap->Read(c4, 4);

          offset = 0;
          offset += c4[0] * 256 * 256 * 256;
          offset += c4[1] * 256 * 256 ;
          offset += c4[2] * 256 ;
          offset += c4[3];

          pline_table[ifplt] = offset;
      }

#endif



#if 0
      //    Allocate the Line Cache
      if(bUseLineCache)
      {
            pLineCache = (CachedLine *)malloc(Size_Y * sizeof(CachedLine));
            CachedLine *pt;

            for(int ylc = 0 ; ylc < Size_Y ; ylc++)
            {
                  pt = &pLineCache[ylc];
                  pt->bValid = false;
                  pt->xstart = 0;
                  pt->xlength = 1;
                  pt->pPix = NULL;        //(unsigned char *)malloc(1);
            }
      }
      else
            pLineCache = NULL;
#endif
            
      if(bUseLineCache)
      {
          pLineCache = (CachedLine *)malloc(Size_Y * sizeof(CachedLine));
          CachedLine *pt;
          
          for(int ylc = 0 ; ylc < Size_Y ; ylc++)
          {
              pt = &pLineCache[ylc];
              pt->bValid = false;
              pt->pPix = NULL;        //(unsigned char *)malloc(1);
              pt->pTileOffset = NULL;
          }
      }
      else
          pLineCache = NULL;
      

      //    Validate/Set Depth Unit Type
      wxString test_str = m_DepthUnits.Upper();
      if(test_str.IsSameAs(_T("FEET"), FALSE))
          m_depth_unit_id = PI_DEPTH_UNIT_FEET;
      else if(test_str.IsSameAs(_T("METERS"), FALSE))
          m_depth_unit_id = PI_DEPTH_UNIT_METERS;
      else if(test_str.IsSameAs(_T("METRES"), FALSE))                  // Special case for alternate spelling
          m_depth_unit_id = PI_DEPTH_UNIT_METERS;
      else if(test_str.IsSameAs(_T("FATHOMS"), FALSE))
          m_depth_unit_id = PI_DEPTH_UNIT_FATHOMS;
      else if(test_str.Find(_T("FATHOMS")) != wxNOT_FOUND)             // Special case for "Fathoms and Feet"
          m_depth_unit_id = PI_DEPTH_UNIT_FATHOMS;
      else if(test_str.Find(_T("METERS")) != wxNOT_FOUND)             // Special case for "Meters and decimeters"
            m_depth_unit_id = PI_DEPTH_UNIT_METERS;

           //    Setup the datum transform parameters
      char d_str[100];
      strncpy(d_str, m_datum_str.mb_str(), 99);
      d_str[99] = 0;

      m_datum_index = GetDatumIndex(d_str);


      //   Analyze Refpoints
      int analyze_ret_val = AnalyzeRefpoints();
      if(0 != analyze_ret_val)
            return INIT_FAIL_REMOVE;

      //    Establish defaults, may be overridden later
      m_lon_datum_adjust = (-m_dtm_lon) / 3600.;
      m_lat_datum_adjust = (-m_dtm_lat) / 3600.;

      m_bReadyToRender = true;
      return INIT_OK;
}



//    Invalidate and Free the line cache contents
void Chart_oeuRNC::InvalidateLineCache(void)
{
      if(pLineCache)
      {
            CachedLine *pt;
            for(int ylc = 0 ; ylc < Size_Y ; ylc++)
            {
                  pt = &pLineCache[ylc];
                  if(pt)
                  {
                        if(pt->pPix)
                        {
                              free (pt->pPix);
                              pt->pPix = NULL;
                        }
                        pt->bValid = 0;
                  }
            }
      }
}

bool Chart_oeuRNC::GetChartExtent(ExtentPI *pext)
{
      pext->NLAT = m_LatMax;
      pext->SLAT = m_LatMin;
      pext->ELON = m_LonMax;
      pext->WLON = m_LonMin;

      return true;
}


bool Chart_oeuRNC::SetMinMax(void)
{
      //    Calculate the Chart Extents(M_LatMin, M_LonMin, etc.)
      //     from the COVR data, for fast database search
      m_LonMax = -360.0;
      m_LonMin = 360.0;
      m_LatMax = -90.0;
      m_LatMin = 90.0;

      Plypoint *ppp = (Plypoint *)GetCOVRTableHead(0);
      int cnPlypoint = GetCOVRTablenPoints(0);

      for(int u=0 ; u<cnPlypoint ; u++)
      {
            if(ppp->lnp > m_LonMax)
                  m_LonMax = ppp->lnp;
            if(ppp->lnp < m_LonMin)
                  m_LonMin = ppp->lnp;

            if(ppp->ltp > m_LatMax)
                  m_LatMax = ppp->ltp;
            if(ppp->ltp < m_LatMin)
                  m_LatMin = ppp->ltp;

            ppp++;
      }

      //    Check for special cases

      //    Case 1:  Chart spans International Date Line or Greenwich, Longitude min/max is non-obvious.
      if((m_LonMax * m_LonMin) < 0)              // min/max are opposite signs
      {
            //    Georeferencing is not yet available, so find the reference points closest to min/max ply points

            if(0 == nRefpoint)
                  return false;        // have to bail here

                  //    for m_LonMax
            double min_dist_x = 360;
            int imaxclose = 0;
            for(int ic=0 ; ic<nRefpoint ; ic++)
            {
                  double dist = sqrt(((m_LatMax - pRefTable[ic].latr) * (m_LatMax - pRefTable[ic].latr))
                                    + ((m_LonMax - pRefTable[ic].lonr) * (m_LonMax - pRefTable[ic].lonr)));

                  if(dist < min_dist_x)
                  {
                        min_dist_x = dist;
                        imaxclose = ic;
                  }
            }

                  //    for m_LonMin
            double min_dist_n = 360;
            int iminclose = 0;
            for(int id=0 ; id<nRefpoint ; id++)
            {
                  double dist = sqrt(((m_LatMin - pRefTable[id].latr) * (m_LatMin - pRefTable[id].latr))
                                    + ((m_LonMin - pRefTable[id].lonr) * (m_LonMin - pRefTable[id].lonr)));

                  if(dist < min_dist_n)
                  {
                        min_dist_n = dist;
                        iminclose = id;
                  }
            }

            //    Is this chart crossing IDL or Greenwich?
            // Make the check
            if(pRefTable[imaxclose].xr < pRefTable[iminclose].xr)
            {
                  //    This chart crosses IDL and needs a flip, meaning that all negative longitudes need to be normalized
                  //    and the min/max relcalculated
                  //    This code added to correct non-rectangular charts crossing IDL, such as nz14605.kap

                  m_LonMax = -360.0;
                  m_LonMin = 360.0;
                  m_LatMax = -90.0;
                  m_LatMin = 90.0;


                  Plypoint *ppp = (Plypoint *)GetCOVRTableHead(0);      // Normalize the plypoints
                  int cnPlypoint = GetCOVRTablenPoints(0);


                  for(int u=0 ; u<cnPlypoint ; u++)
                  {
                        if( ppp->lnp < 0.)
                              ppp->lnp += 360.;

                        if(ppp->lnp > m_LonMax)
                              m_LonMax = ppp->lnp;
                        if(ppp->lnp < m_LonMin)
                              m_LonMin = ppp->lnp;

                        if(ppp->ltp > m_LatMax)
                              m_LatMax = ppp->ltp;
                        if(ppp->ltp < m_LatMin)
                              m_LatMin = ppp->ltp;

                        ppp++;
                  }
            }


      }

      // Case 2 Lons are both < -180, which means the extent will be reported incorrectly
      // and the plypoint structure will be wrong
      // This case is seen first on 81004_1.KAP, (Mariannas)

      if((m_LonMax < -180.) && (m_LonMin < -180.))
      {
            m_LonMin += 360.;               // Normalize the extents
            m_LonMax += 360.;

            Plypoint *ppp = (Plypoint *)GetCOVRTableHead(0);      // Normalize the plypoints
            int cnPlypoint = GetCOVRTablenPoints(0);

            for(int u=0 ; u<cnPlypoint ; u++)
            {
                  ppp->lnp += 360.;
                  ppp++;
            }
      }

      return true;
}

void Chart_oeuRNC::SetColorScheme(int cs, bool bApplyImmediate)
{
    //  Here we convert (subjectively) the Global ColorScheme
    //  to an appropriate BSB_Color_Capability index.

    switch(cs)
    {
        case PI_GLOBAL_COLOR_SCHEME_RGB:
            m_mapped_color_index = COLOR_RGB_DEFAULT;
            break;
        case PI_GLOBAL_COLOR_SCHEME_DAY:
            m_mapped_color_index = DAY;
            break;
        case PI_GLOBAL_COLOR_SCHEME_DUSK:
            m_mapped_color_index = DUSK;
            break;
        case PI_GLOBAL_COLOR_SCHEME_NIGHT:
            m_mapped_color_index = NIGHT;
            break;
        default:
            m_mapped_color_index = DAY;
            break;
    }

    pPalette = GetPalettePtr(m_mapped_color_index);

    m_global_color_scheme = cs;

    //      Force a cache dump in a simple sideways manner
    if(bApplyImmediate)
    {
          m_cached_scale_ppm = 1.0;
    }


}


wxBitmap *Chart_oeuRNC::GetThumbnail(int tnx, int tny, int cs)
{
      if(m_pBMPThumb && (m_pBMPThumb->GetWidth() == tnx) && (m_pBMPThumb->GetHeight() == tny) && (m_thumbcs == cs))
            return m_pBMPThumb;

      delete m_pBMPThumb;
      m_thumbcs = cs;

//    Calculate the size and divisors

      int divx = Size_X / tnx;
      int divy = Size_Y / tny;

      int div_factor = wxMin(divx, divy);

      int des_width = Size_X / div_factor;
      int des_height = Size_Y / div_factor;

      wxRect gts;
      gts.x = 0;                                // full chart
      gts.y = 0;
      gts.width = Size_X;
      gts.height = Size_Y;

      int this_bpp = 24;                       // for wxImage
//    Allocate the pixel storage needed for one line of chart bits
      unsigned char *pLineT = (unsigned char *)malloc((Size_X+1) * BPP/8);

//    Scale the data quickly
      unsigned char *pPixTN = (unsigned char *)malloc(des_width * des_height * this_bpp/8 );

      int ix = 0;
      int iy = 0;
      int iyd = 0;
      int ixd = 0;
      int yoffd;
      unsigned char *pxs;
      unsigned char *pxd;

      //    Temporarily set the color scheme
      int cs_tmp = m_global_color_scheme;
      SetColorScheme(cs, false);


      while(iyd < des_height)
      {
            if(0 == BSBGetScanline( pLineT, iy, 0, Size_X, 1))          // get a line
            {
                  free(pLineT);
                  free(pPixTN);
                  return NULL;
            }


            yoffd = iyd * des_width * this_bpp/8;                 // destination y

            ix = 0;
            ixd = 0;
            while(ixd < des_width )
            {
                  pxs = pLineT + (ix * BPP/8);
                  pxd = pPixTN + (yoffd + (ixd * this_bpp/8));
                  *pxd++ = *pxs++;
                  *pxd++ = *pxs++;
                  *pxd = *pxs;

                  ix += div_factor;
                  ixd++;

            }

            iy += div_factor;
            iyd++;
      }

      free(pLineT);

      //    Reset ColorScheme
      SetColorScheme(cs_tmp, false);




//#ifdef ocpnUSE_ocpnBitmap
//      m_pBMPThumb = new PIocpnBitmap(pPixTN, des_width, des_height, -1);
//#else
      wxImage thumb_image(des_width, des_height, pPixTN, true);
      m_pBMPThumb = new wxBitmap(thumb_image);
//#endif

      free(pPixTN);

      return m_pBMPThumb;

}

#if 0
//-------------------------------------------------------------------------------------------------
//          Get the Chart thumbnail data structure
//          Creating the thumbnail bitmap as required
//-------------------------------------------------------------------------------------------------

ThumbData *ChartNVC::GetThumbData(int tnx, int tny, float lat, float lon)
{
//    Create the bitmap if needed
      if(!pThumbData->pDIBThumb)
            pThumbData->pDIBThumb = CreateThumbnail(tnx, tny, m_global_color_scheme);


      pThumbData->Thumb_Size_X = tnx;
      pThumbData->Thumb_Size_Y = tny;

//    Plot the supplied Lat/Lon on the thumbnail
      int divx = Size_X / tnx;
      int divy = Size_Y / tny;

      int div_factor = wxMin(divx, divy);

      int pixx, pixy;


      //    Using a temporary synthetic ViewPort and source rectangle,
      //    calculate the ships position on the thumbnail
      PlugIn_ViewPort tvp;
      tvp.pix_width = tnx;
      tvp.pix_height = tny;
      tvp.view_scale_ppm = GetPPM() / div_factor;
      wxRect trex = Rsrc;
      Rsrc.x = 0;
      Rsrc.y = 0;
      latlong_to_pix_vp(lat, lon, pixx, pixy, tvp);
      Rsrc = trex;

      pThumbData->ShipX = pixx;// / div_factor;
      pThumbData->ShipY = pixy;// / div_factor;


      return pThumbData;
}

bool ChartNVC::UpdateThumbData(double lat, double lon)
{
//    Plot the supplied Lat/Lon on the thumbnail
//  Return TRUE if the pixel location of ownship has changed

    int divx = Size_X / pThumbData->Thumb_Size_X;
    int divy = Size_Y / pThumbData->Thumb_Size_Y;

    int div_factor = wxMin(divx, divy);

    int pixx_test, pixy_test;


      //    Using a temporary synthetic ViewPort and source rectangle,
      //    calculate the ships position on the thumbnail
    PlugIn_ViewPort tvp;
    tvp.pix_width =  pThumbData->Thumb_Size_X;
    tvp.pix_height =  pThumbData->Thumb_Size_Y;
    tvp.view_scale_ppm = GetPPM() / div_factor;
    wxRect trex = Rsrc;
    Rsrc.x = 0;
    Rsrc.y = 0;
    latlong_to_pix_vp(lat, lon, pixx_test, pixy_test, tvp);
    Rsrc = trex;

    if((pixx_test != pThumbData->ShipX) || (pixy_test != pThumbData->ShipY))
    {
        pThumbData->ShipX = pixx_test;
        pThumbData->ShipY = pixy_test;
        return TRUE;
    }
    else
        return FALSE;
}

*/

#endif



//-----------------------------------------------------------------------
//          Pixel to Lat/Long Conversion helpers
//-----------------------------------------------------------------------
double polytrans( double* coeff, double lon, double lat );

int Chart_oeuRNC::vp_pix_to_latlong(PlugIn_ViewPort& vp, int pixx, int pixy, double *plat, double *plon)
{
      if(bHaveEmbeddedGeoref)
      {
            double raster_scale = GetPPM() / vp.view_scale_ppm;

            int px = (int)(pixx*raster_scale) + Rsrc.x;
            int py = (int)(pixy*raster_scale) + Rsrc.y;
//            pix_to_latlong(px, py, plat, plon);

            if(1)
            {
                  double lon = polytrans( pwx, px, py );
                  lon = (lon < 0) ? lon + m_cph : lon - m_cph;
                  *plon = lon - m_lon_datum_adjust;
                  *plat = polytrans( pwy, px, py ) - m_lat_datum_adjust;
            }

            return 0;
      }
      else
      {
            double slat, slon;
            double xp, yp;

            if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
            {
                   //      Use Projected Polynomial algorithm

                  double raster_scale = GetPPM() / vp.view_scale_ppm;

                  //      Apply poly solution to vp center point
                  double easting, northing;
                  toTM(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                  double xc = polytrans( cPoints.wpx, easting, northing );
                  double yc = polytrans( cPoints.wpy, easting, northing );

                  //    convert screen pixels to chart pixmap relative
                  double px = xc + (pixx- (vp.pix_width / 2))*raster_scale;
                  double py = yc + (pixy- (vp.pix_height / 2))*raster_scale;

                  //    Apply polynomial solution to chart relative pixels to get e/n
                  double east  = polytrans( cPoints.pwx, px, py );
                  double north = polytrans( cPoints.pwy, px, py );

                  //    Apply inverse Projection to get lat/lon
                  double lat,lon;
                  fromTM ( east, north, m_proj_lat, m_proj_lon, &lat, &lon );

                  //    Datum adjustments.....
//??                  lon = (lon < 0) ? lon + m_cph : lon - m_cph;
                  double slon_p = lon - m_lon_datum_adjust;
                  double slat_p = lat - m_lat_datum_adjust;

//                  printf("%8g %8g %8g %8g %g\n", slat, slat_p, slon, slon_p, slon - slon_p);
                  slon = slon_p;
                  slat = slat_p;

            }
            else if(m_projection == PI_PROJECTION_MERCATOR)
            {
                   //      Use Projected Polynomial algorithm

                  double raster_scale = GetPPM() / vp.view_scale_ppm;

                  //      Apply poly solution to vp center point
                  double easting, northing;
                  toSM_ECC(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                  double xc = polytrans( cPoints.wpx, easting, northing );
                  double yc = polytrans( cPoints.wpy, easting, northing );

                  //    convert screen pixels to chart pixmap relative
                  double px = xc + (pixx- (vp.pix_width / 2))*raster_scale;
                  double py = yc + (pixy- (vp.pix_height / 2))*raster_scale;

                  //    Apply polynomial solution to chart relative pixels to get e/n
                  double east  = polytrans( cPoints.pwx, px, py );
                  double north = polytrans( cPoints.pwy, px, py );

                  //    Apply inverse Projection to get lat/lon
                  double lat,lon;
                  fromSM_ECC ( east, north, m_proj_lat, m_proj_lon, &lat, &lon );

                  //    Make Datum adjustments.....
                  double slon_p = lon - m_lon_datum_adjust;
                  double slat_p = lat - m_lat_datum_adjust;

                  slon = slon_p;
                  slat = slat_p;

//                  printf("vp.clon  %g    xc  %g   px   %g   east  %g  \n", vp.clon, xc, px, east);

            }
            else
            {
                  // Use a Mercator estimator, with Eccentricity corrrection applied
                  int dx = pixx - ( vp.pix_width  / 2 );
                  int dy = ( vp.pix_height / 2 ) - pixy;

                  xp = ( dx * cos ( vp.skew ) ) - ( dy * sin ( vp.skew ) );
                  yp = ( dy * cos ( vp.skew ) ) + ( dx * sin ( vp.skew ) );

                  double d_east = xp / vp.view_scale_ppm;
                  double d_north = yp / vp.view_scale_ppm;

                  fromSM_ECC ( d_east, d_north, vp.clat, vp.clon, &slat, &slon );
            }

            *plat = slat;

            if(slon < -180.)
                  slon += 360.;
            else if(slon > 180.)
                  slon -= 360.;
            *plon = slon;

            return 0;
      }

}




int Chart_oeuRNC::latlong_to_pix_vp(double lat, double lon, int &pixx, int &pixy, PlugIn_ViewPort& vp)
{
    int px, py;

    double alat, alon;

    if(bHaveEmbeddedGeoref)
    {
          double alat, alon;

          alon = lon + m_lon_datum_adjust;
          alat = lat + m_lat_datum_adjust;

          if(m_bIDLcross)
          {
                if(alon < 0.)
                      alon += 360.;
          }

          if(1)
          {
                /* change longitude phase (CPH) */
                double lonp = (alon < 0) ? alon + m_cph : alon - m_cph;
                double xd = polytrans( wpx, lonp, alat );
                double yd = polytrans( wpy, lonp, alat );
                px = (int)(xd + 0.5);
                py = (int)(yd + 0.5);


                double raster_scale = GetPPM() / vp.view_scale_ppm;

                pixx = (int)(((px - Rsrc.x) / raster_scale) + 0.5);
                pixy = (int)(((py - Rsrc.y) / raster_scale) + 0.5);

            return 0;
          }
    }
    else
    {
          double easting, northing;
          double xlon = lon;

                //  Make sure lon and lon0 are same phase
/*
          if((xlon * vp.clon) < 0.)
          {
                if(xlon < 0.)
                      xlon += 360.;
                else
                      xlon -= 360.;
          }

          if(fabs(xlon - vp.clon) > 180.)
          {
                if(xlon > vp.clon)
                      xlon -= 360.;
                else
                      xlon += 360.;
          }
*/


          if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
          {
                //      Use Projected Polynomial algorithm

                alon = lon + m_lon_datum_adjust;
                alat = lat + m_lat_datum_adjust;

                //      Get e/n from TM Projection
                toTM(alat, alon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                double xd = polytrans( cPoints.wpx, easting, northing );
                double yd = polytrans( cPoints.wpy, easting, northing );

                //      Apply poly solution to vp center point
                toTM(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                double xc = polytrans( cPoints.wpx, easting, northing );
                double yc = polytrans( cPoints.wpy, easting, northing );

                //      Calculate target point relative to vp center
                double raster_scale = GetPPM() / vp.view_scale_ppm;

                int xs = (int)xc - (int)(vp.pix_width  * raster_scale / 2);
                int ys = (int)yc - (int)(vp.pix_height * raster_scale / 2);

                int pixx_p = (int)(((xd - xs) / raster_scale) + 0.5);
                int pixy_p = (int)(((yd - ys) / raster_scale) + 0.5);

//                printf("  %d  %d  %d  %d\n", pixx, pixx_p, pixy, pixy_p);

                pixx = pixx_p;
                pixy = pixy_p;

          }
          else if(m_projection == PI_PROJECTION_MERCATOR)
          {
                //      Use Projected Polynomial algorithm

                alon = lon + m_lon_datum_adjust;
                alat = lat + m_lat_datum_adjust;

                //      Get e/n from  Projection
                xlon = alon;
                if(m_bIDLcross)
                {
                      if(xlon < 0.)
                            xlon += 360.;
                }
                toSM_ECC(alat, xlon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                double xd = polytrans( cPoints.wpx, easting, northing );
                double yd = polytrans( cPoints.wpy, easting, northing );

                //      Apply poly solution to vp center point
                double xlonc = vp.clon;
                if(m_bIDLcross)
                {
                      if(xlonc < 0.)
                            xlonc += 360.;
                }

                toSM_ECC(vp.clat + m_lat_datum_adjust, xlonc + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
                double xc = polytrans( cPoints.wpx, easting, northing );
                double yc = polytrans( cPoints.wpy, easting, northing );

                //      Calculate target point relative to vp center
                double raster_scale = GetPPM() / vp.view_scale_ppm;

                int xs = (int)xc - (int)(vp.pix_width  * raster_scale / 2);
                int ys = (int)yc - (int)(vp.pix_height * raster_scale / 2);

                int pixx_p = (int)(((xd - xs) / raster_scale) + 0.5);
                int pixy_p = (int)(((yd - ys) / raster_scale) + 0.5);

                pixx = pixx_p;
                pixy = pixy_p;

          }
          else
         {
                toSM_ECC(lat, xlon, vp.clat, vp.clon, &easting, &northing);

                double epix = easting  * vp.view_scale_ppm;
                double npix = northing * vp.view_scale_ppm;

                double dx = epix * cos ( vp.skew ) + npix * sin ( vp.skew );
                double dy = npix * cos ( vp.skew ) - epix * sin ( vp.skew );

                pixx = ( int ) /*rint*/( ( vp.pix_width  / 2 ) + dx );
                pixy = ( int ) /*rint*/( ( vp.pix_height / 2 ) - dy );
         }
                return 0;
    }

    return 1;
}

void Chart_oeuRNC::latlong_to_chartpix(double lat, double lon, double &pixx, double &pixy)
{
      double alat, alon;

      if(bHaveEmbeddedGeoref)
      {
            double alat, alon;

            alon = lon + m_lon_datum_adjust;
            alat = lat + m_lat_datum_adjust;

            if(m_bIDLcross)
            {
                  if(alon < 0.)
                        alon += 360.;
            }


            /* change longitude phase (CPH) */
            double lonp = (alon < 0) ? alon + m_cph : alon - m_cph;
            pixx = polytrans( wpx, lonp, alat );
            pixy = polytrans( wpy, lonp, alat );
      }
      else
      {
            double easting, northing;
            double xlon = lon;

            if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
            {
                //      Use Projected Polynomial algorithm

                  alon = lon + m_lon_datum_adjust;
                  alat = lat + m_lat_datum_adjust;

                //      Get e/n from TM Projection
                  toTM(alat, alon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                  pixx = polytrans( cPoints.wpx, easting, northing );
                  pixy = polytrans( cPoints.wpy, easting, northing );


            }
            else if(m_projection == PI_PROJECTION_MERCATOR)
            {
                //      Use Projected Polynomial algorithm

                  alon = lon + m_lon_datum_adjust;
                  alat = lat + m_lat_datum_adjust;

                //      Get e/n from  Projection
                  xlon = alon;
                  if(m_bIDLcross)
                  {
                        if(xlon < 0.)
                              xlon += 360.;
                  }
                  toSM_ECC(alat, xlon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                  pixx = polytrans( cPoints.wpx, easting, northing );
                  pixy = polytrans( cPoints.wpy, easting, northing );


            }
#if 0       // PlugIns do not support Polyconic charts
            else if(m_projection == PROJECTION_POLYCONIC)
            {
                //      Use Projected Polynomial algorithm

                  alon = lon + m_lon_datum_adjust;
                  alat = lat + m_lat_datum_adjust;

                //      Get e/n from  Projection
                  xlon = alon;
                  if(m_bIDLcross)
                  {
                        if(xlon < 0.)
                              xlon += 360.;
                  }
                  toPOLY(alat, xlon, m_proj_lat, m_proj_lon, &easting, &northing);

                //      Apply poly solution to target point
                  pixx = polytrans( cPoints.wpx, easting, northing );
                  pixy = polytrans( cPoints.wpy, easting, northing );

            }
#endif
      }
}

void Chart_oeuRNC::chartpix_to_latlong(double pixx, double pixy, double *plat, double *plon)
{
      if(bHaveEmbeddedGeoref)
      {
            double lon = polytrans( pwx, pixx, pixy );
            lon = (lon < 0) ? lon + m_cph : lon - m_cph;
            *plon = lon - m_lon_datum_adjust;
            *plat = polytrans( pwy, pixx, pixy ) - m_lat_datum_adjust;
      }
      else
      {
            double slat, slon;
            if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
            {
                   //      Use Projected Polynomial algorithm

                  //    Apply polynomial solution to chart relative pixels to get e/n
                  double east  = polytrans( cPoints.pwx, pixx, pixy );
                  double north = polytrans( cPoints.pwy, pixx, pixy );

                  //    Apply inverse Projection to get lat/lon
                  double lat,lon;
                  fromTM ( east, north, m_proj_lat, m_proj_lon, &lat, &lon );

                  //    Datum adjustments.....
//??                  lon = (lon < 0) ? lon + m_cph : lon - m_cph;
                  slon = lon - m_lon_datum_adjust;
                  slat = lat - m_lat_datum_adjust;


            }
            else if(m_projection == PI_PROJECTION_MERCATOR)
            {
                   //      Use Projected Polynomial algorithm
                  //    Apply polynomial solution to chart relative pixels to get e/n
                  double east  = polytrans( cPoints.pwx, pixx, pixy );
                  double north = polytrans( cPoints.pwy, pixx, pixy );

                  //    Apply inverse Projection to get lat/lon
                  double lat,lon;
                  fromSM_ECC ( east, north, m_proj_lat, m_proj_lon, &lat, &lon );

                  //    Make Datum adjustments.....
                  slon = lon - m_lon_datum_adjust;
                  slat = lat - m_lat_datum_adjust;
            }
            else
            {
                  slon = 0.;
                  slat = 0.;
            }

            *plat = slat;

            if(slon < -180.)
                  slon += 360.;
            else if(slon > 180.)
                  slon -= 360.;
            *plon = slon;

      }

}

void Chart_oeuRNC::ComputeSourceRectangle(const PlugIn_ViewPort &vp, wxRect *pSourceRect)
{

    //      This funny contortion is necessary to allow scale factors < 1, i.e. overzoom
    double binary_scale_factor = (wxRound(100000 * GetPPM() / vp.view_scale_ppm)) / 100000.;

//    if((binary_scale_factor > 1.0) && (fabs(binary_scale_factor - wxRound(binary_scale_factor)) < 1e-2))
//          binary_scale_factor = wxRound(binary_scale_factor);

    m_piraster_scale_factor = binary_scale_factor;

    if(m_b_cdebug)printf(" ComputeSourceRect... PPM: %g  vp.view_scale_ppm: %g   m_piraster_scale_factor: %g\n", GetPPM(), vp.view_scale_ppm, m_piraster_scale_factor);

      double xd, yd;
      latlong_to_chartpix(vp.clat, vp.clon, xd, yd);


      pSourceRect->x = wxRound(xd - (vp.pix_width  * binary_scale_factor / 2));
      pSourceRect->y = wxRound(yd - (vp.pix_height * binary_scale_factor / 2));

      pSourceRect->width =  (int)wxRound(vp.pix_width  * binary_scale_factor) ;
      pSourceRect->height = (int)wxRound(vp.pix_height * binary_scale_factor) ;

#if 0
    if(bHaveEmbeddedGeoref)
    {

          /* change longitude phase (CPH) */
          double lonp = (vp.clon < 0) ? vp.clon + m_cph : vp.clon - m_cph;
          double xd = polytrans( wpx, lonp + m_lon_datum_adjust,  vp.clat + m_lat_datum_adjust );
          double yd = polytrans( wpy, lonp + m_lon_datum_adjust,  vp.clat + m_lat_datum_adjust );
          pixxd = (int)wxRound(xd);
          pixyd = (int)wxRound(yd);

          pSourceRect->x = pixxd - (int)wxRound(vp.pix_width  * binary_scale_factor / 2);
          pSourceRect->y = pixyd - (int)wxRound(vp.pix_height * binary_scale_factor / 2);

          pSourceRect->width =  (int)wxRound(vp.pix_width  * binary_scale_factor) ;
          pSourceRect->height = (int)wxRound(vp.pix_height * binary_scale_factor) ;

    }

    else
    {
        if(m_projection == PI_PROJECTION_MERCATOR)
        {
                  //      Apply poly solution to vp center point
              double easting, northing;
              double xlon = vp.clon;
              if(m_bIDLcross)
              {
                    if(xlon < 0)
                        xlon += 360.;
              }
              toSM_ECC(vp.clat + m_lat_datum_adjust, xlon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
              double xc = polytrans( cPoints.wpx, easting, northing );
              double yc = polytrans( cPoints.wpy, easting, northing );

                  //    convert screen pixels to chart pixmap relative
              pSourceRect->x = (int)(xc - (vp.pix_width / 2)*binary_scale_factor);
              pSourceRect->y = (int)(yc - (vp.pix_height / 2)*binary_scale_factor);

              pSourceRect->width =  (int)(vp.pix_width  * binary_scale_factor) ;
              pSourceRect->height = (int)(vp.pix_height * binary_scale_factor) ;

 //             printf("Compute Rsrc:  vp.clat:  %g    Rsrc.y: %d  \n", vp.clat, pSourceRect->y);

        }


        if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
        {
                  //      Apply poly solution to vp center point
              double easting, northing;
              toTM(vp.clat + m_lat_datum_adjust, vp.clon + m_lon_datum_adjust, m_proj_lat, m_proj_lon, &easting, &northing);
              double xc = polytrans( cPoints.wpx, easting, northing );
              double yc = polytrans( cPoints.wpy, easting, northing );

                  //    convert screen pixels to chart pixmap relative
              pSourceRect->x = (int)(xc - (vp.pix_width / 2)*binary_scale_factor);
              pSourceRect->y = (int)(yc - (vp.pix_height / 2)*binary_scale_factor);

              pSourceRect->width =  (int)(vp.pix_width  * binary_scale_factor) ;
              pSourceRect->height = (int)(vp.pix_height * binary_scale_factor) ;

        }
    }

//    printf("Compute Rsrc:  vp.clat:  %g  clon: %g     Rsrc.y: %d  Rsrc.x:  %d\n", vp.clat, vp.clon, pSourceRect->y, pSourceRect->x);
#endif

}


void Chart_oeuRNC::SetVPRasterParms(const PlugIn_ViewPort &vpt)
{
      //    Calculate the potential datum offset parameters for this viewport, if not WGS84

      if(m_datum_index == DATUM_INDEX_WGS84)
      {
            m_lon_datum_adjust = 0.;
            m_lat_datum_adjust = 0.;
      }

      else if(m_datum_index == DATUM_INDEX_UNKNOWN)
      {
            m_lon_datum_adjust = (-m_dtm_lon) / 3600.;
            m_lat_datum_adjust = (-m_dtm_lat) / 3600.;
      }

      else
      {
            double to_lat, to_lon;
            MolodenskyTransform (vpt.clat, vpt.clon, &to_lat, &to_lon, m_datum_index, DATUM_INDEX_WGS84);
            m_lon_datum_adjust = -(to_lon - vpt.clon);
            m_lat_datum_adjust = -(to_lat - vpt.clat);

      }

      ComputeSourceRectangle(vpt, &Rsrc);

      if(vpt.bValid)
            m_vp_render_last = vpt;

}

bool Chart_oeuRNC::AdjustVP(PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed)
{
      bool bInside = G_FloatPtInPolygon ( ( MyFlPoint * ) GetCOVRTableHead ( 0 ), GetCOVRTablenPoints ( 0 ), vp_proposed.clon, vp_proposed.clat );
      if(!bInside)
            return false;

      PlugIn_ViewPort vp_save = vp_proposed;                 // save a copy

      int ret_val = 0;

      if(vp_last.bValid)
      {

                  double binary_scale_factor = GetPPM() / vp_proposed.view_scale_ppm;

                  //    We only need to adjust the VP if the cache is valid and potentially usable, i.e. the scale factor is integer...
                  //    The objective here is to ensure that the VP center falls on an exact pixel boundary within the cache

                  double dscale = fabs(binary_scale_factor - wxRound(binary_scale_factor));
                  if(m_b_cdebug)printf(" Adjust VP dscale: %g\n", dscale);

                  if(cached_image_ok && (binary_scale_factor > 1.0) && (fabs(binary_scale_factor - wxRound(binary_scale_factor)) < 1e-5))
                  {
                        wxRect rprop;
                        ComputeSourceRectangle(vp_proposed, &rprop);

                        int cs1d = rprop.width/vp_proposed.pix_width;

                        if(cs1d > 0)
                        {
                              double new_lat = vp_proposed.clat;
                              double new_lon = vp_proposed.clon;


                              int dx = (rprop.x - cache_rect.x) % cs1d;
                              if(dx)
                              {
                                  fromSM((double)-dx / m_pi_ppm_avg, 0., vp_proposed.clat, vp_proposed.clon, &new_lat, &new_lon);
                                    vp_proposed.clon = new_lon;
                                    ret_val++;

                              }

                              ComputeSourceRectangle(vp_proposed, &rprop);
                              int dy = (rprop.y - cache_rect.y) % cs1d;
                              if(dy)
                              {
                                  fromSM(0, (double)dy / m_pi_ppm_avg, vp_proposed.clat, vp_proposed.clon, &new_lat, &new_lon);
                                    vp_proposed.clat = new_lat;
                                    ret_val++;

                              }

                              if(m_b_cdebug)printf(" Adjust VP dx: %d  dy:%d\n", dx, dy);

                              //    Check the results...if not good(i.e. VP center is not on cache pixel boundary),
                              //    then leave the vp unmodified by restoring from the saved copy...
                              if(ret_val > 0)
                              {
                                    wxRect rprop_cor;
                                    ComputeSourceRectangle(vp_proposed, &rprop_cor);
                                    int cs2d = rprop_cor.width/vp_proposed.pix_width;
                                    int dxc = (rprop_cor.x - cache_rect.x) % cs2d;
                                    int dyc = (rprop_cor.y - cache_rect.y) % cs2d;

                                    if(m_b_cdebug)printf(" Adjust VP dxc: %d  dyc:%d\n", dxc, dyc);
                                    if(dxc || dyc)
                                    {
                                          vp_proposed.clat = vp_save.clat;
                                          vp_proposed.clon = vp_save.clon;
                                          ret_val = 0;
                                          if(m_b_cdebug)printf(" Adjust VP failed\n");
                                    }
                                    else
                                          if(m_b_cdebug)printf(" Adjust VP succeeded \n");

                              }

                        }
                  }
      }

      return (ret_val > 0);
}

bool Chart_oeuRNC::IsRenderDelta(PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed)
{
//      if ( !vp_last.IsValid()  ||   !vp_proposed.IsValid() )
//            return true;

      wxRect rlast, rthis;

      ComputeSourceRectangle(vp_last, &rlast);
      ComputeSourceRectangle(vp_proposed, &rthis);

      return ((rlast != rthis) || !(IsCacheValid()) || (vp_last.view_scale_ppm != vp_proposed.view_scale_ppm));
}


void Chart_oeuRNC::GetValidCanvasRegion(const PlugIn_ViewPort& VPoint, wxRegion *pValidRegion)
{
      SetVPRasterParms(VPoint);

      double raster_scale =  VPoint.view_scale_ppm / GetPPM();

      int rxl, rxr;
      if(Rsrc.x < 0)
            rxl = (int)(-Rsrc.x * raster_scale);
      else
            rxl = 0;

      if(((Size_X - Rsrc.x) * raster_scale) < VPoint.pix_width)
            rxr = (int)((Size_X - Rsrc.x) * raster_scale);
      else
            rxr = VPoint.pix_width;

      int ryb, ryt;
      if(Rsrc.y < 0)
            ryt = (int)(-Rsrc.y * raster_scale);
      else
            ryt = 0;

      if(((Size_Y - Rsrc.y) * raster_scale) < VPoint.pix_height)
            ryb = (int)((Size_Y - Rsrc.y) * raster_scale);
      else
            ryb = VPoint.pix_height;

      pValidRegion->Clear();
      pValidRegion->Union(rxl, ryt, rxr - rxl, ryb - ryt);
}


bool Chart_oeuRNC::GetViewUsingCache( wxRect& source, wxRect& dest, const wxRegion& Region, int scale_type )
{
      wxRect s1;
      int scale_type_corrected;

      if(m_b_cdebug)printf(" source:  %d %d\n", source.x, source.y);
      if(m_b_cdebug)printf(" cache:   %d %d\n", cache_rect.x, cache_rect.y);

//    Anything to do?
      if((source == cache_rect) /*&& (cache_scale_method == scale_type)*/ && (cached_image_ok) )
      {
            if(m_b_cdebug)printf("    GVUC: Cache is good, nothing to do\n");
            return false;
      }

      double scale_x = (double)source.width / (double)dest.width;

      if(m_b_cdebug)printf("GVUC: scale_x: %g\n", scale_x);

      //    Enforce a limit on bilinear scaling, for performance reasons
      scale_type_corrected = scale_type; //RENDER_LODEF; //scale_type;
      if(scale_x > m_bilinear_limit)
            scale_type_corrected = RENDER_LODEF;

      {
//            if(b_cdebug)printf("   MISS<<<>>>GVUC: Intentional out\n");
//            return GetView( source, dest, scale_type_corrected );
      }


      //    Using the cache only works for pure binary scale factors......
      if((fabs(scale_x - wxRound(scale_x))) > .0001)
      {
            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: Not digital scale test 1\n");
            return GetView( source, dest, scale_type_corrected );
      }

//      scale_type_corrected = RENDER_LODEF;


      if(!cached_image_ok)
      {
            if(m_b_cdebug)printf("    MISS<<<>>>GVUC:  Cache NOk\n");
            return GetView( source, dest, scale_type_corrected );
      }

      if(scale_x <= 1.0)                                        // overzoom
      {
            if(m_b_cdebug)printf("    MISS<<<>>>GVUC:  Overzoom\n");
            return GetView( source, dest, scale_type_corrected );
      }

      //    Scale must be exactly digital...
      if((int)(source.width/dest.width) != (int)wxRound(scale_x))
      {
            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: Not digital scale test 2\n");
            return GetView( source, dest, scale_type_corrected );
      }

//    Calculate the digital scale, e.g. 1,2,4,8,,,
      int cs1d = source.width/dest.width;
      if(abs(source.x - cache_rect.x) % cs1d)
      {
            if(m_b_cdebug)printf("   source.x: %d  cache_rect.x: %d  cs1d: %d\n", source.x, cache_rect.x, cs1d);
            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: x mismatch\n");
            return GetView( source, dest, scale_type_corrected );
      }
      if(abs(source.y - cache_rect.y) % cs1d)
      {
            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: y mismatch\n");
            return GetView( source, dest, scale_type_corrected );
      }

      if(pPixCache && ((pPixCache->GetWidth() != dest.width) || (pPixCache->GetHeight() != dest.height)))
      {
            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: dest size mismatch\n");
            return GetView( source, dest, scale_type_corrected );
      }

      int stride_rows = (source.y + source.height) - (cache_rect.y + cache_rect.height);
      int scaled_stride_rows = (int)(stride_rows / scale_x);

      if(abs(stride_rows) >= source.height)                       // Pan more than one screen
            return GetView( source, dest, scale_type_corrected );

      int stride_pixels = (source.x + source.width) - (cache_rect.x + cache_rect.width);
      int scaled_stride_pixels = (int)(stride_pixels / scale_x);


      if(abs(stride_pixels) >= source.width)                      // Pan more than one screen
            return GetView( source, dest, scale_type_corrected );

      if(m_b_cdebug)printf("    GVUC Using raster data cache\n");

      int pan_scale_type_x = scale_type_corrected;
      int pan_scale_type_y = scale_type_corrected;


      //    "Blit" the valid pixels out of the way
      int height = pPixCache->GetHeight();
      int width = pPixCache->GetWidth();
      int stride = pPixCache->GetLinePitch();

      unsigned char *ps;
      unsigned char *pd;

      if(stride_rows > 0)                             // pan down
      {
           ps = pPixCache->GetpData() +  (abs(scaled_stride_rows) * stride);
           if(stride_pixels > 0)
                 ps += scaled_stride_pixels * BPP/8;

           pd = pPixCache->GetpData();
           if(stride_pixels <= 0)
                 pd += abs(scaled_stride_pixels) * BPP/8;

           for(int iy=0 ; iy< (height - abs(scaled_stride_rows)) ; iy++)
           {
                 memmove(pd, ps, (width - abs(scaled_stride_pixels)) *BPP/8);

                 ps += width * BPP/8;
                 pd += width * BPP/8;
           }

      }
      else
      {
            ps = pPixCache->GetpData() + ((height - abs(scaled_stride_rows)-1) * stride);
            if(stride_pixels > 0)               // make a hole on right
                  ps += scaled_stride_pixels * BPP/8;

            pd = pPixCache->GetpData() +  ((height -1) * stride);
            if(stride_pixels <= 0)              // make a hole on the left
                  pd += abs(scaled_stride_pixels) * BPP/8;


            for(int iy=0 ; iy< (height - abs(scaled_stride_rows)) ; iy++)
            {
                  memmove(pd, ps, (width - abs(scaled_stride_pixels)) *BPP/8);

                  ps -= width * BPP/8;
                  pd -= width * BPP/8;
            }
      }







//    Y Pan
      if(source.y != cache_rect.y)
      {
            wxRect sub_dest = dest;
            sub_dest.height = abs(scaled_stride_rows);

            if(stride_rows > 0)                             // pan down
            {
                  sub_dest.y = height - scaled_stride_rows;

            }
            else
            {
                  sub_dest.y = 0;

            }

            //    Get the new bits needed

            //    A little optimization...
            //    No sense in fetching bits that are not part of the ultimate render region
            wxRegionContain rc = Region.Contains(sub_dest);
            if((wxPartRegion == rc) || (wxInRegion == rc))
            {
                  GetAndScaleData(pPixCache->GetpData(), source, source.width, sub_dest, width, cs1d, pan_scale_type_y);
            }
            pPixCache->Update();

//    Update the cached parameters, Y only

            cache_rect.y = source.y;
//          cache_rect = source;
            cache_rect_scaled = dest;
            cached_image_ok = 1;

      }                 // Y Pan




//    X Pan
      if(source.x != cache_rect.x)
      {
            wxRect sub_dest = dest;
            sub_dest.width = abs(scaled_stride_pixels);

            if(stride_pixels > 0)                           // pan right
            {
                  sub_dest.x = width - scaled_stride_pixels;
            }
            else                                                  // pan left
            {
                  sub_dest.x = 0;
            }

            //    Get the new bits needed

            //    A little optimization...
            //    No sense in fetching bits that are not part of the ultimate render region
            wxRegionContain rc = Region.Contains(sub_dest);
            if((wxPartRegion == rc) || (wxInRegion == rc))
            {
                  GetAndScaleData(pPixCache->GetpData(), source, source.width, sub_dest, width, cs1d, pan_scale_type_x);
            }

            pPixCache->Update();

//    Update the cached parameters
            cache_rect = source;
            cache_rect_scaled = dest;
            cached_image_ok = 1;

      }           // X pan

      return true;
}




















int s_dc;

bool Chart_oeuRNC::IsRenderCacheable( wxRect& source, wxRect& dest )
{
      double scale_x = (double)source.width / (double)dest.width;

      if(scale_x <= 1.0)                                        // overzoom
      {
//            if(m_b_cdebug)printf("    MISS<<<>>>GVUC:  Overzoom\n");
            return false;
      }


      //    Using the cache only works for pure binary scale factors......
      if((fabs(scale_x - wxRound(scale_x))) > .0001)
      {
//            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: Not digital scale test 1\n");
            return false;
      }


      //    Scale must be exactly digital...
      if((int)(source.width/dest.width) != (int)wxRound(scale_x))
      {
//            if(m_b_cdebug)printf("   MISS<<<>>>GVUC: Not digital scale test 2\n");
            return false;
      }

      return true;
}


wxBitmap &Chart_oeuRNC::RenderRegionView(const PlugIn_ViewPort& VPoint, const wxRegion &Region)
{
      SetVPRasterParms(VPoint);

      wxRect dest(0,0,VPoint.pix_width, VPoint.pix_height);
//      double factor = ((double)Rsrc.width)/((double)dest.width);
      double factor = m_piraster_scale_factor;
      if(m_b_cdebug)printf("%d RenderRegion  ScaleType:  %d   factor:  %g\n", s_dc++, RENDER_HIDEF, factor );

            //    Invalidate the cache if the scale has changed or the viewport size has changed....
      if((fabs(m_cached_scale_ppm - VPoint.view_scale_ppm) > 1e-9) || (m_last_vprect != dest))
      {
            cached_image_ok = false;
            m_vp_render_last.bValid = false;
      }

      if(pPixCache)
      {
            if((pPixCache->GetWidth() != dest.width) || (pPixCache->GetHeight() != dest.height))
            {
                  delete pPixCache;
                  pPixCache = new PIPixelCache(dest.width, dest.height, BPP);
            }
      }
      else
            pPixCache = new PIPixelCache(dest.width, dest.height, BPP);


      m_cached_scale_ppm = VPoint.view_scale_ppm;
      m_last_vprect = dest;


      if(cached_image_ok)
      {
            //    Anything to do?
           bool bsame_region = (Region == m_last_region);          // only want to do this once

           if((bsame_region) && (Rsrc == cache_rect)  )
           {
              if(m_b_cdebug)printf("  Using Current PixelCache\n");
//              pPixCache->SelectIntoDC(dc);
//              return false;
              pPixCache->BuildBM();
              return pPixCache->GetBitmap();
           }
      }

     m_last_region = Region;


     //     Analyze the region requested
     //     When rendering complex regions, (more than say 4 rectangles)
     //     .OR. small proportions, then rectangle rendering may be faster
     //     Also true  if the scale is less than near unity, or overzoom.
     //     This will be the case for backgrounds of the quilt.


     /*  Update for Version 2.4.0
     This logic seems flawed, at least for quilts which contain charts having non-rectangular coverage areas.
     These quilt regions decompose to ...LOTS... of rectangles, most of which are 1 pixel in height.
     This is very slow, due to the overhead of GetAndScaleData().
     However, remember that overzoom never uses the cache, nor does non-binary scale factors..
     So, we check to see if this is a cacheable render, and that the number of rectangles is "reasonable"
     */

     //     Get the region rectangle count

     int n_rect =0;
     wxRegionIterator upd ( Region ); // get the requested rect list
     while ( upd )
     {
           n_rect++;
           upd ++ ;
     }

     if((!IsRenderCacheable( Rsrc, dest ) && ( n_rect > 4 ) && (n_rect < 20)) || ( factor < 1))
     {
           int ren_type = RENDER_LODEF;

           if(m_b_cdebug)printf("   RenderRegion by rect iterator   n_rect: %d\n", n_rect);

//           PixelCache *pPixCacheTemp = new PixelCache(dest.width, dest.height, BPP);

      //    Decompose the region into rectangles, and fetch them into the target dc
           wxRegionIterator upd ( Region ); // get the requested rect list
           int ir = 0;
           while ( upd )
           {
                 wxRect rect = upd.GetRect();
                 GetAndScaleData(pPixCache->GetpData(), Rsrc, Rsrc.width, rect, dest.width, factor, ren_type);
                 ir++;
                 upd ++ ;
           }

//           delete pPixCache;                           // new cache is OK
//           pPixCache = pPixCacheTemp;

           pPixCache->Update();

      //    Update cache parameters
           cache_rect = Rsrc;
           cache_scale_method = ren_type;
           cached_image_ok = false;//true;            // Never cache this type of render

           pPixCache->BuildBM();
           return pPixCache->GetBitmap();

      //    Select the data into the dc
//           pPixCache->SelectIntoDC(dc);

//           return true;
     }



     //     Default is to try using the cache
     if(m_b_cdebug)printf("  Render Region By GVUC\n");
     GetViewUsingCache(Rsrc, dest, Region, RENDER_HIDEF);

     //    Select the data into the dc
//     pPixCache->SelectIntoDC(dc);
//     return bnewview;

     pPixCache->BuildBM();
     return pPixCache->GetBitmap();
}





bool Chart_oeuRNC::GetView( wxRect& source, wxRect& dest, int scale_type )
{
//    Get and Rescale the data directly into the  PixelCache data buffer
      double factor = ((double)source.width)/((double)dest.width);

      GetAndScaleData(pPixCache->GetpData(), source, source.width, dest, dest.width, factor, scale_type);
      pPixCache->Update();

//    Update cache parameters

      cache_rect = source;
      cache_rect_scaled = dest;
      cache_scale_method = scale_type;

      cached_image_ok = 1;


      return TRUE;
}



bool Chart_oeuRNC::GetAndScaleData(unsigned char *ppn, wxRect& source, int source_stride,
                                   wxRect& dest, int dest_stride, double scale_factor, int scale_type)
{

      unsigned char *s_data = NULL;

      double factor = scale_factor;  //((double)source_stride)/((double)dest.width);
      int Factor =  (int)factor;

      int target_width = (int)wxRound((double)source.width  / factor) ;
      int target_height = (int)wxRound((double)source.height / factor);


      if((target_height == 0) || (target_width == 0))
            return false;

      unsigned char *target_data;
      unsigned char *data;

      data = ppn;
      target_data = data;

      if(factor > 1)                // downsampling
      {

            if(scale_type == RENDER_HIDEF)
            {
//    Allocate a working buffer based on scale factor
                  int blur_factor = wxMax(2, Factor);
                  int wb_size = (source.width) * (blur_factor * 2) * BPP/8 ;
                  s_data = (unsigned char *) malloc( wb_size ); // work buffer
                  unsigned char *pixel;
                  int y_offset;

                  for (int y = dest.y; y < (dest.y + dest.height); y++)
                  {
                  //    Read "blur_factor" lines

                        wxRect s1;
                        s1.x = source.x;
                        s1.y = source.y  + (int)(y * factor);
                        s1.width = source.width;
                        s1.height = blur_factor;
                        GetChartBits_Internal(s1, s_data, 1);

                        target_data = data + (y * dest_stride * BPP/8);

                        for (int x = 0; x < target_width; x++)
                        {
                              unsigned int avgRed = 0 ;
                              unsigned int avgGreen = 0;
                              unsigned int avgBlue = 0;
                              unsigned int pixel_count = 0;
                              unsigned char *pix0 = s_data +  BPP/8 * ((int)( x * factor )) ;
                              y_offset = 0;

                              if((x * Factor) < (Size_X - source.x))
                              {
            // determine average
                                    for ( int y1 = 0 ; y1 < blur_factor ; ++y1 )
                                    {
                                        pixel = pix0 + (BPP/8 * y_offset ) ;
                                        for ( int x1 = 0 ; x1 < blur_factor ; ++x1 )
                                        {
                                            avgRed   += pixel[0] ;
                                            avgGreen += pixel[1] ;
                                            avgBlue  += pixel[2] ;

                                            pixel += BPP/8;

                                            pixel_count++;
                                        }
                                        y_offset += source.width ;
                                    }

                                    target_data[0] = avgRed / pixel_count;     // >> scounter;
                                    target_data[1] = avgGreen / pixel_count;   // >> scounter;
                                    target_data[2] = avgBlue / pixel_count;    // >> scounter;
                                    target_data += BPP/8;
                              }
                              else
                              {
                                    target_data[0] = 0;
                                    target_data[1] = 0;
                                    target_data[2] = 0;
                                    target_data += BPP/8;
                              }

                        }  // for x

                  }  // for y

            }           // SCALE_BILINEAR

            else if (scale_type == RENDER_LODEF)
            {
                        int get_bits_submap = 1;

                        int scaler = 16;

                        if(source.width > 32767)                  // High underscale can exceed signed math bits
                              scaler = 8;

                        int wb_size = (Size_X) * ((/*Factor +*/ 1) * 2) * BPP/8 ;
                        s_data = (unsigned char *) malloc( wb_size ); // work buffer

                        long x_delta = (source.width<<scaler) / target_width;
                        long y_delta = (source.height<<scaler) / target_height;

                        int y = dest.y;                // starting here
                        long ys = dest.y * y_delta;

                        while ( y < dest.y + dest.height)
                        {
                        //    Read 1 line at the right place from the source

                              wxRect s1;
                              s1.x = 0;
                              s1.y = source.y + (ys >> scaler);
                              s1.width = Size_X;
                              s1.height = 1;
                              GetChartBits_Internal(s1, s_data, get_bits_submap);

                              target_data = data + (y * dest_stride * BPP/8) + (dest.x * BPP / 8);

                              long x = (source.x << scaler) + (dest.x * x_delta);
                              long sizex16 = Size_X << scaler;
                              int xt = dest.x;

                              while((xt < dest.x + dest.width) && (x < 0))
                              {
                                    target_data[0] = 0;
                                    target_data[1] = 0;
                                    target_data[2] = 0;

                                    target_data += BPP/8;
                                    x += x_delta;
                                    xt++;
                              }

                              while ((xt < dest.x + dest.width) && ( x < sizex16))
                              {

                                    unsigned char* src_pixel = &s_data[(x>>scaler)*BPP/8];

                                    target_data[0] = src_pixel[0];
                                    target_data[1] = src_pixel[1];
                                    target_data[2] = src_pixel[2];

                                    target_data += BPP/8;
                                    x += x_delta;
                                    xt++;
                              }

                              while(xt < dest.x + dest.width)
                              {
                                    target_data[0] = 0;
                                    target_data[1] = 0;
                                    target_data[2] = 0;

                                    target_data += BPP/8;
                                    xt++;
                              }

                              y++;
                              ys += y_delta;
                        }

            }     // SCALE_SUBSAMP

      }
      else  //factor < 1, overzoom
      {
            int i=0;
            int j=0;
            unsigned char *target_line_start = NULL;
            unsigned char *target_data_x = NULL;
            int y_offset = 0;



                  //    Seems safe enough to read all the required data
                  int sx = wxMax(source.x, 0);

                  //    Although we must adjust (increase) temporary allocation for negative source.x

                  s_data = (unsigned char *) malloc( (sx + source.width) * source.height * BPP/8 );

                  GetChartBits_Internal(source, s_data, 1);


                  int s_data_offset = (int)(1./ m_piraster_scale_factor);
                  s_data_offset /= 2;
                  s_data_offset *= source.width * BPP/8;

                  unsigned char *source_data =  s_data; //+ s_data_offset;


                  j = dest.y;

                  while( j < dest.y + dest.height)
                  {
                      y_offset = (int)(j *m_piraster_scale_factor) * source.width;

                        target_line_start = target_data + (j * dest_stride * BPP / 8);
                        target_data_x = target_line_start + (dest.x * BPP / 8);

                        i = dest.x;
                        while( i < dest.x + dest.width)
                        {

                              memcpy( target_data_x,
                                      source_data + BPP/8*(y_offset + (int)(i * m_piraster_scale_factor)),
                                    BPP/8 );
                              target_data_x += BPP/8;

                              i++;
                        }

                        j++;
                  }


      }

      free(s_data);

      return true;

}

bool Chart_oeuRNC::GetChartBits(wxRect& source, unsigned char *pPix, int sub_samp)
{
    wxCriticalSectionLocker locker(m_critSect);
    
    // temporarily change the palette direction
#ifdef __NEED_PALETTE_REV__
    palette_direction = PaletteRev;
    pPalette = GetPalettePtr(m_mapped_color_index);
#endif

    bool ret_val = GetChartBits_Internal(source, pPix, sub_samp);

#ifdef __NEED_PALETTE_REV__
    palette_direction = PaletteFwd;
    pPalette = GetPalettePtr(m_mapped_color_index);
#endif

    return ret_val;
}


bool Chart_oeuRNC::GetChartBits_Internal(wxRect& source, unsigned char *pPix, int sub_samp)
{
    if(!m_bImageReady)
        DecodeImage();

      int iy;
#define FILL_BYTE 0

//    Decode the KAP file RLL stream into image pPix

      unsigned char *pCP;
      pCP = pPix;

      iy = source.y;

      while (iy < source.y + source.height)
      {
            if((iy >= 0) && (iy < Size_Y))
            {
                    if(source.x >= 0)
                    {
                            if((source.x + source.width) > Size_X)
                            {
                                if((Size_X - source.x) < 0)
                                        memset(pCP, FILL_BYTE, source.width  * BPP/8);
                                else
                                {

                                        BSBGetScanline( pCP,  iy, source.x, Size_X, sub_samp);
                                        memset(pCP + (Size_X - source.x) * BPP/8, FILL_BYTE,
                                               (source.x + source.width - Size_X) * BPP/8);
                                }
                            }
                            else
                                BSBGetScanline( pCP, iy, source.x, source.x + source.width, sub_samp);
                    }
                    else
                    {
                            if((source.width + source.x) >= 0)
                            {
                                // Special case, black on left side
                                //  must ensure that (black fill length % sub_samp) == 0

                                int xfill_corrected = -source.x + (source.x % sub_samp);    //+ve
                                memset(pCP, FILL_BYTE, (xfill_corrected * BPP/8));
                                BSBGetScanline( pCP + (xfill_corrected * BPP/8),  iy, 0,
                                        source.width + source.x , sub_samp);

                            }
                            else
                            {
                                memset(pCP, FILL_BYTE, source.width  * BPP/8);
                            }
                    }
            }

            else              // requested y is off chart
            {
                  memset(pCP, FILL_BYTE, source.width  * BPP/8);

            }

            pCP += source.width * BPP/8 * sub_samp;

            iy += sub_samp;
      }     // while iy


      return true;
}






//-----------------------------------------------------------------------------------------------
//    BSB File Read Support
//-----------------------------------------------------------------------------------------------


#if 0
//-----------------------------------------------------------------------------------------------
//    ReadBSBHdrLine
//
//    Read and return count of a line of BSB header file
//-----------------------------------------------------------------------------------------------

int ChartXTR1::ReadBSBHdrLine(xtr1_inStream* ifs, char* buf, int buf_len_max)

{
      char  read_char;
      char  cr_test;
      int   line_length = 0;
      char  *lbuf;

      lbuf = buf;


      while( !ifs->Eof() && line_length < buf_len_max )
      {
            read_char = ifs->GetC();
            if(read_char == wxEOF)
                  return 0;

            if(0x1A == read_char)
            {
                  ifs->Ungetch( read_char );
                        return(0);
            }

            //    Manage continued lines
            if( read_char == 10 || read_char == 13 )
            {

                  //    Check to see if there is an extra CR
                  cr_test = ifs->GetC( );
                  if(cr_test == 13)
                        cr_test = ifs->GetC( );             // skip any extra CR

                  if( cr_test != 10 && cr_test != 13 )
                        ifs->Ungetch( cr_test );
                  read_char = '\n';
            }

      //    Look for continued lines, indicated by ' ' in first position
            if( read_char == '\n' )
            {
                  cr_test = 0;
                  cr_test = ifs->GetC( );

                  if(cr_test == wxEOF)
                        return 0;

                  if( cr_test != ' ' )
                        {
                              ifs->Ungetch( cr_test );
                              *lbuf = '\0';
                              return line_length;
                        }

      //    Merge out leading spaces
                  while( cr_test == ' ' )
                        cr_test = ifs->GetC( );
                  ifs->Ungetch( cr_test );

      //    Add a comma
                  *lbuf = ',';
                  lbuf++;
            }

            else
            {
                  *lbuf = read_char;
                  lbuf++;
                  line_length++;
            }

      }     // while


      // Terminate line
      *(lbuf-1) = '\0';

      return line_length;
}

#endif

//-----------------------------------------------------------------------
//    Scan a BSB Scan Line from raw data
//      Leaving stream pointer at start of next line
//-----------------------------------------------------------------------
#if 0
int   Chart_oeuRNC::BSBScanScanline(wxInputStream *pinStream )
{
      int nLineMarker,  iPixel = 0;
      unsigned char  byCountMask;
      unsigned char byNext;
      //int coffset;

//      if(1)
      {
//      Read the line number.
            nLineMarker = 0;
            do
            {
                  byNext = pinStream->GetC();
                  nLineMarker = nLineMarker * 128 + (byNext & 0x7f);
            } while( (byNext & 0x80) != 0 );

//      Setup masking values.
            //nValueShift = 7 - nColorSize;
            //byValueMask = (((1 << nColorSize)) - 1) << nValueShift;
            byCountMask = (1 << (7 - nColorSize)) - 1;

//      Read and simulate expansion of runs.

            while( ((byNext = pinStream->GetC()) != 0 ) && (iPixel < Size_X))
            {

                  //int   nPixValue;
                  int nRunCount;
                  //nPixValue = (byNext & byValueMask) >> nValueShift;

                  nRunCount = byNext & byCountMask;

                  while( (byNext & 0x80) != 0 )
                  {
                        byNext = pinStream->GetC();
                        nRunCount = nRunCount * 128 + (byNext & 0x7f);
                  }

                  if( iPixel + nRunCount + 1 > Size_X )
                        nRunCount = Size_X - iPixel - 1;


//          Store nPixValue in the destination
//                  memset(pCL, nPixValue, nRunCount+1);
//                  pCL += nRunCount+1;
                  iPixel += nRunCount+1;
            }
            //coffset = pinStream->TellI();

      }


      return nLineMarker;
}
#endif

void Chart_oeuRNC::FillLineCache( void )
{
      unsigned char *buf = (unsigned char *)malloc((Size_X + 1) * (BPP/8));

      for(int i=0 ; i < Size_Y ; i++)
      {
            BSBGetScanline( buf, i, 0, Size_X, 1);
      }

      free(buf);
}


#if 0
//-----------------------------------------------------------------------
//    Get a BSB Scan Line Using Cache and scan line index if available
//-----------------------------------------------------------------------
int   ChartXTR1::BSBGetScanline( unsigned char *pLineBuf, int y, int xs, int xl, int sub_samp)

{
      int nLineMarker, nValueShift, iPixel = 0;
      unsigned char byValueMask, byCountMask;
      unsigned char byNext;
      CachedLine *pt = NULL;
      unsigned char *pCL;
      int rgbval;
      unsigned char *lp;
      unsigned char *xtemp_line;
      register int ix = xs;

      if(bUseLineCache && pLineCache)
      {
//    Is the requested line in the cache, and valid?
            pt = &pLineCache[y];
            if(!pt->bValid)                                 // not valid, so get it
            {
                  if(pt->pPix)
                        free(pt->pPix);
                  pt->pPix = (unsigned char *)malloc(Size_X);
            }

            xtemp_line = pt->pPix;
      }
      else
            xtemp_line = (unsigned char *)malloc(Size_X);


      if((bUseLineCache && !pt->bValid) || (!bUseLineCache))
      {
          off_t rll_y = ifs_hdr->GetBitmapOffset( y );
          off_t rll_y1 = ifs_hdr->GetBitmapOffset( y + 1 );
          
          if(rll_y == 0)
              return 0;

          if(rll_y1 == 0)
              return 0;

          int thisline_size = rll_y1 - rll_y;

            if(thisline_size > ifs_bufsize)
            {
                ifs_buf = (unsigned char *)realloc(ifs_buf, thisline_size);
                ifs_buf_decrypt = (unsigned char *)realloc(ifs_buf_decrypt, thisline_size);
            }

            if( wxInvalidOffset == ifs_bitmap->SeekI(rll_y, wxFromStart))
                  return 0;

            ifs_bitmap->Read(ifs_buf_decrypt, thisline_size);

//    At this point, the unexpanded, raw line is at *lp, and the expansion destination is xtemp_line

            lp = ifs_buf_decrypt;

//      Read the line number.
            nLineMarker = 0;
            do
            {
                  byNext = *lp++;
                  nLineMarker = nLineMarker * 128 + (byNext & 0x7f);
            } while( (byNext & 0x80) != 0 );

//      Setup masking values.
            nValueShift = 7 - nColorSize;
            byValueMask = (((1 << nColorSize)) - 1) << nValueShift;
            byCountMask = (1 << (7 - nColorSize)) - 1;

//      Read and expand runs.

            pCL = xtemp_line;

            while( ((byNext = *lp++) != 0 ) && (iPixel < Size_X))
            {
                  int   nPixValue;
                  int nRunCount;
                  nPixValue = (byNext & byValueMask) >> nValueShift;

                  nRunCount = byNext & byCountMask;

                  while( (byNext & 0x80) != 0 )
                  {
                        byNext = *lp++;
                        nRunCount = nRunCount * 128 + (byNext & 0x7f);
                  }
/*
                  if( iPixel + nRunCount + 1 > Size_X )     // protection
                        nRunCount = Size_X - iPixel - 1;

                  if(nRunCount < 0)                         // against corrupt data
                      nRunCount = 0;
*/

                  if(( iPixel + nRunCount + 1 > Size_X ) || (nRunCount < 0) )   // against corrupt data
                  {
                    if(!bUseLineCache)
                        free (xtemp_line);
                    return 1;
                  }


//          Store nPixValue in the destination
                  memset(pCL, nPixValue, nRunCount+1);
                  pCL += nRunCount+1;
                  iPixel += nRunCount+1;
            }
      }

      if(bUseLineCache)
            pt->bValid = true;

//          Line is valid, de-reference thru proper pallete directly to target


      if(xl > Size_X-1)
            xl = Size_X-1;

      pCL = xtemp_line + xs;
      unsigned char *prgb = pLineBuf;

      int dest_inc_val_bytes = (BPP/8) * sub_samp;
      ix = xs;
      while(ix < xl-1)
      {
            unsigned char cur_by = *pCL;
            rgbval = (int)(pPalette[cur_by]);
            while((ix < xl-1))
            {
                  if(cur_by != *pCL)
                        break;
                  *((int *)prgb) = rgbval;
                  prgb+=dest_inc_val_bytes ;
                  pCL += sub_samp;
                  ix  += sub_samp;
            }
      }

// Get the last pixel explicitely
//  irrespective of the sub_sampling factor

      if(xs < xl-1)
      {
        unsigned char *pCLast = xtemp_line + (xl - 1);
        unsigned char *prgb_last = pLineBuf + ((xl - 1)-xs) * BPP/8;

        rgbval = (int)(pPalette[*pCLast]);        // last pixel
        unsigned char a = rgbval & 0xff;
        *prgb_last++ = a;
        a = (rgbval >> 8) & 0xff;
        *prgb_last++ = a;
        a = (rgbval >> 16) & 0xff;
        *prgb_last = a;
      }

      if(!bUseLineCache)
          free (xtemp_line);

      return 1;
}

#endif

// could use a larger value for slightly less ram but slower random access,
// this is chosen as it is also the opengl tile size so should work well
#define TILE_SIZE 512

#define FAIL \
do { \
    free(pt->pTileOffset); \
    pt->pTileOffset = NULL; \
    free(pt->pPix); \
    pt->pPix = NULL; \
    pt->bValid = false; \
    return 0; \
    } while(0)
    
    
//-----------------------------------------------------------------------
//    Get a BSB Scan Line Using Cache and scan line index if available
//-----------------------------------------------------------------------
int   Chart_oeuRNC::BSBGetScanline( unsigned char *pLineBuf, int y, int xs, int xl, int sub_samp)

{
      unsigned char *prgb = pLineBuf;
//      int nValueShift, iPixel = 0;
//      unsigned char byValueMask, byCountMask;
//      unsigned char byNext;
//      CachedLine *pt = NULL, cached_line;
      unsigned char *pCL;
      int rgbval;
//      unsigned char *lp;
      int ix = xs;

      
//          de-reference m_imageMap thru proper pallete directly to target

      if(xl > Size_X)
            xl = Size_X;

      if(m_nColors < 16){                       // imageMap is 4 bits per pixel
          size_t lineLengthBytes = ((size_t)(Size_X / 8) * 4) /*+ 1*/ + ((Size_X & 7) * 4 + 7) / 8;

          int offset = lineLengthBytes * y;
          int dest_inc_val_bytes = (BPP/8) * sub_samp;
          ix = xs;
          while(ix < xl-1){
              int byteOffset = offset + ix/2;
              unsigned char cur_by = m_imageMap[byteOffset];
              if(ix & 1){
                  cur_by &= 0x0f;
              }
              else{
                  cur_by &= 0xf0;
                  cur_by = cur_by >> 4;
              }
                  
              rgbval = (int)(pPalette[cur_by]);
              *((int *)prgb) = rgbval;
              prgb+=dest_inc_val_bytes ;
              ix  += sub_samp;
          }
      }

      else{                                     // imageMap is 8 bits per pixel
        size_t lineLengthBytes = ((size_t)(Size_X / 8) * 8) /*+ 1*/ + ((Size_X & 7) * 8 + 7) / 8;
        pCL = &m_imageMap[((lineLengthBytes) * y) + xs];

      //    Optimization for most usual case
        if((BPP == 24) && (1 == sub_samp))
        {
            ix = xs;
            while(ix < xl-1)
            {
                  unsigned char cur_by = *pCL;
                  rgbval = (int)(pPalette[cur_by]);
                  while((ix < xl-1))
                  {
                        if(cur_by != *pCL)
                              break;
                        *((int *)prgb) = rgbval;
                        prgb += 3;
                        pCL ++;
                        ix  ++;
                  }
            }
        }
        else
        {
            int dest_inc_val_bytes = (BPP/8) * sub_samp;
            ix = xs;
            while(ix < xl-1)
            {
                  unsigned char cur_by = *pCL;
                  rgbval = (int)(pPalette[cur_by]);
                  while((ix < xl-1))
                  {
                        if(cur_by != *pCL)
                              break;
                        *((int *)prgb) = rgbval;
                        prgb+=dest_inc_val_bytes ;
                        pCL += sub_samp;
                        ix  += sub_samp;
                  }
            }
        }
      }

#if 0          
#ifdef USE_OLD_CACHE
      pCL = pt->pPix + xs;

      //    Optimization for most usual case
      if((BPP == 24) && (1 == sub_samp))
      {
            ix = xs;
            while(ix < xl-1)
            {
                  unsigned char cur_by = *pCL;
                  rgbval = (int)(pPalette[cur_by]);
                  while((ix < xl-1))
                  {
                        if(cur_by != *pCL)
                              break;
                        *((int *)prgb) = rgbval;
                        prgb += 3;
                        pCL ++;
                        ix  ++;
                  }
            }
      }
      else
      {
            int dest_inc_val_bytes = (BPP/8) * sub_samp;
            ix = xs;
            while(ix < xl-1)
            {
                  unsigned char cur_by = *pCL;
                  rgbval = (int)(pPalette[cur_by]);
                  while((ix < xl-1))
                  {
                        if(cur_by != *pCL)
                              break;
                        *((int *)prgb) = rgbval;
                        prgb+=dest_inc_val_bytes ;
                        pCL += sub_samp;
                        ix  += sub_samp;
                  }
            }
      }

// Get the last pixel explicitely
//  irrespective of the sub_sampling factor

      if(xs < xl-1)
      {
        unsigned char *pCLast = pt->pPix + (xl - 1);
        unsigned char *prgb_last = pLineBuf + ((xl - 1)-xs) * BPP/8;

        rgbval = (int)(pPalette[*pCLast]);        // last pixel
        unsigned char a = rgbval & 0xff;
        *prgb_last++ = a;
        a = (rgbval >> 8) & 0xff;
        *prgb_last++ = a;
        a = (rgbval >> 16) & 0xff;
        *prgb_last = a;
      }
#else
      {
          int tileindex = xs / TILE_SIZE;
          int tileoffset = pt->pTileOffset[tileindex].offset;

          lp = pt->pPix + tileoffset;
          ix = pt->pTileOffset[tileindex].pixel;
      }

nocachestart:
      unsigned int i = 0;

      nValueShift = 7 - nColorSize;
      byValueMask = (((1 << nColorSize)) - 1) << nValueShift;
      byCountMask = (1 << (7 - nColorSize)) - 1;
      int nPixValue = 0; // satisfy stupid compiler warning
      bool bLastPixValueValid = false;

      while(ix < xl - 1 ) {
          byNext = *lp++;

          nPixValue = (byNext & byValueMask) >> nValueShift;
          unsigned int nRunCount;

          if(byNext == 0)
              nRunCount = xl - ix; // corrupted chart, just run to the end
          else {
              nRunCount = byNext & byCountMask;
              while( (byNext & 0x80) != 0 )
              {
                  byNext = *lp++;
                  nRunCount = nRunCount * 128 + (byNext & 0x7f);
              }

              nRunCount++;
          }

          if(ix < xs) {
              if(ix + nRunCount <= (unsigned int)xs) {
                  ix += nRunCount;
                  continue;
              }
              nRunCount -= xs - ix;
              ix = xs;
          }

          if(ix + nRunCount >= (unsigned int)xl) {
              nRunCount = xl - 1 - ix;
              bLastPixValueValid = true;
          }

          rgbval = (int)(pPalette[nPixValue]);

          //    Optimization for most usual case
// currently this is the only case possible...
//          if((BPP == 24) && (1 == sub_samp))
          {
              int count = nRunCount;
              if( count < 16 ) {
                  // for short runs, use simple loop
                  while(count--) {
                      *(uint32_t*)prgb = rgbval;
                      prgb += 3;
                  } 
              } else if(rgbval == 0 || rgbval == 0xffffff) {
                  // optimization for black or white (could work for any gray too)
                  memset(prgb, rgbval, nRunCount*3);
                  prgb += nRunCount*3;
              } else {
                  // note: this may not be optimal for all processors and compilers
                  // I optimized for x86_64 using gcc with -O3
                  // it is probably possible to gain even faster performance by ensuring alignment
                  // to 16 or 32byte boundary (depending on processor) then using inline assembly

#ifdef __ARM_ARCH
//  ARM needs 8 byte alignment for *(uint64_T *x) = *(uint64_T *y)
//  because the compiler will (probably) use the ldrd/strd instuction pair.
//  So, advance the prgb pointer until it is 8-byte aligned,
//  and then carry on if enough bytes are left to process as 64 bit elements
                  
                  if((long)prgb & 7){
                    while(count--) {
                        *(uint32_t*)prgb = rgbval;
                        prgb += 3;
                        if( !((long)prgb & 7) ){
                            if(count >= 8)
                                break;
                        }
                    }
                  }
#endif                  
                  

                  // fill first 24 bytes
                  uint64_t *b = (uint64_t*)prgb;
                  for(int i=0; i < 8; i++) {
                      *(uint32_t*)prgb = rgbval;
                      prgb += 3;
                  }
                  count -= 8;

                  // fill in blocks of 24 bytes
                  uint64_t *y = (uint64_t*)prgb;
                  int count_d8 = count >> 3;
                  prgb += 24*count_d8;
                  while(count_d8--) {
                      *y++ = b[0];
                      *y++ = b[1];
                      *y++ = b[2];
                  }

                  // fill remaining bytes
                  int rcount = count & 0x7;
                  while(rcount--) {
                      *(uint32_t*)prgb = rgbval;
                      prgb += 3;
                  }
              }
          }

          ix += nRunCount;
      }

// Get the last pixel explicitely
//  irrespective of the sub_sampling factor

    if(ix < xl) {
        if(!bLastPixValueValid) {
            byNext = *lp++;
            nPixValue = (byNext & byValueMask) >> nValueShift;
        }
        rgbval = (int)(pPalette[nPixValue]);        // last pixel
        unsigned char a = rgbval & 0xff;

        *prgb++ = a;
        a = (rgbval >> 8) & 0xff;
        *prgb++ = a;
        a = (rgbval >> 16) & 0xff;
        *prgb = a;
    }
#endif
#endif

    return 1;
}





int  *Chart_oeuRNC::GetPalettePtr(BSB_Color_Capability color_index)
{
      if(pPalettes[color_index])
      {

            if(palette_direction == PaletteFwd)
                  return (int *)(pPalettes[color_index]->FwdPalette);
            else
                  return (int *)(pPalettes[color_index]->RevPalette);
      }
      else
            return NULL;
 }


PaletteDir Chart_oeuRNC::GetPaletteDir(void)
 {
  // make a pixel cache
       PIPixelCache *pc = new PIPixelCache(4,4,BPP);
       RGBO r = pc->GetRGBO();
       delete pc;

       if(r == RGB)
             return PaletteFwd;
       else
             return PaletteRev;
 }



int   Chart_oeuRNC::AnalyzeRefpoints(void)
{
      int i,n;
      double elt, elg;

//    Calculate the max/min reference points

      float lonmin = 1000;
      float lonmax = -1000;
      float latmin = 90.;
      float latmax = -90.;

      int plonmin = 100000;
      int plonmax = 0;
      int platmin = 100000;
      int platmax = 0;
      int nlonmin, nlonmax;
      nlonmin =0; nlonmax=0;
      //nlatmax=0; nlatmin=0;

      for(n=0 ; n<nRefpoint ; n++)
      {
            //    Longitude
            if(pRefTable[n].lonr > lonmax)
            {
                  lonmax = pRefTable[n].lonr;
                  plonmax = (int)pRefTable[n].xr;
                  nlonmax = n;
            }
            if(pRefTable[n].lonr < lonmin)
            {
                  lonmin = pRefTable[n].lonr;
                  plonmin = (int)pRefTable[n].xr;
                  nlonmin = n;
            }

            //    Latitude
            if(pRefTable[n].latr < latmin)
            {
                  latmin = pRefTable[n].latr;
                  platmin = (int)pRefTable[n].yr;
                  //nlatmin = n;
            }
            if(pRefTable[n].latr > latmax)
            {
                  latmax = pRefTable[n].latr;
                  platmax = (int)pRefTable[n].yr;
                  //nlatmax = n;
            }
      }

      //    Special case for charts which cross the IDL
      if((lonmin * lonmax) < 0)
      {
            if(pRefTable[nlonmin].xr > pRefTable[nlonmax].xr)
            {
                  //    walk the reference table and add 360 to any longitude which is < 0
                  for(n=0 ; n<nRefpoint ; n++)
                  {
                        if(pRefTable[n].lonr < 0.0)
                              pRefTable[n].lonr += 360.;
                  }

                  //    And recalculate the  min/max
                  lonmin = 1000;
                  lonmax = -1000;

                  for(n=0 ; n<nRefpoint ; n++)
                  {
            //    Longitude
                        if(pRefTable[n].lonr > lonmax)
                        {
                              lonmax = pRefTable[n].lonr;
                              plonmax = (int)pRefTable[n].xr;
                              nlonmax = n;
                        }
                        if(pRefTable[n].lonr < lonmin)
                        {
                              lonmin = pRefTable[n].lonr;
                              plonmin = (int)pRefTable[n].xr;
                              nlonmin = n;
                        }

            //    Latitude
                        if(pRefTable[n].latr < latmin)
                        {
                              latmin = pRefTable[n].latr;
                              platmin = (int)pRefTable[n].yr;
                              //nlatmin = n;
                        }
                        if(pRefTable[n].latr > latmax)
                        {
                              latmax = pRefTable[n].latr;
                              platmax = (int)pRefTable[n].yr;
                              //nlatmax = n;
                        }
                  }
                  m_bIDLcross = true;
            }
      }


//          Build the Control Point Structure, etc
        cPoints.count = nRefpoint;

        cPoints.tx  = (double *)malloc(nRefpoint * sizeof(double));
        cPoints.ty  = (double *)malloc(nRefpoint * sizeof(double));
        cPoints.lon = (double *)malloc(nRefpoint * sizeof(double));
        cPoints.lat = (double *)malloc(nRefpoint * sizeof(double));

        cPoints.pwx = (double *)malloc(12 * sizeof(double));
        cPoints.wpx = (double *)malloc(12 * sizeof(double));
        cPoints.pwy = (double *)malloc(12 * sizeof(double));
        cPoints.wpy = (double *)malloc(12 * sizeof(double));


        //  Find the two REF points that are farthest apart
        double dist_max = 0.;
        int imax = 0;
        int jmax = 0;

        for(i=0 ; i<nRefpoint ; i++)
        {
              for(int j=i+1 ; j < nRefpoint ; j++)
              {
                    double dx = pRefTable[i].xr - pRefTable[j].xr;
                    double dy = pRefTable[i].yr - pRefTable[j].yr;
                    double dist = (dx * dx) + (dy * dy);
                    if(dist > dist_max)
                    {
                          dist_max = dist;
                          imax = i;
                          jmax = j;
                    }
              }
        }

        //  Georef solution depends on projection type

        if(m_projection == PI_PROJECTION_TRANSVERSE_MERCATOR)
        {
              double easting0, easting1, northing0, northing1;
              //  Get the TMerc projection of the two REF points
              toTM(pRefTable[imax].latr, pRefTable[imax].lonr, m_proj_lat, m_proj_lon, &easting0, &northing0);
              toTM(pRefTable[jmax].latr, pRefTable[jmax].lonr, m_proj_lat, m_proj_lon, &easting1, &northing1);

              //  Calculate the scale factor using exact REF point math
              double dx2 =  (pRefTable[jmax].xr - pRefTable[imax].xr) *  (pRefTable[jmax].xr - pRefTable[imax].xr);
              double dy2 =  (pRefTable[jmax].yr - pRefTable[imax].yr) *  (pRefTable[jmax].yr - pRefTable[imax].yr);
              double dn2 =  (northing1 - northing0) * (northing1 - northing0);
              double de2 =  (easting1 - easting0) * (easting1 - easting0);

              m_pi_ppm_avg = sqrt(dx2 + dy2) / sqrt(dn2 + de2);

              //  Set up and solve polynomial solution for pix<->east/north as projected
              // Fill the cpoints structure with pixel points and transformed lat/lon

              for(int n=0 ; n<nRefpoint ; n++)
              {
                    double easting, northing;
                    toTM(pRefTable[n].latr, pRefTable[n].lonr, m_proj_lat, m_proj_lon, &easting, &northing);

                    cPoints.tx[n] = pRefTable[n].xr;
                    cPoints.ty[n] = pRefTable[n].yr;
                    cPoints.lon[n] = easting;
                    cPoints.lat[n] = northing;
              }

        //      Helper parameters
              cPoints.txmax = plonmax;
              cPoints.txmin = plonmin;
              cPoints.tymax = platmax;
              cPoints.tymin = platmin;
              toTM(latmax, lonmax, m_proj_lat, m_proj_lon, &cPoints.lonmax, &cPoints.latmax);
              toTM(latmin, lonmin, m_proj_lat, m_proj_lon, &cPoints.lonmin, &cPoints.latmin);

              cPoints.status = 1;

              Georef_Calculate_Coefficients_Proj(&cPoints);

       }


       else if(m_projection == PI_PROJECTION_MERCATOR)
       {


             double easting0, easting1, northing0, northing1;
              //  Get the Merc projection of the two REF points
             toSM_ECC(pRefTable[imax].latr, pRefTable[imax].lonr, m_proj_lat, m_proj_lon, &easting0, &northing0);
             toSM_ECC(pRefTable[jmax].latr, pRefTable[jmax].lonr, m_proj_lat, m_proj_lon, &easting1, &northing1);

              //  Calculate the scale factor using exact REF point math in x(longitude) direction


             double dx =  (pRefTable[jmax].xr - pRefTable[imax].xr);
             double de =  (easting1 - easting0);

             m_pi_ppm_avg = fabs(dx / de);

             double dx2 =  (pRefTable[jmax].xr - pRefTable[imax].xr) *  (pRefTable[jmax].xr - pRefTable[imax].xr);
             double dy2 =  (pRefTable[jmax].yr - pRefTable[imax].yr) *  (pRefTable[jmax].yr - pRefTable[imax].yr);
             double dn2 =  (northing1 - northing0) * (northing1 - northing0);
             double de2 =  (easting1 - easting0) * (easting1 - easting0);

             m_pi_ppm_avg = sqrt(dx2 + dy2) / sqrt(dn2 + de2);


              //  Set up and solve polynomial solution for pix<->east/north as projected
              // Fill the cpoints structure with pixel points and transformed lat/lon

             for(int n=0 ; n<nRefpoint ; n++)
             {
                   double easting, northing;
                   toSM_ECC(pRefTable[n].latr, pRefTable[n].lonr, m_proj_lat, m_proj_lon, &easting, &northing);

                   cPoints.tx[n] = pRefTable[n].xr;
                   cPoints.ty[n] = pRefTable[n].yr;
                   cPoints.lon[n] = easting;
                   cPoints.lat[n] = northing;
//                   printf(" x: %g  y: %g  east: %g  north: %g\n",pRefTable[n].xr, pRefTable[n].yr, easting, northing);
             }

        //      Helper parameters
             cPoints.txmax = plonmax;
             cPoints.txmin = plonmin;
             cPoints.tymax = platmax;
             cPoints.tymin = platmin;
             toSM_ECC(latmax, lonmax, m_proj_lat, m_proj_lon, &cPoints.lonmax, &cPoints.latmax);
             toSM_ECC(latmin, lonmin, m_proj_lat, m_proj_lon, &cPoints.lonmin, &cPoints.latmin);

             cPoints.status = 1;

             Georef_Calculate_Coefficients_Proj(&cPoints);

//              for(int h=0 ; h < 10 ; h++)
//                    printf("pix to east %d  %g\n",  h, cPoints.pwx[h]);          // pix to lon
 //             for(int h=0 ; h < 10 ; h++)
//                    printf("east to pix %d  %g\n",  h, cPoints.wpx[h]);          // lon to pix


       }

       //   Any other projection had better have embedded coefficients
       else if(bHaveEmbeddedGeoref)
       {
             //   Use a Mercator Projection to get a rough ppm.
             double easting0, easting1, northing0, northing1;
              //  Get the Merc projection of the two REF points
             toSM_ECC(pRefTable[imax].latr, pRefTable[imax].lonr, m_proj_lat, m_proj_lon, &easting0, &northing0);
             toSM_ECC(pRefTable[jmax].latr, pRefTable[jmax].lonr, m_proj_lat, m_proj_lon, &easting1, &northing1);

              //  Calculate the scale factor using exact REF point math in x(longitude) direction

             double dx =  (pRefTable[jmax].xr - pRefTable[imax].xr);
             double de =  (easting1 - easting0);

             m_pi_ppm_avg = fabs(dx / de);

             m_ExtraInfo = _("---<<< Warning:  Chart georef accuracy may be poor. >>>---");
       }

       else
           m_pi_ppm_avg = 1.0;                      // absolute fallback to prevent div-0 errors


/*
       if ((0 != m_Chart_DU ) && (0 != m_Chart_Scale))
       {
             double m_ppm_avg1 = m_Chart_DU * 39.37 / m_Chart_Scale;
             m_ppm_avg1 *= cos(m_proj_lat * PI / 180.);                    // correct to chart centroid

             printf("%g %g\n", m_ppm_avg, m_ppm_avg1);
       }
*/

        // Do a last little test using a synthetic ViewPort of nominal size.....
        PlugIn_ViewPort vp;
        vp.clat = pRefTable[0].latr;
        vp.clon = pRefTable[0].lonr;
        vp.view_scale_ppm = m_pi_ppm_avg;
        vp.skew = 0.;
        vp.pix_width = 1000;
        vp.pix_height = 1000;
//        vp.rv_rect = wxRect(0,0, vp.pix_width, vp.pix_height);
        SetVPRasterParms(vp);


        double xpl_err_max = 0;
        double ypl_err_max = 0;
        //double xpl_err_max_meters, ypl_err_max_meters;
        int px, py;

        int pxref, pyref;
        pxref = (int)pRefTable[0].xr;
        pyref = (int)pRefTable[0].yr;

        for(i=0 ; i<nRefpoint ; i++)
        {
              px = (int)(vp.pix_width/2 + pRefTable[i].xr) - pxref;
              py = (int)(vp.pix_height/2 + pRefTable[i].yr) - pyref;

              vp_pix_to_latlong(vp, px, py, &elt, &elg);

              double lat_error  = elt - pRefTable[i].latr;
              pRefTable[i].ypl_error = lat_error;

              double lon_error = elg - pRefTable[i].lonr;

                    //  Longitude errors could be compounded by prior adjustment to ref points
              if(fabs(lon_error) > 180.)
              {
                    if(lon_error > 0.)
                          lon_error -= 360.;
                    else if(lon_error < 0.)
                          lon_error += 360.;
              }
              pRefTable[i].xpl_error = lon_error;

              if(fabs(pRefTable[i].ypl_error) > fabs(ypl_err_max))
                    ypl_err_max = pRefTable[i].ypl_error;
              if(fabs(pRefTable[i].xpl_error) > fabs(xpl_err_max))
                    xpl_err_max = pRefTable[i].xpl_error;

              //xpl_err_max_meters = fabs(xpl_err_max * 60 * 1852.0);
              //ypl_err_max_meters = fabs(ypl_err_max * 60 * 1852.0);

        }

        m_Chart_Error_Factor = wxMax(fabs(xpl_err_max/(lonmax - lonmin)), fabs(ypl_err_max/(latmax - latmin)));

        //        Good enough for navigation?
        if(m_Chart_Error_Factor > .02)
        {
                    wxString msg = _T("   VP Final Check: Georeference Chart_Error_Factor on chart ");
                    msg.Append(m_FullPath);
                    wxString msg1;
                    msg1.Printf(_T(" is %5g"), m_Chart_Error_Factor);
                    msg.Append(msg1);

                    wxLogMessage(msg);

                    m_ExtraInfo = _("---<<< Warning:  Chart georef accuracy is poor. >>>---");
        }

        //  Try again with my calculated georef
        //  This problem was found on NOAA 514_1.KAP.  The embedded coefficients are just wrong....
        if((m_Chart_Error_Factor > .02) && bHaveEmbeddedGeoref)
        {
              wxString msg = _T("   Trying again with internally calculated georef solution ");
              wxLogMessage(msg);

              bHaveEmbeddedGeoref = false;
              SetVPRasterParms(vp);

              xpl_err_max = 0;
              ypl_err_max = 0;

              pxref = (int)pRefTable[0].xr;
              pyref = (int)pRefTable[0].yr;

              for(i=0 ; i<nRefpoint ; i++)
              {
                    px = (int)(vp.pix_width/2 + pRefTable[i].xr) - pxref;
                    py = (int)(vp.pix_height/2 + pRefTable[i].yr) - pyref;

                    vp_pix_to_latlong(vp, px, py, &elt, &elg);

                    double lat_error  = elt - pRefTable[i].latr;
                    pRefTable[i].ypl_error = lat_error;

                    double lon_error = elg - pRefTable[i].lonr;

                    //  Longitude errors could be compounded by prior adjustment to ref points
                    if(fabs(lon_error) > 180.)
                    {
                          if(lon_error > 0.)
                                lon_error -= 360.;
                          else if(lon_error < 0.)
                                lon_error += 360.;
                    }
                    pRefTable[i].xpl_error = lon_error;

                    if(fabs(pRefTable[i].ypl_error) > fabs(ypl_err_max))
                          ypl_err_max = pRefTable[i].ypl_error;
                    if(fabs(pRefTable[i].xpl_error) > fabs(xpl_err_max))
                          xpl_err_max = pRefTable[i].xpl_error;

                    //xpl_err_max_meters = fabs(xpl_err_max * 60 * 1852.0);
                    //ypl_err_max_meters = fabs(ypl_err_max * 60 * 1852.0);

              }

              m_Chart_Error_Factor = wxMax(fabs(xpl_err_max/(lonmax - lonmin)), fabs(ypl_err_max/(latmax - latmin)));

        //        Good enough for navigation?
              if(m_Chart_Error_Factor > .02)
              {
                    wxString msg = _T("   VP Final Check with internal georef: Georeference Chart_Error_Factor on chart ");
                    msg.Append(m_FullPath);
                    wxString msg1;
                    msg1.Printf(_T(" is %5g"), m_Chart_Error_Factor);
                    msg.Append(msg1);

                    wxLogMessage(msg);

                    m_ExtraInfo = _("---<<< Warning:  Chart georef accuracy is poor. >>>---");
              }
              else
              {
                    wxString msg = _T("   Result: OK, Internal georef solution used.");

                    wxLogMessage(msg);
              }

        }


      return(0);

}


/*
*  Extracted from bsb_io.c - implementation of libbsb reading and writing
*
*  Copyright (C) 2000  Stuart Cunningham <stuart_hc@users.sourceforge.net>
*
*  This library is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  This library is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with this library; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*  $Id: chartimg.cpp,v 1.57 2010/06/25 13:28:46 bdbcat Exp $
*
*/


/**
 * generic polynomial to convert georeferenced lat/lon to char's x/y
 *
 * @param coeff list of polynomial coefficients
 * @param lon longitute or x
 * @param lat latitude or y
 *
 * @return coordinate corresponding to the coeff list
 */
double polytrans( double* coeff, double lon, double lat )
{
    double ret = coeff[0] + coeff[1]*lon + coeff[2]*lat;
    ret += coeff[3]*lon*lon;
    ret += coeff[4]*lon*lat;
    ret += coeff[5]*lat*lat;
    ret += coeff[6]*lon*lon*lon;
    ret += coeff[7]*lon*lon*lat;
    ret += coeff[8]*lon*lat*lat;
    ret += coeff[9]*lat*lat*lat;
//    ret += coeff[10]*lat*lat*lat*lat;
//    ret += coeff[11]*lat*lat*lat*lat*lat;

//    for(int n = 0 ; n < 10 ; n++)
//          printf("  %g\n", coeff[n]);

    return ret;
}



/* --------------------------------------------------------------------------------- */
/****************************************************************************/
/* Convert Lat/Lon <-> Simple Mercator                                      */
/****************************************************************************/
void toSM(double lat, double lon, double lat0, double lon0, double *x, double *y)
{
    double xlon, z, x1, s, y3, s0, y30, y4;

    xlon = lon;

/*  Make sure lon and lon0 are same phase */

    if((lon * lon0 < 0.) && (fabs(lon - lon0) > 180.))
    {
            if(lon < 0.)
                  xlon += 360.;
            else
                  xlon -= 360.;
    }

    z = WGS84_semimajor_axis_meters * mercator_k0;

    x1 = (xlon - lon0) * DEGREE * z;
    *x = x1;

     // y =.5 ln( (1 + sin t) / (1 - sin t) )
    s = sin(lat * DEGREE);
    y3 = (.5 * log((1 + s) / (1 - s))) * z;

    s0 = sin(lat0 * DEGREE);
    y30 = (.5 * log((1 + s0) / (1 - s0))) * z;
    y4 = y3 - y30;

    *y = y4;

}

void
fromSM(double x, double y, double lat0, double lon0, double *lat, double *lon)
{
      double z, s0, y0, lat3, lon1;
      z = WGS84_semimajor_axis_meters * mercator_k0;

// lat = arcsin((e^2(y+y0) - 1)/(e^2(y+y0) + 1))
/*
      double s0 = sin(lat0 * DEGREE);
      double y0 = (.5 * log((1 + s0) / (1 - s0))) * z;

      double e = exp(2 * (y0 + y) / z);
      double e11 = (e - 1)/(e + 1);
      double lat2 =(atan2(e11, sqrt(1 - e11 * e11))) / DEGREE;
*/
//    which is the same as....

      s0 = sin(lat0 * DEGREE);
      y0 = (.5 * log((1 + s0) / (1 - s0))) * z;

      lat3 = (2.0 * atan(exp((y0+y)/z)) - PI/2.) / DEGREE;
      *lat = lat3;


      // lon = x + lon0
      lon1 = lon0 + (x / (DEGREE * z));
      *lon = lon1;

}

void toSM_ECC(double lat, double lon, double lat0, double lon0, double *x, double *y)
{
      double xlon, z, x1, s, s0;

      double falsen;
      double test;
      double ypy;

      double f = 1.0 / WGSinvf;       // WGS84 ellipsoid flattening parameter
      double e2 = 2 * f - f * f;      // eccentricity^2  .006700
      double e = sqrt(e2);

      xlon = lon;


      /*  Make sure lon and lon0 are same phase */

/*  Took this out for 530 pacific....what else is broken??
      if(lon * lon0 < 0.)
      {
            if(lon < 0.)
                  xlon += 360.;
            else
                  xlon -= 360.;
      }

      if(fabs(xlon - lon0) > 180.)
      {
            if(xlon > lon0)
                  xlon -= 360.;
            else
                  xlon += 360.;
      }
*/
      z = WGS84_semimajor_axis_meters * mercator_k0;

      x1 = (xlon - lon0) * DEGREE * z;
      *x = x1;

// y =.5 ln( (1 + sin t) / (1 - sin t) )
      s = sin(lat * DEGREE);
      //y3 = (.5 * log((1 + s) / (1 - s))) * z;

      s0 = sin(lat0 * DEGREE);
      //y30 = (.5 * log((1 + s0) / (1 - s0))) * z;
      //y4 = y3 - y30;
//      *y = y4;

    //Add eccentricity terms

      falsen =  z *log(tan(PI/4 + lat0 * DEGREE / 2)*pow((1. - e * s0)/(1. + e * s0), e/2.));
      test =    z *log(tan(PI/4 + lat  * DEGREE / 2)*pow((1. - e * s )/(1. + e * s ), e/2.));
      ypy = test - falsen;

      *y = ypy;
}

void fromSM_ECC(double x, double y, double lat0, double lon0, double *lat, double *lon)
{
      double z, s0, lon1;
      double falsen, t, xi, esf;

      double f = 1.0 / WGSinvf;       // WGS84 ellipsoid flattening parameter
      double es = 2 * f - f * f;      // eccentricity^2  .006700
      double e = sqrt(es);

      z = WGS84_semimajor_axis_meters * mercator_k0;

      lon1 = lon0 + (x / (DEGREE * z));
      *lon = lon1;

      s0 = sin(lat0 * DEGREE);

      falsen = z *log(tan(PI/4 + lat0 * DEGREE / 2)*pow((1. - e * s0)/(1. + e * s0), e/2.));
      t = exp((y + falsen) / (z));
      xi = (PI / 2.) - 2.0 * atan(t);

      //    Add eccentricity terms

      esf = (es/2. + (5*es*es/24.) + (es*es*es/12.) + (13.0 *es*es*es*es/360.)) * sin( 2 * xi);
      esf += ((7.*es*es/48.) + (29.*es*es*es/240.) + (811.*es*es*es*es/11520.)) * sin (4. * xi);
      esf += ((7.*es*es*es/120.) + (81*es*es*es*es/1120.) + (4279.*es*es*es*es/161280.)) * sin(8. * xi);


     *lat = -(xi + esf) / DEGREE;

}

/****************************************************************************/
/* Convert Lat/Lon <-> Transverse Mercator                                                                  */
/****************************************************************************/



//converts TM coords to lat/long.  Equations from USGS Bulletin 1532
//East Longitudes are positive, West longitudes are negative.
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees.
      //Written by Chuck Gantz- chuck.gantz@globalstar.com
      //Adapted for opencpn by David S. Register - bdbcat@yahoo.com

void  toTM(float lat, float lon, float lat0, float lon0, double *x, double *y)
{
      double A, a, es, f, k0;
      //double lambda, phi, lambda0, phi0;

      double eccPrimeSquared;
      double N, T, C, MM;

      double eccSquared;
      double deg2rad = DEGREE;
      double LongOrigin = lon0;
      double LongOriginRad = LongOrigin * deg2rad;
      double LatRad = lat*deg2rad;
      double LongRad = lon*deg2rad;


// constants for WGS-84
      f = 1.0 / WGSinvf;       /* WGS84 ellipsoid flattening parameter */
      a = WGS84_semimajor_axis_meters;
      k0 = 1.;                /*  Scaling factor    */

      es = 2 * f - f * f;
      eccSquared = es;
      //lambda = lon * DEGREE;
      //phi = lat * DEGREE;

      //phi0 = lat0 * DEGREE;
      //lambda0 = lon0 * DEGREE;

      eccPrimeSquared = (eccSquared)/(1-eccSquared);

      N = a/sqrt(1-eccSquared*sin(LatRad)*sin(LatRad));
      T = tan(LatRad)*tan(LatRad);
      C = eccPrimeSquared*cos(LatRad)*cos(LatRad);
      A = cos(LatRad)*(LongRad-LongOriginRad);

      MM = a*((1   - eccSquared/4          - 3*eccSquared*eccSquared/64  - 5*eccSquared*eccSquared*eccSquared/256)*LatRad
                  - (3*eccSquared/8 + 3*eccSquared*eccSquared/32  + 45*eccSquared*eccSquared*eccSquared/1024)*sin(2*LatRad)
                  + (15*eccSquared*eccSquared/256 + 45*eccSquared*eccSquared*eccSquared/1024)*sin(4*LatRad)
                  - (35*eccSquared*eccSquared*eccSquared/3072)*sin(6*LatRad));

      *x = (double)(k0*N*(A+(1-T+C)*A*A*A/6
                  + (5-18*T+T*T+72*C-58*eccPrimeSquared)*A*A*A*A*A/120));


      *y = (double)(k0*(MM+N*tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
                  + (61-58*T+T*T+600*C-330*eccPrimeSquared)*A*A*A*A*A*A/720)));


}

/* --------------------------------------------------------------------------------- */

//converts lat/long to TM coords.  Equations from USGS Bulletin 1532
//East Longitudes are positive, West longitudes are negative.
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees
      //Written by Chuck Gantz- chuck.gantz@globalstar.com
      //Adapted for opencpn by David S. Register - bdbcat@yahoo.com

void fromTM (double x, double y, double lat0, double lon0, double *lat, double *lon)
{

      double a, k0, es, f, e1, mu;
      //double phi1, phi0, lambda0;

      double eccSquared;
      double eccPrimeSquared;
      double N1, T1, C1, R1, D, MM;
      double phi1Rad;
      double LongOrigin = lon0;
      double rad2deg = 1./DEGREE;

      //phi0 = lat0 * DEGREE;
      //lambda0 = lon0 * DEGREE;

// constants for WGS-84

      f = 1.0 / WGSinvf;       /* WGS84 ellipsoid flattening parameter */
      a = WGS84_semimajor_axis_meters;
      k0 = 1.;                /*  Scaling factor    */

      es = 2 * f - f * f;
      eccSquared = es;

      eccPrimeSquared = (eccSquared)/(1-eccSquared);
      e1 = (1.0 - sqrt(1.0 - es)) / (1.0 + sqrt(1.0 - es));

      MM = y / k0;
      mu = MM/(a*(1-eccSquared/4-3*eccSquared*eccSquared/64-5*eccSquared*eccSquared*eccSquared/256));

      phi1Rad = mu      + (3*e1/2-27*e1*e1*e1/32)*sin(2*mu)
                  + (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
                  +(151*e1*e1*e1/96)*sin(6*mu);
      //phi1 = phi1Rad*rad2deg;

      N1 = a/sqrt(1-eccSquared*sin(phi1Rad)*sin(phi1Rad));
      T1 = tan(phi1Rad)*tan(phi1Rad);
      C1 = eccPrimeSquared*cos(phi1Rad)*cos(phi1Rad);
      R1 = a*(1-eccSquared)/pow(1-eccSquared*sin(phi1Rad)*sin(phi1Rad), 1.5);
      D = x/(N1*k0);

      *lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrimeSquared)*D*D*D*D/24
                  +(61+90*T1+298*C1+45*T1*T1-252*eccPrimeSquared-3*C1*C1)*D*D*D*D*D*D/720);
      *lat = lat0 + (*lat * rad2deg);

      *lon = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrimeSquared+24*T1*T1)
                  *D*D*D*D*D/120)/cos(phi1Rad);
      *lon = LongOrigin + *lon * rad2deg;

}


/* --------------------------------------------------------------------------------- */
/*
      Geodesic Forward and Reverse calculation functions
      Abstracted and adapted from PROJ-4.5.0 by David S.Register (bdbcat@yahoo.com)

      Original source code contains the following license:

      Copyright (c) 2000, Frank Warmerdam

 Permission is hereby granted, free of charge, to any person obtaining a
 copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
*/
/* --------------------------------------------------------------------------------- */




#define DTOL                 1e-12

#define HALFPI  1.5707963267948966
#define SPI     3.14159265359
#define TWOPI   6.2831853071795864769
#define ONEPI   3.14159265358979323846
#define MERI_TOL 1e-9

static double th1,costh1,sinth1,sina12,cosa12,M,N,c1,c2,D,P,s1;
static int merid, signS;

/*   Input/Output from geodesic functions   */
static double al12;           /* Forward azimuth */
static double al21;           /* Back azimuth    */
static double geod_S;         /* Distance        */
static double phi1, lam1, phi2, lam2;

static int ellipse;
static double geod_f;
static double geod_a;
static double es, onef, f, f64, f2, f4;

double adjlon (double lon) {
      if (fabs(lon) <= SPI) return( lon );
      lon += ONEPI;  /* adjust to 0..2pi rad */
      lon -= TWOPI * floor(lon / TWOPI); /* remove integral # of 'revolutions'*/
      lon -= ONEPI;  /* adjust back to -pi..pi rad */
      return( lon );
}



void geod_inv() {
      double      th1,th2,thm,dthm,dlamm,dlam,sindlamm,costhm,sinthm,cosdthm,
      sindthm,L,E,cosd,d,X,Y,T,sind,tandlammp,u,v,D,A,B;


            /*   Stuff the WGS84 projection parameters as necessary
      To avoid having to include <geodesic,h>
            */

      ellipse = 1;
      f = 1.0 / WGSinvf;       /* WGS84 ellipsoid flattening parameter */
      geod_a = WGS84_semimajor_axis_meters;

      es = 2 * f - f * f;
      onef = sqrt(1. - es);
      geod_f = 1 - onef;
      f2 = geod_f/2;
      f4 = geod_f/4;
      f64 = geod_f*geod_f/64;


      if (ellipse) {
            th1 = atan(onef * tan(phi1));
            th2 = atan(onef * tan(phi2));
      } else {
            th1 = phi1;
            th2 = phi2;
      }
      thm = .5 * (th1 + th2);
      dthm = .5 * (th2 - th1);
      dlamm = .5 * ( dlam = adjlon(lam2 - lam1) );
      if (fabs(dlam) < DTOL && fabs(dthm) < DTOL) {
            al12 =  al21 = geod_S = 0.;
            return;
      }
      sindlamm = sin(dlamm);
      costhm = cos(thm);      sinthm = sin(thm);
      cosdthm = cos(dthm);    sindthm = sin(dthm);
      L = sindthm * sindthm + (cosdthm * cosdthm - sinthm * sinthm)
                  * sindlamm * sindlamm;
      d = acos(cosd = 1 - L - L);
      if (ellipse) {
            E = cosd + cosd;
            sind = sin( d );
            Y = sinthm * cosdthm;
            Y *= (Y + Y) / (1. - L);
            T = sindthm * costhm;
            T *= (T + T) / L;
            X = Y + T;
            Y -= T;
            T = d / sind;
            D = 4. * T * T;
            A = D * E;
            B = D + D;
            geod_S = geod_a * sind * (T - f4 * (T * X - Y) +
                        f64 * (X * (A + (T - .5 * (A - E)) * X) -
                        Y * (B + E * Y) + D * X * Y));
            tandlammp = tan(.5 * (dlam - .25 * (Y + Y - E * (4. - X)) *
                        (f2 * T + f64 * (32. * T - (20. * T - A)
                        * X - (B + 4.) * Y)) * tan(dlam)));
      } else {
            geod_S = geod_a * d;
            tandlammp = tan(dlamm);
      }
      u = atan2(sindthm , (tandlammp * costhm));
      v = atan2(cosdthm , (tandlammp * sinthm));
      al12 = adjlon(TWOPI + v - u);
      al21 = adjlon(TWOPI - v - u);
}




void geod_pre(void) {

      /*   Stuff the WGS84 projection parameters as necessary
      To avoid having to include <geodesic,h>
      */
      ellipse = 1;
      f = 1.0 / WGSinvf;       /* WGS84 ellipsoid flattening parameter */
      geod_a = WGS84_semimajor_axis_meters;

      es = 2 * f - f * f;
      onef = sqrt(1. - es);
      geod_f = 1 - onef;
      f2 = geod_f/2;
      f4 = geod_f/4;
      f64 = geod_f*geod_f/64;

      al12 = adjlon(al12); /* reduce to  +- 0-PI */
      signS = fabs(al12) > HALFPI ? 1 : 0;
      th1 = ellipse ? atan(onef * tan(phi1)) : phi1;
      costh1 = cos(th1);
      sinth1 = sin(th1);
      if ((merid = fabs(sina12 = sin(al12)) < MERI_TOL)) {
            sina12 = 0.;
            cosa12 = fabs(al12) < HALFPI ? 1. : -1.;
            M = 0.;
      } else {
            cosa12 = cos(al12);
            M = costh1 * sina12;
      }
      N = costh1 * cosa12;
      if (ellipse) {
            if (merid) {
                  c1 = 0.;
                  c2 = f4;
                  D = 1. - c2;
                  D *= D;
                  P = c2 / D;
            } else {
                  c1 = geod_f * M;
                  c2 = f4 * (1. - M * M);
                  D = (1. - c2)*(1. - c2 - c1 * M);
                  P = (1. + .5 * c1 * M) * c2 / D;
            }
      }
      if (merid) s1 = HALFPI - th1;
      else {
            s1 = (fabs(M) >= 1.) ? 0. : acos(M);
            s1 =  sinth1 / sin(s1);
            s1 = (fabs(s1) >= 1.) ? 0. : acos(s1);
      }
}


void  geod_for(void) {
      double d,sind,u,V,X,ds,cosds,sinds,ss,de;

      ss = 0.;

      if (ellipse) {
            d = geod_S / (D * geod_a);
            if (signS) d = -d;
            u = 2. * (s1 - d);
            V = cos(u + d);
            X = c2 * c2 * (sind = sin(d)) * cos(d) * (2. * V * V - 1.);
            ds = d + X - 2. * P * V * (1. - 2. * P * cos(u)) * sind;
            ss = s1 + s1 - ds;
      } else {
            ds = geod_S / geod_a;
            if (signS) ds = - ds;
      }
      cosds = cos(ds);
      sinds = sin(ds);
      if (signS) sinds = - sinds;
      al21 = N * cosds - sinth1 * sinds;
      if (merid) {
            phi2 = atan( tan(HALFPI + s1 - ds) / onef);
            if (al21 > 0.) {
                  al21 = PI;
                  if (signS)
                        de = PI;
                  else {
                        phi2 = - phi2;
                        de = 0.;
                  }
            } else {
                  al21 = 0.;
                  if (signS) {
                        phi2 = - phi2;
                        de = 0;
                  } else
                        de = PI;
            }
      } else {
            al21 = atan(M / al21);
            if (al21 > 0)
                  al21 += PI;
            if (al12 < 0.)
                  al21 -= PI;
            al21 = adjlon(al21);
            phi2 = atan(-(sinth1 * cosds + N * sinds) * sin(al21) /
                        (ellipse ? onef * M : M));
            de = atan2(sinds * sina12 ,
                       (costh1 * cosds - sinth1 * sinds * cosa12));
            if (ellipse){
                  if (signS)
                        de += c1 * ((1. - c2) * ds +
                                    c2 * sinds * cos(ss));
                  else
                        de -= c1 * ((1. - c2) * ds -
                                    c2 * sinds * cos(ss));
            }
      }
      lam2 = adjlon( lam1 + de );
}







/* --------------------------------------------------------------------------------- */
/*
// Given the lat/long of starting point, and traveling a specified distance,
// at an initial bearing, calculates the lat/long of the resulting location.
// using elliptic earth model.
*/
/* --------------------------------------------------------------------------------- */

void ll_gc_ll(double lat, double lon, double brg, double dist, double *dlat, double *dlon)
{
    /*      Setup the static parameters  */
    phi1 = lat * DEGREE;            /* Initial Position  */
    lam1 = lon * DEGREE;
    al12 = brg * DEGREE;            /* Forward azimuth */
    geod_S = dist * 1852.0;         /* Distance        */

    geod_pre();
    geod_for();

    *dlat = phi2 / DEGREE;
    *dlon = lam2 / DEGREE;
}

void ll_gc_ll_reverse(double lat1, double lon1, double lat2, double lon2,
                     double *bearing, double *dist)
{
    /*      Setup the static parameters  */
    phi1 = lat1 * DEGREE;            /* Initial Position  */
    lam1 = lon1 * DEGREE;
    phi2 = lat2 * DEGREE;
    lam2 = lon2 * DEGREE;

    geod_inv();
    if(al12 < 0)
       al12 += 2*PI;

    if(bearing)
       *bearing = al12 / DEGREE;
    if(dist)
       *dist = geod_S / 1852.0;
}



/* --------------------------------------------------------------------------------- */
/*
// Given the lat/long of starting point and ending point,
// calculates the distance along a geodesic curve, using elliptic earth model.
*/
/* --------------------------------------------------------------------------------- */


double DistGreatCircle(double slat, double slon, double dlat, double dlon)
{

      double d5;
      phi1 = slat * DEGREE;
      lam1 = slon * DEGREE;
      phi2 = dlat * DEGREE;
      lam2 = dlon * DEGREE;

      geod_inv();
      d5 = geod_S / 1852.0;

      return d5;
}

void DistanceBearingMercator(double lat0, double lon0, double lat1, double lon1, double *brg, double *dist)
{
      double east, north, brgt, C;
      double lon0x, lon1x, dlat;
      double mlat0;

      //    Calculate bearing by conversion to SM (Mercator) coordinates, then simple trigonometry

      lon0x = lon0;
      lon1x = lon1;

      //    Make lon points the same phase
      if((lon0x * lon1x) < 0.)
      {
            if(lon0x < 0.)
                  lon0x += 360.;
            else
                  lon1x += 360.;

            //    Choose the shortest distance
            if(fabs(lon0x - lon1x) > 180.)
            {
                  if(lon0x > lon1x)
                        lon0x -= 360.;
                  else
                        lon1x -= 360.;
            }

            //    Make always positive
            lon1x += 360.;
            lon0x += 360.;
      }

      //    In the case of exactly east or west courses
      //    we must make an adjustment if we want true Mercator distances

      //    This idea comes from Thomas(Cagney)
      //    We simply require the dlat to be (slightly) non-zero, and carry on.
      //    MAS022210 for HamishB from 1e-4 && .001 to 1e-9 for better precision
      //    on small latitude diffs
      mlat0 = lat0;
      if(fabs(lat1 - lat0) < 1e-9)
            mlat0 += 1e-9;

      toSM_ECC(lat1, lon1x, mlat0, lon0x, &east, &north);

      C = atan2(east, north);
      dlat = (lat1 - mlat0) * 60.;              // in minutes

      //    Classic formula, which fails for due east/west courses....

      if(dist)
      {
            if(cos(C))
                  *dist = (dlat /cos(C));
            else
                  *dist = DistGreatCircle(lat0, lon0, lat1, lon1);

      }

      //    Calculate the bearing using the un-adjusted original latitudes and Mercator Sailing
      if(brg)
      {
            toSM_ECC(lat1, lon1x, lat0, lon0x, &east, &north);

            C = atan2(east, north);
            brgt = 180. + (C * 180. / PI);
            if (brgt < 0)
                  brgt += 360.;
            if (brgt > 360.)
                  brgt -= 360;

            *brg = brgt;
      }


      //    Alternative formulary
      //  From Roy Williams, "Geometry of Navigation", we have p = Dlo (Dlat/DMP) where p is the departure.
      // Then distance is then:D = sqrt(Dlat^2 + p^2)

/*
          double dlo =  (lon1x - lon0x) * 60.;
          double departure = dlo * dlat / ((north/1852.));

          if(dist)
             *dist = sqrt((dlat*dlat) + (departure * departure));
*/



}


/* --------------------------------------------------------------------------------- */
/*
 * lmfit
 *
 * Solves or minimizes the sum of squares of m nonlinear
 * functions of n variables.
 *
 * From public domain Fortran version
 * of Argonne National Laboratories MINPACK
 *     argonne national laboratory. minpack project. march 1980.
 *     burton s. garbow, kenneth e. hillstrom, jorge j. more
 * C translation by Steve Moshier
 * Joachim Wuttke converted the source into C++ compatible ANSI style
 * and provided a simplified interface
 */


#include <stdlib.h>
#include <math.h>
//#include "lmmin.h"            // all moved to georef.h
#define _LMDIF

///=================================================================================
///     Customized section for openCPN georeferencing

double my_fit_function( double tx, double ty, int n_par, double* p )
{

    double ret = p[0] + p[1]*tx;

    if(n_par > 2)
          ret += p[2]*ty;
    if(n_par > 3)
    {
        ret += p[3]*tx*tx;
        ret += p[4]*tx*ty;
        ret += p[5]*ty*ty;
    }
    if(n_par > 6)
    {
        ret += p[6]*tx*tx*tx;
        ret += p[7]*tx*tx*ty;
        ret += p[8]*tx*ty*ty;
        ret += p[9]*ty*ty*ty;
    }

    return ret;
}

int Georef_Calculate_Coefficients_Onedir(int n_points, int n_par, double *tx, double *ty, double *y, double *p,
                                        double hintp0, double hintp1, double hintp2)
        /*
        n_points : number of sample points
        n_par :  3, 6, or 10,  6 is probably good
        tx:  sample data independent variable 1
        ty:  sample data independent variable 2
        y:   sample data dependent result
        p:   curve fit result coefficients
        */
{

    int i;
    lm_control_type control;
    lm_data_type data;

    lm_initialize_control( &control );


    for(i=0 ; i<12 ; i++)
        p[i] = 0.;

//    memset(p, 0, 12 * sizeof(double));

    //  Insert hints
    p[0] = hintp0;
    p[1] = hintp1;
    p[2] = hintp2;

    data.user_func = my_fit_function;
    data.user_tx = tx;
    data.user_ty = ty;
    data.user_y = y;
    data.n_par = n_par;
    data.print_flag = 0;

// perform the fit:

            lm_minimize( n_points, n_par, p, lm_evaluate_default, lm_print_default,
                         &data, &control );

// print results:
//            printf( "status: %s after %d evaluations\n",
//                    lm_infmsg[control.info], control.nfev );

            //      Control.info results [1,2,3] are success, other failure
            return control.info;
}

int Georef_Calculate_Coefficients(struct GeoRef *cp, int nlin_lon)
{
    int  r1, r2, r3, r4;
    int mp;
    int mp_lat, mp_lon;

    double *pnull;
    double *px;

    //      Zero out the points
    cp->pwx[6] = cp->pwy[6] = cp->wpx[6] = cp->wpy[6] = 0.;
    cp->pwx[7] = cp->pwy[7] = cp->wpx[7] = cp->wpy[7] = 0.;
    cp->pwx[8] = cp->pwy[8] = cp->wpx[8] = cp->wpy[8] = 0.;
    cp->pwx[9] = cp->pwy[9] = cp->wpx[9] = cp->wpy[9] = 0.;
    cp->pwx[3] = cp->pwy[3] = cp->wpx[3] = cp->wpy[3] = 0.;
    cp->pwx[4] = cp->pwy[4] = cp->wpx[4] = cp->wpy[4] = 0.;
    cp->pwx[5] = cp->pwy[5] = cp->wpx[5] = cp->wpy[5] = 0.;
    cp->pwx[0] = cp->pwy[0] = cp->wpx[0] = cp->wpy[0] = 0.;
    cp->pwx[1] = cp->pwy[1] = cp->wpx[1] = cp->wpy[1] = 0.;
    cp->pwx[2] = cp->pwy[2] = cp->wpx[2] = cp->wpy[2] = 0.;

    mp = 3;

    switch (cp->order)
    {
    case 1:
        mp = 3;
        break;
    case 2:
        mp = 6;
        break;
    case 3:
        mp = 10;
        break;
    default:
        mp = 3;
        break;
    }

    mp_lat = mp;

    //      Force linear fit for longitude if nlin_lon > 0
    mp_lon = mp;
    if(nlin_lon)
          mp_lon = 2;

    //      Make a dummay double array
    pnull = (double *)calloc(cp->count * sizeof(double), 1);

    //      pixel(tx,ty) to (lat,lon)
    //      Calculate and use a linear equation for p[0..2] to hint the solver

    r1 = Georef_Calculate_Coefficients_Onedir(cp->count, mp_lon, cp->tx, cp->ty, cp->lon, cp->pwx,
                                         cp->lonmin - (cp->txmin * (cp->lonmax - cp->lonmin) /(cp->txmax - cp->txmin)),
                                         (cp->lonmax - cp->lonmin) /(cp->txmax - cp->txmin),
                                         0.);

    //      if blin_lon > 0, force cross terms in latitude equation coefficients to be zero by making lat not dependent on tx,
    if(nlin_lon)
          px = pnull;
    else
          px = cp->tx;

    r2 = Georef_Calculate_Coefficients_Onedir(cp->count, mp_lat, px, cp->ty, cp->lat, cp->pwy,
                                         cp->latmin - (cp->tymin * (cp->latmax - cp->latmin) /(cp->tymax - cp->tymin)),
                                         0.,
                                         (cp->latmax - cp->latmin) /(cp->tymax - cp->tymin));

    //      (lat,lon) to pixel(tx,ty)
    //      Calculate and use a linear equation for p[0..2] to hint the solver

    r3 = Georef_Calculate_Coefficients_Onedir(cp->count, mp_lon, cp->lon, cp->lat, cp->tx, cp->wpx,
                                         cp->txmin - ((cp->txmax - cp->txmin) * cp->lonmin / (cp->lonmax - cp->lonmin)),
                                         (cp->txmax - cp->txmin) / (cp->lonmax - cp->lonmin),
                                         0.0);

    //      if blin_lon > 0, force cross terms in latitude equation coefficients to be zero by making ty not dependent on lon,
    if(nlin_lon)
          px = pnull;
    else
          px = cp->lon;

    r4 = Georef_Calculate_Coefficients_Onedir(cp->count, mp_lat, pnull/*cp->lon*/, cp->lat, cp->ty, cp->wpy,
                                         cp->tymin - ((cp->tymax - cp->tymin) * cp->latmin / (cp->latmax - cp->latmin)),
                                        0.0,
                                        (cp->tymax - cp->tymin) / (cp->latmax - cp->latmin));

    free (pnull);

    if((r1) && (r1 < 4) && (r2) && (r2 < 4) && (r3) && (r3 < 4) && (r4) && (r4 < 4))
        return 0;
    else
        return 1;

}



int Georef_Calculate_Coefficients_Proj(struct GeoRef *cp)
{
      int  r1, r2, r3, r4;
      int mp;

    //      Zero out the points
      cp->pwx[6] = cp->pwy[6] = cp->wpx[6] = cp->wpy[6] = 0.;
      cp->pwx[7] = cp->pwy[7] = cp->wpx[7] = cp->wpy[7] = 0.;
      cp->pwx[8] = cp->pwy[8] = cp->wpx[8] = cp->wpy[8] = 0.;
      cp->pwx[9] = cp->pwy[9] = cp->wpx[9] = cp->wpy[9] = 0.;
      cp->pwx[3] = cp->pwy[3] = cp->wpx[3] = cp->wpy[3] = 0.;
      cp->pwx[4] = cp->pwy[4] = cp->wpx[4] = cp->wpy[4] = 0.;
      cp->pwx[5] = cp->pwy[5] = cp->wpx[5] = cp->wpy[5] = 0.;
      cp->pwx[0] = cp->pwy[0] = cp->wpx[0] = cp->wpy[0] = 0.;
      cp->pwx[1] = cp->pwy[1] = cp->wpx[1] = cp->wpy[1] = 0.;
      cp->pwx[2] = cp->pwy[2] = cp->wpx[2] = cp->wpy[2] = 0.;

      mp = 3;


    //      pixel(tx,ty) to (northing,easting)
    //      Calculate and use a linear equation for p[0..2] to hint the solver

      r1 = Georef_Calculate_Coefficients_Onedir(cp->count, mp, cp->tx, cp->ty, cp->lon, cp->pwx,
                  cp->lonmin - (cp->txmin * (cp->lonmax - cp->lonmin) /(cp->txmax - cp->txmin)),
                                (cp->lonmax - cp->lonmin) /(cp->txmax - cp->txmin),
                                 0.);



      r2 = Georef_Calculate_Coefficients_Onedir(cp->count, mp, cp->tx, cp->ty, cp->lat, cp->pwy,
                  cp->latmin - (cp->tymin * (cp->latmax - cp->latmin) /(cp->tymax - cp->tymin)),
                                0.,
                                (cp->latmax - cp->latmin) /(cp->tymax - cp->tymin));

    //      (northing/easting) to pixel(tx,ty)
    //      Calculate and use a linear equation for p[0..2] to hint the solver

      r3 = Georef_Calculate_Coefficients_Onedir(cp->count, mp, cp->lon, cp->lat, cp->tx, cp->wpx,
                  cp->txmin - ((cp->txmax - cp->txmin) * cp->lonmin / (cp->lonmax - cp->lonmin)),
                                (cp->txmax - cp->txmin) / (cp->lonmax - cp->lonmin),
                                 0.0);

      r4 = Georef_Calculate_Coefficients_Onedir(cp->count, mp, cp->lon, cp->lat, cp->ty, cp->wpy,
                  cp->tymin - ((cp->tymax - cp->tymin) * cp->latmin / (cp->latmax - cp->latmin)),
                                0.0,
                                (cp->tymax - cp->tymin) / (cp->latmax - cp->latmin));


      if((r1) && (r1 < 4) && (r2) && (r2 < 4) && (r3) && (r3 < 4) && (r4) && (r4 < 4))
            return 0;
      else
            return 1;

}


/*
 * This file contains default implementation of the evaluate and printout
 * routines. In most cases, customization of lmfit can be done by modifying
 * these two routines. Either modify them here, or copy and rename them.
 */

void lm_evaluate_default( double* par, int m_dat, double* fvec,
                          void *data, int *info )
/*
        *    par is an input array. At the end of the minimization, it contains
        *        the approximate solution vector.
        *
        *    m_dat is a positive integer input variable set to the number
        *      of functions.
        *
        *    fvec is an output array of length m_dat which contains the function
        *        values the square sum of which ought to be minimized.
        *
        *    data is a read-only pointer to lm_data_type, as specified by lmuse.h.
        *
        *      info is an integer output variable. If set to a negative value, the
        *        minimization procedure will stop.
 */
{
    int i;
    lm_data_type *mydata;
    mydata = (lm_data_type*)data;

    for (i=0; i<m_dat; i++)
    {
        fvec[i] = mydata->user_y[i] - mydata->user_func( mydata->user_tx[i], mydata->user_ty[i], mydata->n_par, par);
    }
    *info = *info; /* to prevent a 'unused variable' warning */
    /* if <parameters drifted away> { *info = -1; } */
}

void lm_print_default( int n_par, double* par, int m_dat, double* fvec,
                       void *data, int iflag, int iter, int nfev )
/*
        *       data  : for soft control of printout behaviour, add control
        *                 variables to the data struct
        *       iflag : 0 (init) 1 (outer loop) 2(inner loop) -1(terminated)
        *       iter  : outer loop counter
        *       nfev  : number of calls to *evaluate
 */
{
    double f, y, tx, ty;
    int i;
    lm_data_type *mydata;
    mydata = (lm_data_type*)data;

    if(mydata->print_flag)
    {
        if (iflag==2) {
            printf ("trying step in gradient direction\n");
        } else if (iflag==1) {
            printf ("determining gradient (iteration %d)\n", iter);
        } else if (iflag==0) {
            printf ("starting minimization\n");
        } else if (iflag==-1) {
            printf ("terminated after %d evaluations\n", nfev);
        }

        printf( "  par: " );
        for( i=0; i<n_par; ++i )
            printf( " %12g", par[i] );
        printf ( " => norm: %12g\n", lm_enorm( m_dat, fvec ) );

        if ( iflag == -1 ) {
            printf( "  fitting data as follows:\n" );
            for( i=0; i<m_dat; ++i ) {
                tx = (mydata->user_tx)[i];
                ty = (mydata->user_ty)[i];
                y = (mydata->user_y)[i];
                f = mydata->user_func( tx, ty, mydata->n_par, par );
                printf( "    tx[%2d]=%8g     ty[%2d]=%8g     y=%12g fit=%12g     residue=%12g\n",
                        i, tx, i, ty, y, f, y-f );
            }
        }
    }       // if print_flag
}





///=================================================================================

/* *********************** high-level interface **************************** */


void lm_initialize_control( lm_control_type *control )
{
    control->maxcall = 100;
    control->epsilon = 1.e-10; //1.e-14;
    control->stepbound = 100; //100.;
    control->ftol = 1.e-14;
    control->xtol = 1.e-14;
    control->gtol = 1.e-14;
}

void lm_minimize( int m_dat, int n_par, double* par,
                  lm_evaluate_ftype *evaluate, lm_print_ftype *printout,
                  void *data, lm_control_type *control )
{

// *** allocate work space.

    double *fvec, *diag, *fjac, *qtf, *wa1, *wa2, *wa3, *wa4;
    int *ipvt;

    int n = n_par;
    int m = m_dat;

    fvec = (double*) malloc(  m*sizeof(double));
    diag = (double*) malloc(n*  sizeof(double));
    qtf =  (double*) malloc(n*  sizeof(double));
    fjac = (double*) malloc(n*m*sizeof(double));
    wa1 =  (double*) malloc(n*  sizeof(double));
    wa2 =  (double*) malloc(n*  sizeof(double));
    wa3 =  (double*) malloc(n*  sizeof(double));
    wa4 =  (double*) malloc(  m*sizeof(double));
    ipvt = (int*)    malloc(n*  sizeof(int));

    if (!(fvec)    ||
          !(diag ) ||
          !(qtf )  ||
          !(fjac ) ||
          !(wa1 )  ||
          !(wa2 )  ||
          !(wa3 )  ||
          !(wa4 )  ||
          !(ipvt )) {
        control->info = 9;
        return;
          }

// *** perform fit.

          control->info = 0;
          control->nfev = 0;

    // this goes through the modified legacy interface:
          lm_lmdif( m, n, par, fvec, control->ftol, control->xtol, control->gtol,
                    control->maxcall*(n+1), control->epsilon, diag, 1,
                    control->stepbound, &(control->info),
                    &(control->nfev), fjac, ipvt, qtf, wa1, wa2, wa3, wa4,
                    evaluate, printout, data );

          (*printout)( n, par, m, fvec, data, -1, 0, control->nfev );
          control->fnorm = lm_enorm(m, fvec);
          if (control->info < 0 ) control->info = 10;

// *** clean up.

          free(fvec);
          free(diag);
          free(qtf);
          free(fjac);
          free(wa1);
          free(wa2);
          free(wa3 );
          free(wa4);
          free(ipvt);
}


// ***** the following messages are referenced by the variable info.
/*
char *lm_infmsg[] = {
    "improper input parameters",
    "the relative error in the sum of squares is at most tol",
    "the relative error between x and the solution is at most tol",
    "both errors are at most tol",
    "fvec is orthogonal to the columns of the jacobian to machine precision",
    "number of calls to fcn has reached or exceeded 200*(n+1)",
    "ftol is too small. no further reduction in the sum of squares is possible",
    "xtol too small. no further improvement in approximate solution x possible",
    "gtol too small. no further improvement in approximate solution x possible",
    "not enough memory",
    "break requested within function evaluation"
};

char *lm_shortmsg[] = {
    "invalid input",
    "success (f)",
    "success (p)",
    "success (f,p)",
    "degenerate",
    "call limit",
    "failed (f)",
    "failed (p)",
    "failed (o)",
    "no memory",
    "user break"
};
*/

/* ************************** implementation ******************************* */


#define BUG 0
#if BUG
#include <stdio.h>
#endif

// the following values seem good for an x86:
//#define LM_MACHEP .555e-16 /* resolution of arithmetic */
//#define LM_DWARF  9.9e-324 /* smallest nonzero number */
// the follwoing values should work on any machine:
 #define LM_MACHEP 1.2e-16
 #define LM_DWARF 1.0e-38

// the squares of the following constants shall not under/overflow:
// these values seem good for an x86:
//#define LM_SQRT_DWARF 1.e-160
//#define LM_SQRT_GIANT 1.e150
// the following values should work on any machine:
 #define LM_SQRT_DWARF 3.834e-20
 #define LM_SQRT_GIANT 1.304e19


void lm_qrfac( int m, int n, double* a, int pivot, int* ipvt,
               double* rdiag, double* acnorm, double* wa);
void lm_qrsolv(int n, double* r, int ldr, int* ipvt, double* diag,
               double* qtb, double* x, double* sdiag, double* wa);
void lm_lmpar( int n, double* r, int ldr, int* ipvt, double* diag, double* qtb,
               double delta, double* par, double* x, double* sdiag,
               double* wa1, double* wa2);

#define MIN(a,b) (((a)<=(b)) ? (a) : (b))
#define MAX(a,b) (((a)>=(b)) ? (a) : (b))
#define SQR(x)   (x)*(x)


// ***** the low-level legacy interface for full control.

void lm_lmdif( int m, int n, double* x, double* fvec, double ftol, double xtol,
               double gtol, int maxfev, double epsfcn, double* diag, int mode,
               double factor, int *info, int *nfev,
               double* fjac, int* ipvt, double* qtf,
               double* wa1, double* wa2, double* wa3, double* wa4,
               lm_evaluate_ftype *evaluate, lm_print_ftype *printout,
               void *data )
{
/*
    *   the purpose of lmdif is to minimize the sum of the squares of
    *   m nonlinear functions in n variables by a modification of
    *   the levenberg-marquardt algorithm. the user must provide a
    *   subroutine evaluate which calculates the functions. the jacobian
    *   is then calculated by a forward-difference approximation.
    *
    *   the multi-parameter interface lm_lmdif is for users who want
    *   full control and flexibility. most users will be better off using
    *   the simpler interface lmfit provided above.
    *
    *   the parameters are the same as in the legacy FORTRAN implementation,
 *   with the following exceptions:
    *      the old parameter ldfjac which gave leading dimension of fjac has
    *        been deleted because this C translation makes no use of two-
    *        dimensional arrays;
    *      the old parameter nprint has been deleted; printout is now controlled
    *        by the user-supplied routine *printout;
    *      the parameter field *data and the function parameters *evaluate and
    *        *printout have been added; they help avoiding global variables.
    *
 *   parameters:
    *
    *    m is a positive integer input variable set to the number
    *      of functions.
    *
    *    n is a positive integer input variable set to the number
    *      of variables. n must not exceed m.
    *
    *    x is an array of length n. on input x must contain
    *      an initial estimate of the solution vector. on output x
    *      contains the final estimate of the solution vector.
    *
    *    fvec is an output array of length m which contains
    *      the functions evaluated at the output x.
    *
    *    ftol is a nonnegative input variable. termination
    *      occurs when both the actual and predicted relative
    *      reductions in the sum of squares are at most ftol.
    *      therefore, ftol measures the relative error desired
    *      in the sum of squares.
    *
    *    xtol is a nonnegative input variable. termination
    *      occurs when the relative error between two consecutive
    *      iterates is at most xtol. therefore, xtol measures the
    *      relative error desired in the approximate solution.
    *
    *    gtol is a nonnegative input variable. termination
    *      occurs when the cosine of the angle between fvec and
    *      any column of the jacobian is at most gtol in absolute
    *      value. therefore, gtol measures the orthogonality
    *      desired between the function vector and the columns
    *      of the jacobian.
    *
    *    maxfev is a positive integer input variable. termination
    *      occurs when the number of calls to lm_fcn is at least
    *      maxfev by the end of an iteration.
    *
    *    epsfcn is an input variable used in determining a suitable
    *      step length for the forward-difference approximation. this
    *      approximation assumes that the relative errors in the
    *      functions are of the order of epsfcn. if epsfcn is less
    *      than the machine precision, it is assumed that the relative
    *      errors in the functions are of the order of the machine
    *      precision.
    *
    *    diag is an array of length n. if mode = 1 (see below), diag is
    *        internally set. if mode = 2, diag must contain positive entries
    *        that serve as multiplicative scale factors for the variables.
    *
    *    mode is an integer input variable. if mode = 1, the
    *      variables will be scaled internally. if mode = 2,
    *      the scaling is specified by the input diag. other
    *      values of mode are equivalent to mode = 1.
    *
    *    factor is a positive input variable used in determining the
    *      initial step bound. this bound is set to the product of
    *      factor and the euclidean norm of diag*x if nonzero, or else
    *      to factor itself. in most cases factor should lie in the
    *      interval (.1,100.). 100. is a generally recommended value.
    *
    *    info is an integer output variable that indicates the termination
 *        status of lm_lmdif as follows:
    *
    *        info < 0  termination requested by user-supplied routine *evaluate;
    *
    *      info = 0  improper input parameters;
    *
    *      info = 1  both actual and predicted relative reductions
    *              in the sum of squares are at most ftol;
    *
    *      info = 2  relative error between two consecutive iterates
    *              is at most xtol;
    *
    *      info = 3  conditions for info = 1 and info = 2 both hold;
    *
    *      info = 4  the cosine of the angle between fvec and any
    *              column of the jacobian is at most gtol in
    *              absolute value;
    *
    *      info = 5  number of calls to lm_fcn has reached or
    *              exceeded maxfev;
    *
    *      info = 6  ftol is too small. no further reduction in
    *              the sum of squares is possible;
    *
    *      info = 7  xtol is too small. no further improvement in
    *              the approximate solution x is possible;
    *
    *      info = 8  gtol is too small. fvec is orthogonal to the
    *              columns of the jacobian to machine precision;
    *
    *    nfev is an output variable set to the number of calls to the
    *        user-supplied routine *evaluate.
    *
    *    fjac is an output m by n array. the upper n by n submatrix
    *      of fjac contains an upper triangular matrix r with
    *      diagonal elements of nonincreasing magnitude such that
    *
    *           t     t       t
    *          p *(jac *jac)*p = r *r,
    *
    *      where p is a permutation matrix and jac is the final
    *      calculated jacobian. column j of p is column ipvt(j)
    *      (see below) of the identity matrix. the lower trapezoidal
    *      part of fjac contains information generated during
    *      the computation of r.
    *
    *    ipvt is an integer output array of length n. ipvt
    *      defines a permutation matrix p such that jac*p = q*r,
    *      where jac is the final calculated jacobian, q is
    *      orthogonal (not stored), and r is upper triangular
    *      with diagonal elements of nonincreasing magnitude.
    *      column j of p is column ipvt(j) of the identity matrix.
    *
    *    qtf is an output array of length n which contains
    *      the first n elements of the vector (q transpose)*fvec.
    *
    *    wa1, wa2, and wa3 are work arrays of length n.
    *
    *    wa4 is a work array of length m.
    *
 *   the following parameters are newly introduced in this C translation:
    *
    *      evaluate is the name of the subroutine which calculates the functions.
    *        a default implementation lm_evaluate_default is provided in lm_eval.c;
    *        alternatively, evaluate can be provided by a user calling program.
 *        it should be written as follows:
    *
    *        void evaluate ( double* par, int m_dat, double* fvec,
    *                       void *data, int *info )
    *        {
    *           // for ( i=0; i<m_dat; ++i )
    *           //     calculate fvec[i] for given parameters par;
    *           // to stop the minimization,
    *           //     set *info to a negative integer.
    *        }
    *
    *      printout is the name of the subroutine which nforms about fit progress.
    *        a default implementation lm_print_default is provided in lm_eval.c;
    *        alternatively, printout can be provided by a user calling program.
 *        it should be written as follows:
    *
    *        void printout ( int n_par, double* par, int m_dat, double* fvec,
    *                       void *data, int iflag, int iter, int nfev )
    *        {
    *           // iflag : 0 (init) 1 (outer loop) 2(inner loop) -1(terminated)
    *           // iter  : outer loop counter
    *           // nfev  : number of calls to *evaluate
    *        }
    *
    *      data is an input pointer to an arbitrary structure that is passed to
    *        evaluate. typically, it contains experimental data to be fitted.
    *
 */
    int i, iter, j;
    double actred, delta, dirder, eps, fnorm, fnorm1, gnorm, par, pnorm,
    prered, ratio, step, sum, temp, temp1, temp2, temp3, xnorm;
    static double p1 = 0.1;
    static double p5 = 0.5;
    static double p25 = 0.25;
    static double p75 = 0.75;
    static double p0001 = 1.0e-4;

    *nfev = 0; // function evaluation counter
    iter = 1;  // outer loop counter
    par = 0;   // levenberg-marquardt parameter
    delta = 0; // just to prevent a warning (initialization within if-clause)
    xnorm = 0; // dito

    temp = MAX(epsfcn,LM_MACHEP);
    eps = sqrt(temp); // used in calculating the Jacobian by forward differences

// *** check the input parameters for errors.

    if ( (n <= 0) || (m < n) || (ftol < 0.)
          || (xtol < 0.) || (gtol < 0.) || (maxfev <= 0) || (factor <= 0.) )
    {
        *info = 0; // invalid parameter
        return;
    }
    if ( mode == 2 )  /* scaling by diag[] */
    {
        for ( j=0; j<n; j++ )  /* check for nonpositive elements */
        {
            if ( diag[j] <= 0.0 )
            {
                *info = 0; // invalid parameter
                return;
            }
        }
    }
#if BUG
    printf( "lmdif\n" );
#endif

// *** evaluate the function at the starting point and calculate its norm.

    *info = 0;
    (*evaluate)( x, m, fvec, data, info );
    (*printout)( n, x, m, fvec, data, 0, 0, ++(*nfev) );
    if ( *info < 0 ) return;
    fnorm = lm_enorm(m,fvec);

// *** the outer loop.

    do {
#if BUG
        printf( "lmdif/ outer loop iter=%d nfev=%d fnorm=%.10e\n",
                iter, *nfev, fnorm );
#endif

// O** calculate the jacobian matrix.

        for ( j=0; j<n; j++ )
{
    temp = x[j];
    step = eps * fabs(temp);
    if (step == 0.) step = eps;
    x[j] = temp + step;
    *info = 0;
    (*evaluate)( x, m, wa4, data, info );
    (*printout)( n, x, m, wa4, data, 1, iter, ++(*nfev) );
    if ( *info < 0 ) return;  // user requested break
    x[j] = temp;
    for ( i=0; i<m; i++ )
        fjac[j*m+i] = (wa4[i] - fvec[i]) / step;
}
#if BUG>1
        // DEBUG: print the entire matrix
        for ( i=0; i<m; i++ )
{
    for ( j=0; j<n; j++ )
        printf( "%.5e ", y[j*m+i] );
    printf( "\n" );
}
#endif

// O** compute the qr factorization of the jacobian.

        lm_qrfac( m, n, fjac, 1, ipvt, wa1, wa2, wa3);

// O** on the first iteration ...

        if (iter == 1)
{
    if (mode != 2)
//      ... scale according to the norms of the columns of the initial jacobian.
    {
        for ( j=0; j<n; j++ )
        {
            diag[j] = wa2[j];
            if ( wa2[j] == 0. )
                diag[j] = 1.;
        }
    }

//      ... calculate the norm of the scaled x and
//          initialize the step bound delta.

    for ( j=0; j<n; j++ )
        wa3[j] = diag[j] * x[j];

    xnorm = lm_enorm( n, wa3 );
    delta = factor*xnorm;
    if (delta == 0.)
        delta = factor;
}

// O** form (q transpose)*fvec and store the first n components in qtf.

        for ( i=0; i<m; i++ )
            wa4[i] = fvec[i];

        for ( j=0; j<n; j++ )
{
    temp3 = fjac[j*m+j];
    if (temp3 != 0.)
    {
        sum = 0;
        for ( i=j; i<m; i++ )
            sum += fjac[j*m+i] * wa4[i];
        temp = -sum / temp3;
        for ( i=j; i<m; i++ )
            wa4[i] += fjac[j*m+i] * temp;
    }
    fjac[j*m+j] = wa1[j];
    qtf[j] = wa4[j];
}

// O** compute the norm of the scaled gradient and test for convergence.

        gnorm = 0;
        if ( fnorm != 0 )
{
    for ( j=0; j<n; j++ )
    {
        if ( wa2[ ipvt[j] ] == 0 ) continue;

        sum = 0.;
        for ( i=0; i<=j; i++ )
            sum += fjac[j*m+i] * qtf[i] / fnorm;
        gnorm = MAX( gnorm, fabs(sum/wa2[ ipvt[j] ]) );
    }
}

        if ( gnorm <= gtol )
{
    *info = 4;
    return;
}

// O** rescale if necessary.

        if ( mode != 2 )
{
    for ( j=0; j<n; j++ )
        diag[j] = MAX(diag[j],wa2[j]);
}

// O** the inner loop.

        do {
#if BUG
            printf( "lmdif/ inner loop iter=%d nfev=%d\n", iter, *nfev );
#endif

// OI* determine the levenberg-marquardt parameter.

            lm_lmpar( n,fjac,m,ipvt,diag,qtf,delta,&par,wa1,wa2,wa3,wa4 );

// OI* store the direction p and x + p. calculate the norm of p.

            for ( j=0; j<n; j++ )
            {
                wa1[j] = -wa1[j];
                wa2[j] = x[j] + wa1[j];
                wa3[j] = diag[j]*wa1[j];
            }
            pnorm = lm_enorm(n,wa3);

// OI* on the first iteration, adjust the initial step bound.

            if ( *nfev<= 1+n ) // bug corrected by J. Wuttke in 2004
                delta = MIN(delta,pnorm);

// OI* evaluate the function at x + p and calculate its norm.

            *info = 0;
            (*evaluate)( wa2, m, wa4, data, info );
            (*printout)( n, x, m, wa4, data, 2, iter, ++(*nfev) );
            if ( *info < 0 ) return;  // user requested break

            fnorm1 = lm_enorm(m,wa4);
#if BUG
            printf( "lmdif/ pnorm %.10e  fnorm1 %.10e  fnorm %.10e"
                    " delta=%.10e par=%.10e\n",
                    pnorm, fnorm1, fnorm, delta, par );
#endif

// OI* compute the scaled actual reduction.

            if ( p1*fnorm1 < fnorm )
                actred = 1 - SQR( fnorm1/fnorm );
            else
                actred = -1;

// OI* compute the scaled predicted reduction and
//     the scaled directional derivative.

            for ( j=0; j<n; j++ )
            {
                wa3[j] = 0;
                for ( i=0; i<=j; i++ )
                    wa3[i] += fjac[j*m+i]*wa1[ ipvt[j] ];
            }
            temp1 = lm_enorm(n,wa3) / fnorm;
            temp2 = sqrt(par) * pnorm / fnorm;
            prered = SQR(temp1) + 2 * SQR(temp2);
            dirder = - ( SQR(temp1) + SQR(temp2) );

// OI* compute the ratio of the actual to the predicted reduction.

            ratio = prered!=0 ? actred/prered : 0;
#if BUG
            printf( "lmdif/ actred=%.10e prered=%.10e ratio=%.10e"
                    " sq(1)=%.10e sq(2)=%.10e dd=%.10e\n",
                    actred, prered, prered!=0 ? ratio : 0.,
                    SQR(temp1), SQR(temp2), dirder );
#endif

// OI* update the step bound.

            if (ratio <= p25)
{
    if (actred >= 0.)
        temp = p5;
    else
        temp = p5*dirder/(dirder + p5*actred);
    if ( p1*fnorm1 >= fnorm || temp < p1 )
        temp = p1;
    delta = temp * MIN(delta,pnorm/p1);
    par /= temp;
}
            else if ( par == 0. || ratio >= p75 )
{
    delta = pnorm/p5;
    par *= p5;
}

// OI* test for successful iteration...

            if (ratio >= p0001)
{

//     ... successful iteration. update x, fvec, and their norms.

    for ( j=0; j<n; j++ )
    {
        x[j] = wa2[j];
        wa2[j] = diag[j]*x[j];
    }
    for ( i=0; i<m; i++ )
        fvec[i] = wa4[i];
    xnorm = lm_enorm(n,wa2);
    fnorm = fnorm1;
    iter++;
}
#if BUG
            else {
    printf( "ATTN: iteration considered unsuccessful\n" );
            }
#endif

// OI* tests for convergence ( otherwise *info = 1, 2, or 3 )

            *info = 0; // do not terminate (unless overwritten by nonzero value)
            if ( fabs(actred) <= ftol && prered <= ftol && p5*ratio <= 1 )
                *info = 1;
            if (delta <= xtol*xnorm)
                *info += 2;
            if ( *info != 0)
                return;

// OI* tests for termination and stringent tolerances.

            if ( *nfev >= maxfev)
                *info = 5;
            if ( fabs(actred) <= LM_MACHEP &&
                 prered <= LM_MACHEP && p5*ratio <= 1 )
                *info = 6;
            if (delta <= LM_MACHEP*xnorm)
                *info = 7;
            if (gnorm <= LM_MACHEP)
                *info = 8;
            if ( *info != 0)
                return;

// OI* end of the inner loop. repeat if iteration unsuccessful.

        } while (ratio < p0001);

// O** end of the outer loop.

    } while (1);

}



void lm_lmpar(int n, double* r, int ldr, int* ipvt, double* diag, double* qtb,
              double delta, double* par, double* x, double* sdiag,
              double* wa1, double* wa2)
{
/*     given an m by n matrix a, an n by n nonsingular diagonal
    *     matrix d, an m-vector b, and a positive number delta,
    *     the problem is to determine a value for the parameter
    *     par such that if x solves the system
    *
    *        a*x = b ,       sqrt(par)*d*x = 0 ,
    *
    *     in the least squares sense, and dxnorm is the euclidean
    *     norm of d*x, then either par is 0. and
    *
    *        (dxnorm-delta) .le. 0.1*delta ,
    *
    *     or par is positive and
    *
    *        abs(dxnorm-delta) .le. 0.1*delta .
    *
    *     this subroutine completes the solution of the problem
    *     if it is provided with the necessary information from the
    *     qr factorization, with column pivoting, of a. that is, if
    *     a*p = q*r, where p is a permutation matrix, q has orthogonal
    *     columns, and r is an upper triangular matrix with diagonal
    *     elements of nonincreasing magnitude, then lmpar expects
    *     the full upper triangle of r, the permutation matrix p,
    *     and the first n components of (q transpose)*b. on output
    *     lmpar also provides an upper triangular matrix s such that
    *
    *         t       t               t
    *        p *(a *a + par*d*d)*p = s *s .
    *
    *     s is employed within lmpar and may be of separate interest.
    *
    *     only a few iterations are generally needed for convergence
    *     of the algorithm. if, however, the limit of 10 iterations
    *     is reached, then the output par will contain the best
    *     value obtained so far.
    *
 *     parameters:
    *
    *    n is a positive integer input variable set to the order of r.
    *
    *    r is an n by n array. on input the full upper triangle
    *      must contain the full upper triangle of the matrix r.
    *      on output the full upper triangle is unaltered, and the
    *      strict lower triangle contains the strict upper triangle
    *      (transposed) of the upper triangular matrix s.
    *
    *    ldr is a positive integer input variable not less than n
    *      which specifies the leading dimension of the array r.
    *
    *    ipvt is an integer input array of length n which defines the
    *      permutation matrix p such that a*p = q*r. column j of p
    *      is column ipvt(j) of the identity matrix.
    *
    *    diag is an input array of length n which must contain the
    *      diagonal elements of the matrix d.
    *
    *    qtb is an input array of length n which must contain the first
    *      n elements of the vector (q transpose)*b.
    *
    *    delta is a positive input variable which specifies an upper
    *      bound on the euclidean norm of d*x.
    *
    *    par is a nonnegative variable. on input par contains an
    *      initial estimate of the levenberg-marquardt parameter.
    *      on output par contains the final estimate.
    *
    *    x is an output array of length n which contains the least
    *      squares solution of the system a*x = b, sqrt(par)*d*x = 0,
    *      for the output par.
    *
    *    sdiag is an output array of length n which contains the
    *      diagonal elements of the upper triangular matrix s.
    *
    *    wa1 and wa2 are work arrays of length n.
    *
 */
    int i, iter, j, nsing;
    double dxnorm, fp, fp_old, gnorm, parc, parl, paru;
    double sum, temp;
    static double p1 = 0.1;
    static double p001 = 0.001;

#if BUG
    printf( "lmpar\n" );
#endif

// *** compute and store in x the gauss-newton direction. if the
//     jacobian is rank-deficient, obtain a least squares solution.

    nsing = n;
    for ( j=0; j<n; j++ )
    {
        wa1[j] = qtb[j];
        if ( r[j*ldr+j] == 0 && nsing == n )
            nsing = j;
        if (nsing < n)
            wa1[j] = 0;
    }
#if BUG
    printf( "nsing %d ", nsing );
#endif
    for ( j=nsing-1; j>=0; j-- )
{
    wa1[j] = wa1[j]/r[j+ldr*j];
    temp = wa1[j];
    for ( i=0; i<j; i++ )
        wa1[i] -= r[j*ldr+i]*temp;
}

    for ( j=0; j<n; j++ )
        x[ ipvt[j] ] = wa1[j];

// *** initialize the iteration counter.
//     evaluate the function at the origin, and test
//     for acceptance of the gauss-newton direction.

    iter = 0;
    for ( j=0; j<n; j++ )
        wa2[j] = diag[j]*x[j];
    dxnorm = lm_enorm(n,wa2);
    fp = dxnorm - delta;
    if (fp <= p1*delta)
{
#if BUG
      printf( "lmpar/ terminate (fp<delta/10\n" );
#endif
        *par = 0;
        return;
}

// *** if the jacobian is not rank deficient, the newton
//     step provides a lower bound, parl, for the 0. of
//     the function. otherwise set this bound to 0..

    parl = 0;
    if (nsing >= n)
{
    for ( j=0; j<n; j++ )
        wa1[j] = diag[ ipvt[j] ] * wa2[ ipvt[j] ] / dxnorm;

    for ( j=0; j<n; j++ )
    {
        sum = 0.;
        for ( i=0; i<j; i++ )
            sum += r[j*ldr+i]*wa1[i];
        wa1[j] = (wa1[j] - sum)/r[j+ldr*j];
    }
    temp = lm_enorm(n,wa1);
    parl = fp/delta/temp/temp;
}

// *** calculate an upper bound, paru, for the 0. of the function.

    for ( j=0; j<n; j++ )
{
    sum = 0;
    for ( i=0; i<=j; i++ )
        sum += r[j*ldr+i]*qtb[i];
    wa1[j] = sum/diag[ ipvt[j] ];
}
    gnorm = lm_enorm(n,wa1);
    paru = gnorm/delta;
    if (paru == 0.)
        paru = LM_DWARF/MIN(delta,p1);

// *** if the input par lies outside of the interval (parl,paru),
//     set par to the closer endpoint.

    *par = MAX( *par,parl);
    *par = MIN( *par,paru);
    if ( *par == 0.)
        *par = gnorm/dxnorm;
#if BUG
    printf( "lmpar/ parl %.4e  par %.4e  paru %.4e\n", parl, *par, paru );
#endif

// *** iterate.

    for ( ; ; iter++ ) {

// *** evaluate the function at the current value of par.

    if ( *par == 0.)
        *par = MAX(LM_DWARF,p001*paru);
    temp = sqrt( *par );
    for ( j=0; j<n; j++ )
        wa1[j] = temp*diag[j];
    lm_qrsolv( n, r, ldr, ipvt, wa1, qtb, x, sdiag, wa2);
    for ( j=0; j<n; j++ )
        wa2[j] = diag[j]*x[j];
    dxnorm = lm_enorm(n,wa2);
    fp_old = fp;
    fp = dxnorm - delta;

// ***       if the function is small enough, accept the current value
//     of par. also test for the exceptional cases where parl
//     is 0. or the number of iterations has reached 10.

    if ( fabs(fp) <= p1*delta
         || (parl == 0. && fp <= fp_old && fp_old < 0.)
         || iter == 10 )
        break; // the only exit from this loop

// *** compute the Newton correction.

    for ( j=0; j<n; j++ )
        wa1[j] = diag[ ipvt[j] ] * wa2[ ipvt[j] ] / dxnorm;

    for ( j=0; j<n; j++ )
    {
        wa1[j] = wa1[j]/sdiag[j];
        for ( i=j+1; i<n; i++ )
            wa1[i] -= r[j*ldr+i]*wa1[j];
    }
    temp = lm_enorm( n, wa1);
    parc = fp/delta/temp/temp;

// *** depending on the sign of the function, update parl or paru.

    if (fp > 0)
        parl = MAX(parl, *par);
    else if (fp < 0)
        paru = MIN(paru, *par);
        // the case fp==0 is precluded by the break condition

// *** compute an improved estimate for par.

    *par = MAX(parl, *par + parc);

    }

}



void lm_qrfac(int m, int n, double* a, int pivot, int* ipvt,
              double* rdiag, double* acnorm, double* wa)
{
/*
    *     this subroutine uses householder transformations with column
    *     pivoting (optional) to compute a qr factorization of the
    *     m by n matrix a. that is, qrfac determines an orthogonal
    *     matrix q, a permutation matrix p, and an upper trapezoidal
    *     matrix r with diagonal elements of nonincreasing magnitude,
    *     such that a*p = q*r. the householder transformation for
    *     column k, k = 1,2,...,min(m,n), is of the form
    *
    *                    t
    *        i - (1/u(k))*u*u
    *
    *     where u has 0.s in the first k-1 positions. the form of
    *     this transformation and the method of pivoting first
    *     appeared in the corresponding linpack subroutine.
    *
 *     parameters:
    *
    *    m is a positive integer input variable set to the number
    *      of rows of a.
    *
    *    n is a positive integer input variable set to the number
    *      of columns of a.
    *
    *    a is an m by n array. on input a contains the matrix for
    *      which the qr factorization is to be computed. on output
    *      the strict upper trapezoidal part of a contains the strict
    *      upper trapezoidal part of r, and the lower trapezoidal
    *      part of a contains a factored form of q (the non-trivial
    *      elements of the u vectors described above).
    *
    *    pivot is a logical input variable. if pivot is set true,
    *      then column pivoting is enforced. if pivot is set false,
    *      then no column pivoting is done.
    *
    *    ipvt is an integer output array of length lipvt. ipvt
    *      defines the permutation matrix p such that a*p = q*r.
    *      column j of p is column ipvt(j) of the identity matrix.
    *      if pivot is false, ipvt is not referenced.
    *
    *    rdiag is an output array of length n which contains the
    *      diagonal elements of r.
    *
    *    acnorm is an output array of length n which contains the
    *      norms of the corresponding columns of the input matrix a.
    *      if this information is not needed, then acnorm can coincide
    *      with rdiag.
    *
    *    wa is a work array of length n. if pivot is false, then wa
    *      can coincide with rdiag.
    *
 */
    int i, j, k, kmax, minmn;
    double ajnorm, sum, temp;
    static double p05 = 0.05;

// *** compute the initial column norms and initialize several arrays.

    for ( j=0; j<n; j++ )
    {
        acnorm[j] = lm_enorm(m, &a[j*m]);
        rdiag[j] = acnorm[j];
        wa[j] = rdiag[j];
        if ( pivot )
            ipvt[j] = j;
    }
#if BUG
    printf( "qrfac\n" );
#endif

// *** reduce a to r with householder transformations.

    minmn = MIN(m,n);
    for ( j=0; j<minmn; j++ )
    {
        if ( !pivot ) goto pivot_ok;

// *** bring the column of largest norm into the pivot position.

        kmax = j;
        for ( k=j+1; k<n; k++ )
            if (rdiag[k] > rdiag[kmax])
                kmax = k;
        if (kmax == j) goto pivot_ok; // bug fixed in rel 2.1

        for ( i=0; i<m; i++ )
        {
            temp        = a[j*m+i];
            a[j*m+i]    = a[kmax*m+i];
            a[kmax*m+i] = temp;
        }
        rdiag[kmax] = rdiag[j];
        wa[kmax] = wa[j];
        k = ipvt[j];
        ipvt[j] = ipvt[kmax];
        ipvt[kmax] = k;

    pivot_ok:

// *** compute the Householder transformation to reduce the
//     j-th column of a to a multiple of the j-th unit vector.

            ajnorm = lm_enorm( m-j, &a[j*m+j] );
    if (ajnorm == 0.)
    {
        rdiag[j] = 0;
        continue;
    }

    if (a[j*m+j] < 0.)
        ajnorm = -ajnorm;
    for ( i=j; i<m; i++ )
        a[j*m+i] /= ajnorm;
    a[j*m+j] += 1;

// *** apply the transformation to the remaining columns
//     and update the norms.

    for ( k=j+1; k<n; k++ )
    {
        sum = 0;

        for ( i=j; i<m; i++ )
            sum += a[j*m+i]*a[k*m+i];

        temp = sum/a[j+m*j];

        for ( i=j; i<m; i++ )
            a[k*m+i] -= temp * a[j*m+i];

        if ( pivot && rdiag[k] != 0. )
        {
            temp = a[m*k+j]/rdiag[k];
            temp = MAX( 0., 1-temp*temp );
            rdiag[k] *= sqrt(temp);
            temp = rdiag[k]/wa[k];
            if ( p05*SQR(temp) <= LM_MACHEP )
            {
                rdiag[k] = lm_enorm( m-j-1, &a[m*k+j+1]);
                wa[k] = rdiag[k];
            }
        }
    }

    rdiag[j] = -ajnorm;
    }
}



void lm_qrsolv(int n, double* r, int ldr, int* ipvt, double* diag,
               double* qtb, double* x, double* sdiag, double* wa)
{
/*
    *     given an m by n matrix a, an n by n diagonal matrix d,
    *     and an m-vector b, the problem is to determine an x which
    *     solves the system
    *
    *        a*x = b ,       d*x = 0 ,
    *
    *     in the least squares sense.
    *
    *     this subroutine completes the solution of the problem
    *     if it is provided with the necessary information from the
    *     qr factorization, with column pivoting, of a. that is, if
    *     a*p = q*r, where p is a permutation matrix, q has orthogonal
    *     columns, and r is an upper triangular matrix with diagonal
    *     elements of nonincreasing magnitude, then qrsolv expects
    *     the full upper triangle of r, the permutation matrix p,
    *     and the first n components of (q transpose)*b. the system
    *     a*x = b, d*x = 0, is then equivalent to
    *
    *             t     t
    *        r*z = q *b ,  p *d*p*z = 0 ,
    *
    *     where x = p*z. if this system does not have full rank,
    *     then a least squares solution is obtained. on output qrsolv
    *     also provides an upper triangular matrix s such that
    *
    *         t       t           t
    *        p *(a *a + d*d)*p = s *s .
    *
    *     s is computed within qrsolv and may be of separate interest.
    *
    *     parameters
    *
    *    n is a positive integer input variable set to the order of r.
    *
    *    r is an n by n array. on input the full upper triangle
    *      must contain the full upper triangle of the matrix r.
    *      on output the full upper triangle is unaltered, and the
    *      strict lower triangle contains the strict upper triangle
    *      (transposed) of the upper triangular matrix s.
    *
    *    ldr is a positive integer input variable not less than n
    *      which specifies the leading dimension of the array r.
    *
    *    ipvt is an integer input array of length n which defines the
    *      permutation matrix p such that a*p = q*r. column j of p
    *      is column ipvt(j) of the identity matrix.
    *
    *    diag is an input array of length n which must contain the
    *      diagonal elements of the matrix d.
    *
    *    qtb is an input array of length n which must contain the first
    *      n elements of the vector (q transpose)*b.
    *
    *    x is an output array of length n which contains the least
    *      squares solution of the system a*x = b, d*x = 0.
    *
    *    sdiag is an output array of length n which contains the
    *      diagonal elements of the upper triangular matrix s.
    *
    *    wa is a work array of length n.
    *
 */
    int i, kk, j, k, nsing;
    double qtbpj, sum, temp;
    double sin, cos, tan, cotan; // these are local variables, not functions
    static double p25 = 0.25;
    static double p5 = 0.5;

// *** copy r and (q transpose)*b to preserve input and initialize s.
//     in particular, save the diagonal elements of r in x.

    for ( j=0; j<n; j++ )
    {
        for ( i=j; i<n; i++ )
            r[j*ldr+i] = r[i*ldr+j];
        x[j] = r[j*ldr+j];
        wa[j] = qtb[j];
    }
#if BUG
    printf( "qrsolv\n" );
#endif

// *** eliminate the diagonal matrix d using a givens rotation.

    for ( j=0; j<n; j++ )
{

// ***       prepare the row of d to be eliminated, locating the
//     diagonal element using p from the qr factorization.

    if (diag[ ipvt[j] ] == 0.)
        goto L90;
    for ( k=j; k<n; k++ )
        sdiag[k] = 0.;
    sdiag[j] = diag[ ipvt[j] ];

// ***       the transformations to eliminate the row of d
//     modify only a single element of (q transpose)*b
//     beyond the first n, which is initially 0..

    qtbpj = 0.;
    for ( k=j; k<n; k++ )
    {

//        determine a givens rotation which eliminates the
//        appropriate element in the current row of d.

        if (sdiag[k] == 0.)
            continue;
        kk = k + ldr * k; // <! keep this shorthand !>
        if ( fabs(r[kk]) < fabs(sdiag[k]) )
        {
            cotan = r[kk]/sdiag[k];
            sin = p5/sqrt(p25+p25*SQR(cotan));
            cos = sin*cotan;
        }
        else
        {
            tan = sdiag[k]/r[kk];
            cos = p5/sqrt(p25+p25*SQR(tan));
            sin = cos*tan;
        }

// ***          compute the modified diagonal element of r and
//        the modified element of ((q transpose)*b,0).

        r[kk] = cos*r[kk] + sin*sdiag[k];
        temp = cos*wa[k] + sin*qtbpj;
        qtbpj = -sin*wa[k] + cos*qtbpj;
        wa[k] = temp;

// *** accumulate the tranformation in the row of s.

        for ( i=k+1; i<n; i++ )
        {
            temp = cos*r[k*ldr+i] + sin*sdiag[i];
            sdiag[i] = -sin*r[k*ldr+i] + cos*sdiag[i];
            r[k*ldr+i] = temp;
        }
    }
    L90:

// *** store the diagonal element of s and restore
//     the corresponding diagonal element of r.

            sdiag[j] = r[j*ldr+j];
    r[j*ldr+j] = x[j];
}

// *** solve the triangular system for z. if the system is
//     singular, then obtain a least squares solution.

    nsing = n;
    for ( j=0; j<n; j++ )
{
    if ( sdiag[j] == 0. && nsing == n )
        nsing = j;
    if (nsing < n)
        wa[j] = 0;
}

    for ( j=nsing-1; j>=0; j-- )
{
    sum = 0;
    for ( i=j+1; i<nsing; i++ )
        sum += r[j*ldr+i]*wa[i];
    wa[j] = (wa[j] - sum)/sdiag[j];
}

// *** permute the components of z back to components of x.

    for ( j=0; j<n; j++ )
        x[ ipvt[j] ] = wa[j];
}



double lm_enorm( int n, double *x )
{
/*     given an n-vector x, this function calculates the
    *     euclidean norm of x.
    *
    *     the euclidean norm is computed by accumulating the sum of
    *     squares in three different sums. the sums of squares for the
    *     small and large components are scaled so that no overflows
    *     occur. non-destructive underflows are permitted. underflows
    *     and overflows do not occur in the computation of the unscaled
    *     sum of squares for the intermediate components.
    *     the definitions of small, intermediate and large components
    *     depend on two constants, LM_SQRT_DWARF and LM_SQRT_GIANT. the main
    *     restrictions on these constants are that LM_SQRT_DWARF**2 not
    *     underflow and LM_SQRT_GIANT**2 not overflow.
    *
    *     parameters
    *
    *    n is a positive integer input variable.
    *
    *    x is an input array of length n.
 */
    int i;
    double agiant, s1, s2, s3, xabs, x1max, x3max, temp;

    s1 = 0;
    s2 = 0;
    s3 = 0;
    x1max = 0;
    x3max = 0;
    agiant = LM_SQRT_GIANT/( (double) n);

    for ( i=0; i<n; i++ )
    {
        xabs = fabs(x[i]);
        if ( xabs > LM_SQRT_DWARF && xabs < agiant )
        {
// **  sum for intermediate components.
            s2 += xabs*xabs;
            continue;
        }

        if ( xabs >  LM_SQRT_DWARF )
        {
// **  sum for large components.
            if (xabs > x1max)
            {
                temp = x1max/xabs;
                s1 = 1 + s1*SQR(temp);
                x1max = xabs;
            }
            else
            {
                temp = xabs/x1max;
                s1 += SQR(temp);
            }
            continue;
        }
// **  sum for small components.
        if (xabs > x3max)
        {
            temp = x3max/xabs;
            s3 = 1 + s3*SQR(temp);
            x3max = xabs;
        }
        else
        {
            if (xabs != 0.)
            {
                temp = xabs/x3max;
                s3 += SQR(temp);
            }
        }
    }

// *** calculation of norm.

    if (s1 != 0)
        return x1max*sqrt(s1 + (s2/x1max)/x1max);
    if (s2 != 0)
    {
        if (s2 >= x3max)
            return sqrt( s2*(1+(x3max/s2)*(x3max*s3)) );
        else
            return sqrt( x3max*((s2/x3max)+(x3max*s3)) );
    }

    return x3max*sqrt(s3);
}


//  Image decompression support

static int read_scanline_bytes( z_stream *stream, unsigned char *dest, size_t len)
{
    if( stream == NULL || dest == NULL) return 1;

    int ret;
    uint32_t bytes_read;

    stream->avail_out = len;
    stream->next_out = dest;

    if(!stream->avail_in)
        return 1;

    do
    {
        ret = inflate(stream, Z_SYNC_FLUSH);

        if(ret != Z_OK){
            if( ret == Z_STREAM_END)
                return 0;
            else
                return 1;
        }

    }while(stream->avail_out != 0);

    return 0;
}

int ocpn_decode_image( unsigned char *in, unsigned char *out, size_t in_size, size_t out_size, int Size_X, int Size_Y, int nColors)
{

    int ret;
    size_t out_size_required, out_width;

    out_size_required = out_size;
    
    out_width = out_size_required / Size_Y;

    //uint8_t channels = 1; 

    uint8_t bytes_per_pixel = 1;

    int bit_depth = 8;
    if(nColors <= 16)
        bit_depth = 4;

    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    int ec = inflateInit(&stream);
    if(ec != Z_OK){
        return ec;
    }

    int apply_trns = 0;
    int apply_gamma = 0;
    int use_sbit = 0;
    int indexed = 1;
    int do_scaling = 0;

    int interlaced = 0;

    uint32_t i, k, scanline_idx, width;
   
    const uint8_t samples_per_byte = 8 / bit_depth;
    const uint8_t mask = (uint16_t)(1 << bit_depth) - 1;
    const uint8_t initial_shift = 8 - bit_depth;
    size_t pixel_size = 1;
    size_t pixel_offset = 0;
    
    unsigned char *pixel;
    unsigned processing_depth = bit_depth;

    size_t scanline_width = 0;

    /* Calculate scanline width in bits, round up to the nearest byte */
    scanline_width = bit_depth;
    scanline_width = scanline_width * Size_X;
    scanline_width += 8; /* Filter byte */
        /* Round up */
    if(scanline_width % 8 != 0){
        scanline_width = scanline_width + 8;
        scanline_width -= (scanline_width % 8);
    }
    scanline_width /= 8;

    unsigned char *row = out;
    unsigned char *scanline = (unsigned char *)malloc(scanline_width);

    stream.avail_in = in_size;
    stream.next_in = in; 

    {

        /* Read the first filter byte, offsetting all reads by 1 byte.
           The scanlines will be aligned with the start of the array with
           the next scanline's filter byte at the end,
           the last scanline will end up being 1 byte "shorter". */
        unsigned char filter;
        ret = read_scanline_bytes( &stream, &filter, 1);
        if(ret)
            goto decode_err;

        for(scanline_idx=0; scanline_idx < (unsigned int)Size_Y; scanline_idx++)
        {
            /* The last scanline is 1 byte "shorter" */
            if(scanline_idx == (unsigned int)(Size_Y - 1))
                ret = read_scanline_bytes( &stream, scanline, scanline_width - 1);
            else
                ret = read_scanline_bytes( &stream, scanline, scanline_width);

            if(ret)
                goto decode_err;

            pixel_offset = 0;
            width = Size_X;

            uint8_t shift_amount = initial_shift;

            if(bit_depth == 8){
                memcpy( row, scanline, width);
            }
            else{
                for(k=0; k < width; k++){
                    pixel = row + pixel_offset;
                    pixel_offset += pixel_size;

                    uint8_t entry = 0;
                    memcpy(&entry, scanline + k / samples_per_byte, 1);

                    if(shift_amount > 8)
                        shift_amount = initial_shift;

                    entry = (entry >> shift_amount) & mask;
                    shift_amount -= bit_depth;

                    *pixel = entry;
                }
            }

            row += out_width;

        }/* for(scanline_idx */
    }


decode_err:

    inflateEnd(&stream);
    free( scanline );

    return ret;
}

