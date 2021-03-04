/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  NVC Plugin Chart
 * Author:   David Register
 *
 *******************************************************************************/


#ifndef _XTR1CH_H_
#define _XTR1CH_H_

#include "wx/wxprec.h"
#include <wx/wfstream.h>

#include "ocpn_plugin.h"

#include "o-charts_pi.h"
#include "eSENCChart.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
#endif //precompiled headers

#define DATUM_INDEX_WGS84     100
#define DATUM_INDEX_UNKNOWN   -1

#ifndef PI
      #define PI        3.1415926535897931160E0      /* pi */
#endif
#define DEGREE    (PI/180.0)
#define RADIAN    (180.0/PI)

static const double WGS84_semimajor_axis_meters       = 6378137.0;           // WGS84 semimajor axis
static const double mercator_k0                       = 0.9996;
static const double WGSinvf                           = 298.257223563;       /* WGS84 1/f */

#define __PIX_CACHE_WXIMAGE__
#define BPP 24

class wxDynamicLibrary;
class oernc_inStream;


void toDMS(double a, char *bufp, int bufplen);
void toDMM(double a, char *bufp, int bufplen);
void todmm(int flag, double a, char *bufp, int bufplen);

void toTM(float lat, float lon, float lat0, float lon0, double *x, double *y);
void fromTM(double x, double y, double lat0, double lon0, double *lat, double *lon);

void toSM(double lat, double lon, double lat0, double lon0, double *x, double *y);
void fromSM(double x, double y, double lat0, double lon0, double *lat, double *lon);

void toSM_ECC(double lat, double lon, double lat0, double lon0, double *x, double *y);
void fromSM_ECC(double x, double y, double lat0, double lon0, double *lat, double *lon);

void ll_gc_ll(double lat, double lon, double crs, double dist, double *dlat, double *dlon);
void ll_gc_ll_reverse(double lat1, double lon1, double lat2, double lon2,
                                double *bearing, double *dist);

double DistGreatCircle(double slat, double slon, double dlat, double dlon);

extern "C" int GetDatumIndex(const char *str);
extern "C" void MolodenskyTransform (double lat, double lon, double *to_lat, double *to_lon, int from_datum_index, int to_datum_index);

void DistanceBearingMercator(double lat0, double lon0, double lat1, double lon1, double *brg, double *dist);

int Georef_Calculate_Coefficients(struct GeoRef *cp, int nlin_lon);
int Georef_Calculate_Coefficients_Proj(struct GeoRef *cp);

int ocpn_decode_image(unsigned char *in, unsigned char *out, size_t in_size, size_t out_size, int Size_X, int Size_Y, int nColors);


typedef enum PaletteDir
{
      PaletteFwd,
      PaletteRev
}_PaletteDir;


typedef enum BSB_Color_Capability
{
    COLOR_RGB_DEFAULT = 0,                   // Default corresponds to bsb entries "RGB"
    DAY,
    DUSK,
    NIGHT,
    NIGHTRED,
    GRAY,
    PRC,
    PRG,
    N_BSB_COLORS
}_BSB_Color_Capability;

//-----------------------------------------------------------------------------
//    Helper classes
//-----------------------------------------------------------------------------

class Refpoint
{
public:
      int         bXValid;
      int         bYValid;
      float       xr;
      float       yr;
      float       latr;
      float       lonr;
      float       xpl_error;
      float       xlp_error;
      float       ypl_error;
      float       ylp_error;

};


class Plypoint
{
      public:
            float ltp;
            float lnp;
};

struct TileOffsetCache
{
    int offset; // offset from start of line pointer
    int pixel; // offset from current pixel
};

class CachedLine
{
public:
      //int               xstart;
      //int               xlength;
      //unsigned char     *pPix;
      //bool              bValid;
      
      unsigned char    *pPix;
      TileOffsetCache  *pTileOffset; // entries for random access
      
      bool              bValid;
      
};

class opncpnPalette
{
    public:
        opncpnPalette();
        ~opncpnPalette();

        int *FwdPalette;
        int *RevPalette;
        int nFwd;
        int nRev;
};

struct DATUM {
        char *name;
        short ellipsoid;
        double dx;
        double dy;
        double dz;
};

struct ELLIPSOID {
        char *name;             // name of ellipsoid
        double a;               // semi-major axis, meters
        double invf;            // 1/f
};

struct GeoRef {
  int status;
  int count;
  int order;
  double *tx;
  double *ty;
  double *lon;
  double *lat;
  double *pwx;
  double *pwy;
  double *wpx;
  double *wpy;
  int    txmax;
  int    tymax;
  int    txmin;
  int    tymin;
  double lonmax;
  double lonmin;
  double latmax;
  double latmin;

};

#define      RENDER_LODEF  0
#define      RENDER_HIDEF  1


// typedef enum InitReturn
// {
//       INIT_OK = 0,
//       INIT_FAIL_RETRY,        // Init failed, retry suggested
//       INIT_FAIL_REMOVE,       // Init failed, suggest remove from further use
//       INIT_FAIL_NOERROR       // Init failed, request no explicit error message
// }_InitReturn;

typedef enum ChartInitFlag
{
      FULL_INIT = 0,
      HEADER_ONLY,
      THUMB_ONLY
}_ChartInitFlag;

class ThumbData
{
public:
    ThumbData();
    virtual ~ThumbData();

      wxBitmap    *pDIBThumb;
      int         ShipX;
      int         ShipY;
      int         Thumb_Size_X;
      int         Thumb_Size_Y;
};



typedef enum RGBO
{
    RGB = 0,
    BGR
}_RGBO;


// ============================================================================
// PIPixelCache Definition
// ============================================================================
class PIPixelCache
{
    public:

      //    Constructors

        PIPixelCache(int width, int height, int depth);
        ~PIPixelCache();

        void SelectIntoDC(wxMemoryDC &dc);
        void Update(void);
        void BuildBM(void);

        RGBO GetRGBO(){return m_rgbo;}
        unsigned char *GetpData() const;
        int GetLinePitch() const { return line_pitch_bytes; }
        int GetWidth(void){ return m_width; }
        int GetHeight(void){ return m_height; }
        wxBitmap &GetBitmap(void){ return *m_pbm; }

      //    Data storage
    private:
        int               m_width;
        int               m_height;
        int               m_depth;
        int               line_pitch_bytes;
        int               bytes_per_pixel;
        RGBO               m_rgbo;
        unsigned char     *pData;

        wxBitmap          *m_pbm;
        wxImage           *m_pimage;

};




// ----------------------------------------------------------------------------
// Chart_oeRNC Definition
// ----------------------------------------------------------------------------

class  Chart_oeuRNC : public PlugInChartBase
{
      DECLARE_DYNAMIC_CLASS(Chart_oeuRNC)

    public:
      //    Public methods

      Chart_oeuRNC();
      virtual ~Chart_oeuRNC();

      void ChartBaseBSBCTOR(void);
      void ChartBaseBSBDTOR(void);
      wxString GetFileSearchMask(void);

      //    Accessors

      virtual wxBitmap *GetThumbnail(int tnx, int tny, int cs);

      double GetNormalScaleMin(double canvas_scale_factor, bool b_allow_overzoom);
      double GetNormalScaleMax(double canvas_scale_factor, int canvas_width);


      virtual int Init( const wxString& name, int init_flags );

      virtual int latlong_to_pix_vp(double lat, double lon, int &pixx, int &pixy, PlugIn_ViewPort& vp);
      virtual int vp_pix_to_latlong(PlugIn_ViewPort& vp, int pixx, int pixy, double *lat, double *lon);
      virtual void latlong_to_chartpix(double lat, double lon, double &pixx, double &pixy);
      virtual void chartpix_to_latlong(double pixx, double pixy, double *plat, double *plon);

      wxBitmap &RenderRegionView(const PlugIn_ViewPort& VPoint, const wxRegion &Region);


      virtual bool AdjustVP(PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed);
      virtual double GetNearestPreferredScalePPM(double target_scale_ppm);

      virtual bool IsRenderDelta(PlugIn_ViewPort &vp_last, PlugIn_ViewPort &vp_proposed);

      void GetValidCanvasRegion(const PlugIn_ViewPort& VPoint, wxRegion  *pValidRegion);

      virtual bool GetChartExtent(ExtentPI *pext);

      void SetColorScheme(int cs, bool bApplyImmediate);

      void ComputeSourceRectangle(const PlugIn_ViewPort &vp, wxRect *pSourceRect);
      wxRect GetSourceRect(){ return Rsrc; }

      wxImage *GetImage();

      double GetRasterScaleFactor() { return m_piraster_scale_factor; }

      bool CreateChartInfoFile( wxString chartName );
  

protected:
//    Methods

      void SetVPRasterParms(const PlugIn_ViewPort &vpt);

      bool IsCacheValid(){ return cached_image_ok; }
      void InvalidateCache(){cached_image_ok = 0;}

      void FillLineCache( void );

      void CreatePaletteEntry(char *buffer, int palette_index);
      PaletteDir GetPaletteDir(void);
      int  *GetPalettePtr(BSB_Color_Capability);

      double GetClosestValidNaturalScalePPM(double target_scale, double scale_factor_min, double scale_factor_max);

      double GetPPM(){ return m_pi_ppm_avg;}
      int GetSize_X(){ return Size_X;}
      int GetSize_Y(){ return Size_Y;}

      void InvalidateLineCache();

      bool GetChartBits( wxRect& source, unsigned char *pPix, int sub_samp );
      bool GetChartBits_Internal( wxRect& source, unsigned char *pPix, int sub_samp );
      int BSBGetScanline( unsigned char *pLineBuf, int y, int xs, int xl, int sub_samp);

      bool GetAndScaleData(unsigned char *ppn,
                                   wxRect& source, int source_stride, wxRect& dest, int dest_stride,
                                   double scale_factor, int scale_type);


      bool GetViewUsingCache( wxRect& source, wxRect& dest, const wxRegion& Region, int scale_type );
      bool GetView( wxRect& source, wxRect& dest, int scale_type );
      bool IsRenderCacheable( wxRect& source, wxRect& dest );


      //int BSBScanScanline(oernc_inStream *pinStream);
      int ReadBSBHdrLine( oernc_inStream*, char *, int );
      int AnalyzeRefpoints(void);
      bool SetMinMax(void);

      InitReturn PreInit( const wxString& name, int init_flags, int cs );
      InitReturn PostInit(void);
      wxString getKeyAsciiHex(const wxString &name);
      
      void FreeLineCacheRows(int start=0, int end=-1);

      int DecodeImage();

//    Protected Data
      PIPixelCache        *pPixCache;

      int         Size_X;                 // Chart native pixel dimensions
      int         Size_Y;
      int         m_Chart_DU;
      double      m_cph;

      double      m_proj_parameter;                     // Mercator:               Projection Latitude
                                                      // Transverse Mercator:    Central Meridian
      double      m_dx;                                 // Pixel scale factors, from KAP header
      double      m_dy;

      int         m_datum_index;
      double      m_dtm_lat;
      double      m_dtm_lon;


      wxRect      cache_rect;
      wxRect      cache_rect_scaled;
      bool        cached_image_ok;
      int         cache_scale_method;
      double      m_cached_scale_ppm;
      wxRect      m_last_vprect;


      wxRect      Rsrc;                   // Current chart source rectangle


      int         nRefpoint;
      Refpoint    *pRefTable;


      int         nColorSize;

      CachedLine  *pLineCache;

      oernc_inStream     *ifs_hdr;
      wxFileInputStream     *ifss_bitmap;
      wxBufferedInputStream *ifs_bitmap;

      wxString          *pBitmapFilePath;

      unsigned char     *ifs_buf;
      unsigned char     *ifs_buf_decrypt;

      unsigned char     *ifs_bufend;
      int               ifs_bufsize;
      unsigned char     *ifs_lp;
      int               ifs_file_offset;
      int               nFileOffsetDataStart;
      int               m_nLineOffset;

      GeoRef            cPoints;

      double            wpx[12], wpy[12], pwx[12], pwy[12];     // Embedded georef coefficients
      bool              bHaveEmbeddedGeoref;

      opncpnPalette     *pPalettes[N_BSB_COLORS];

      BSB_Color_Capability m_mapped_color_index;

//    Integer digital scale value above which bilinear scaling is not allowed,
//      and subsampled scaling must be performed
      int         m_bilinear_limit;


      bool        bUseLineCache;

      float       m_LonMax;
      float       m_LonMin;
      float       m_LatMax;
      float       m_LatMin;

      int         *pPalette;
      PaletteDir  palette_direction;

      bool        bGeoErrorSent;

      double      m_pi_ppm_avg;              // Calculated true scale factor of the 1X chart,
                                          // pixels per meter

      double      m_piraster_scale_factor;        // exact scaling factor for pixel oversampling calcs

      bool      m_bIDLcross;

      wxRegion  m_last_region;

      int       m_b_cdebug;

      double    m_proj_lat, m_proj_lon;



      int               m_global_color_scheme;
      bool      m_bImageReady;


      //    Chart region coverage information
      //    Charts may have multiple valid regions within the lat/lon box described by the chart extent
      //    The following table structure contains this embedded information

      //    Typically, BSB charts will contain only one entry, corresponding to the PLY information in the chart header
      //    ENC charts often contain multiple entries

      int GetCOVREntries(){ return  m_nCOVREntries; }
      int GetCOVRTablePoints(int iTable){ return m_pCOVRTablePoints[iTable]; }
      int  GetCOVRTablenPoints(int iTable){ return m_pCOVRTablePoints[iTable]; }
      float *GetCOVRTableHead(int iTable){ return m_pCOVRTable[iTable]; }

      int         m_nCOVREntries;                       // number of coverage table entries
      int         *m_pCOVRTablePoints;                  // int table of number of points in each coverage table entry
      float       **m_pCOVRTable;                       // table of pointers to list of floats describing valid COVR


protected:

      double            m_lon_datum_adjust;             // Add these numbers to WGS84 position to obtain internal chart position
      double            m_lat_datum_adjust;

      wxBitmap          *m_pBMPThumb;
      int               m_thumbcs;

      PlugIn_ViewPort  m_vp_render_last;
      wxCriticalSection m_critSect;
      unsigned char     *m_imageComp;
      unsigned char     *m_imageMap;
      size_t             m_lenImageMap;
      int                m_nColors;
      
      std::string m_chartInfo,  m_chartInfoEdition, m_chartInfoExpirationDate;
      std::string m_chartInfoShow, m_chartInfoEULAShow, m_chartInfoDisappearingDate;



};



//--------------------

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

// parameters for calling the high-level interface lmfit
//   ( lmfit.c provides lm_initialize_control which sets default values ):
typedef struct {
    double ftol;       // relative error desired in the sum of squares.
    double xtol;       // relative error between last two approximations.
    double gtol;       // orthogonality desired between fvec and its derivs.
    double epsilon;    // step used to calculate the jacobian.
    double stepbound;  // initial bound to steps in the outer loop.
    double fnorm;      // norm of the residue vector fvec.
    int maxcall;       // maximum number of iterations.
    int nfev;          // actual number of iterations.
    int info;          // status of minimization.
} lm_control_type;


// the subroutine that calculates fvec:
typedef void (lm_evaluate_ftype) (
        double* par, int m_dat, double* fvec, void *data, int *info );
// default implementation therof, provided by lm_eval.c:
void lm_evaluate_default (
        double* par, int m_dat, double* fvec, void *data, int *info );

// the subroutine that informs about fit progress:
typedef void (lm_print_ftype) (
        int n_par, double* par, int m_dat, double* fvec, void *data,
    int iflag, int iter, int nfev );
// default implementation therof, provided by lm_eval.c:
void lm_print_default (
        int n_par, double* par, int m_dat, double* fvec, void *data,
    int iflag, int iter, int nfev );

// compact high-level interface:
void lm_initialize_control( lm_control_type *control );
void lm_minimize ( int m_dat, int n_par, double* par,
                   lm_evaluate_ftype *evaluate, lm_print_ftype *printout,
                   void *data, lm_control_type *control );
double lm_enorm( int, double* );

// low-level interface for full control:
void lm_lmdif( int m, int n, double* x, double* fvec, double ftol, double xtol,
               double gtol, int maxfev, double epsfcn, double* diag, int mode,
               double factor, int *info, int *nfev,
               double* fjac, int* ipvt, double* qtf,
               double* wa1, double* wa2, double* wa3, double* wa4,
               lm_evaluate_ftype *evaluate, lm_print_ftype *printout,
               void *data );


#ifndef _LMDIF
extern char *lm_infmsg[];
extern char *lm_shortmsg[];
#endif

//      This is an opaque (to lmfit) structure set up before the call to lmfit()
typedef struct {
    double* user_tx;
    double* user_ty;
    double* user_y;
    double (*user_func)( double user_tx_point, double user_ty_point, int n_par, double* par );
    int     print_flag;
    int     n_par;
} lm_data_type;





#endif




