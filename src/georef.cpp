/******************************************************************************
 *
 * Project:  OpenCPN
 * Purpose:  OpenCPN Georef utility
 * Author:   David Register
 *
 ***************************************************************************
 *   Copyright (C) 2010 by David S. Register   *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,  USA.             *
 ***************************************************************************

 ***************************************************************************
 *  Parts of this file were adapted from source code found in              *
 *  John F. Waers (jfwaers@csn.net) public domain program MacGPS45         *
 ***************************************************************************
 */



#include <vector>
#include <utility>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "georef.h"
#include "cutil.h"


#ifdef _WIN32
#define snprintf mysnprintf
#endif

#if !defined(NAN)
static const long long lNaN = 0xfff8000000000000;
#define NAN (*(double*)&lNaN)
#endif



//  ellipsoid: index into the gEllipsoid[] array, in which
//             a is the ellipsoid semimajor axis
//             invf is the inverse of the ellipsoid flattening f
//  dx, dy, dz: ellipsoid center with respect to WGS84 ellipsoid center
//    x axis is the prime meridian
//    y axis is 90 degrees east longitude
//    z axis is the axis of rotation of the ellipsoid

// The following values for dx, dy and dz were extracted from the output of
// the GARMIN PCX5 program. The output also includes values for da and df, the
// difference between the reference ellipsoid and the WGS84 ellipsoid semi-
// major axis and flattening, respectively. These are replaced by the
// data contained in the structure array gEllipsoid[], which was obtained from
// the Defence Mapping Agency document number TR8350.2, "Department of Defense
// World Geodetic System 1984."

struct DATUM const gDatum[] = {
//     name               ellipsoid   dx      dy       dz
      { "Adindan",                5,   -162,    -12,    206 },    // 0
      { "Afgooye",               15,    -43,   -163,     45 },    // 1
      { "Ain el Abd 1970",       14,   -150,   -251,     -2 },    // 2
      { "Anna 1 Astro 1965",      2,   -491,    -22,    435 },    // 3
      { "Arc 1950",               5,   -143,    -90,   -294 },    // 4
      { "Arc 1960",               5,   -160,     -8,   -300 },    // 5
      { "Ascension Island  58",  14,   -207,    107,     52 },    // 6
      { "Astro B4 Sorol Atoll",  14,    114,   -116,   -333 },    // 7
      { "Astro Beacon  E ",      14,    145,     75,   -272 },    // 8
      { "Astro DOS 71/4",        14,   -320,    550,   -494 },    // 9
      { "Astronomic Stn  52",    14,    124,   -234,    -25 },    // 10
      { "Australian Geod  66",    2,   -133,    -48,    148 },    // 11
      { "Australian Geod  84",    2,   -134,    -48,    149 },    // 12
      { "Bellevue (IGN)",        14,   -127,   -769,    472 },    // 13
      { "Bermuda 1957",           4,    -73,    213,    296 },    // 14
      { "Bogota Observatory",    14,    307,    304,   -318 },    // 15
      { "Campo Inchauspe",       14,   -148,    136,     90 },    // 16
      { "Canton Astro 1966",     14,    298,   -304,   -375 },    // 17
      { "Cape",                   5,   -136,   -108,   -292 },    // 18
      { "Cape Canaveral",         4,     -2,    150,    181 },    // 19
      { "Carthage",               5,   -263,      6,    431 },    // 20
      { "CH-1903",                3,    674,     15,    405 },    // 21
      { "Chatham 1971",          14,    175,    -38,    113 },    // 22
      { "Chua Astro",            14,   -134,    229,    -29 },    // 23
      { "Corrego Alegre",        14,   -206,    172,     -6 },    // 24
      { "Djakarta (Batavia)",     3,   -377,    681,    -50 },    // 25
      { "DOS 1968",              14,    230,   -199,   -752 },    // 26
      { "Easter Island 1967",    14,    211,    147,    111 },    // 27
      { "European 1950",         14,    -87,    -98,   -121 },    // 28
      { "European 1979",         14,    -86,    -98,   -119 },    // 29
      { "Finland Hayford",       14,    -78,   -231,    -97 },    // 30
      { "Gandajika Base",        14,   -133,   -321,     50 },    // 31
      { "Geodetic Datum  49",    14,     84,    -22,    209 },    // 32
      { "Guam 1963",              4,   -100,   -248,    259 },    // 33
      { "GUX 1 Astro",           14,    252,   -209,   -751 },    // 34
      { "Hermannskogel Datum",    3,    682,   -203,    480 },    // 35
      { "Hjorsey 1955",          14,    -73,     46,    -86 },    // 36
      { "Hong Kong 1963",        14,   -156,   -271,   -189 },    // 37
      { "Indian Bangladesh",      6,    289,    734,    257 },    // 38
      { "Indian Thailand",        6,    214,    836,    303 },    // 39
      { "Ireland 1965",           1,    506,   -122,    611 },    // 40
      { "ISTS 073 Astro  69",    14,    208,   -435,   -229 },    // 41
      { "Johnston Island",       14,    191,    -77,   -204 },    // 42
      { "Kandawala",              6,    -97,    787,     86 },    // 43
      { "Kerguelen Island",      14,    145,   -187,    103 },    // 44
      { "Kertau 1948",            7,    -11,    851,      5 },    // 45
      { "L.C. 5 Astro",           4,     42,    124,    147 },    // 46
      { "Liberia 1964",           5,    -90,     40,     88 },    // 47
      { "Luzon Mindanao",         4,   -133,    -79,    -72 },    // 48
      { "Luzon Philippines",      4,   -133,    -77,    -51 },    // 49
      { "Mahe 1971",              5,     41,   -220,   -134 },    // 50
      { "Marco Astro",           14,   -289,   -124,     60 },    // 51
      { "Massawa",                3,    639,    405,     60 },    // 52
      { "Merchich",               5,     31,    146,     47 },    // 53
      { "Midway Astro 1961",     14,    912,    -58,   1227 },    // 54
      { "Minna",                  5,    -92,    -93,    122 },    // 55
      { "NAD27 Alaska",           4,     -5,    135,    172 },    // 56
      { "NAD27 Bahamas",          4,     -4,    154,    178 },    // 57
      { "NAD27 Canada",           4,    -10,    158,    187 },    // 58
      { "NAD27 Canal Zone",       4,      0,    125,    201 },    // 59
      { "NAD27 Caribbean",        4,     -7,    152,    178 },    // 60
      { "NAD27 Central",          4,      0,    125,    194 },    // 61
      { "NAD27 CONUS",            4,     -8,    160,    176 },    // 62
      { "NAD27 Cuba",             4,     -9,    152,    178 },    // 63
      { "NAD27 Greenland",        4,     11,    114,    195 },    // 64
      { "NAD27 Mexico",           4,    -12,    130,    190 },    // 65
      { "NAD27 San Salvador",     4,      1,    140,    165 },    // 66
      { "NAD83",                 11,      0,      0,      0 },    // 67
      { "Nahrwn Masirah Ilnd",    5,   -247,   -148,    369 },    // 68
      { "Nahrwn Saudi Arbia",     5,   -231,   -196,    482 },    // 69
      { "Nahrwn United Arab",     5,   -249,   -156,    381 },    // 70
      { "Naparima BWI",          14,     -2,    374,    172 },    // 71
      { "Observatorio 1966",     14,   -425,   -169,     81 },    // 72
      { "Old Egyptian",          12,   -130,    110,    -13 },    // 73
      { "Old Hawaiian",           4,     61,   -285,   -181 },    // 74
      { "Oman",                   5,   -346,     -1,    224 },    // 75
      { "Ord Srvy Grt Britn",     0,    375,   -111,    431 },    // 76
      { "Pico De Las Nieves",    14,   -307,    -92,    127 },    // 77
      { "Pitcairn Astro 1967",   14,    185,    165,     42 },    // 78
      { "Prov So Amrican  56",   14,   -288,    175,   -376 },    // 79
      { "Prov So Chilean  63",   14,     16,    196,     93 },    // 80
      { "Puerto Rico",            4,     11,     72,   -101 },    // 81
      { "Qatar National",        14,   -128,   -283,     22 },    // 82
      { "Qornoq",                14,    164,    138,   -189 },    // 83
      { "Reunion",               14,     94,   -948,  -1262 },    // 84
      { "Rome 1940",             14,   -225,    -65,      9 },    // 85
      { "RT 90",                  3,    498,    -36,    568 },    // 86
      { "Santo (DOS)",           14,    170,     42,     84 },    // 87
      { "Sao Braz",              14,   -203,    141,     53 },    // 88
      { "Sapper Hill 1943",      14,   -355,     16,     74 },    // 89
      { "Schwarzeck",            21,    616,     97,   -251 },    // 90
      { "South American  69",    16,    -57,      1,    -41 },    // 91
      { "South Asia",             8,      7,    -10,    -26 },    // 92
      { "Southeast Base",        14,   -499,   -249,    314 },    // 93
      { "Southwest Base",        14,   -104,    167,    -38 },    // 94
      { "Timbalai 1948",          6,   -689,    691,    -46 },    // 95
      { "Tokyo",                  3,   -128,    481,    664 },    // 96
      { "Tristan Astro 1968",    14,   -632,    438,   -609 },    // 97
      { "Viti Levu 1916",         5,     51,    391,    -36 },    // 98
      { "Wake-Eniwetok  60",     13,    101,     52,    -39 },    // 99
      { "WGS 72",                19,      0,      0,      5 },    // 100
      { "WGS 84",                20,      0,      0,      0 },    // 101
      { "Zanderij",              14,   -265,    120,   -358 },    // 102
      { "WGS_84",                20,      0,      0,      0 },    // 103
      { "WGS-84",                20,      0,      0,      0 },    // 104
      { "EUROPEAN DATUM 1950",   14,    -87,    -98,   -121 }, 
      { "European 1950 (Norway Finland)", 14, -87, -98, -121}, 
      { "ED50",                  14,    -87,    -98,   -121 }, 
      { "RT90 (Sweden)",          3,    498,    -36,    568 }, 
      { "Monte Mario 1940",      14,   -104,    -49,     10 },
      { "Ord Surv of Gr Britain 1936", 0, 375, -111,    431 }, 
      { "South American 1969",  16,     -57,      1,    -41 },
      { "PULKOVO 1942 (2)",     15,      25,   -141,    -79 },
      { "EUROPEAN DATUM",       14,     -87,    -98,   -121 }, 
      { "BERMUDA DATUM 1957",    4,     -73,    213,    296 },
      { "COA",                  14,    -206,    172,     -6 },
      { "COABR",                14,    -206,    172,     -6 },
      { "Roma 1940",            14,    -225,    -65,      9 },
      { "ITALIENISCHES LANDESNETZ",14,  -87,    -98,   -121 },
      { "HERMANSKOGEL DATUM",     3,    682,   -203,    480 },
      { "AGD66",                  2 ,  -128,    -52,    153 },
      { "ED",                    14,    -87,    -98,   -121 },
      { "EUROPEAN 1950 (SPAIN AND PORTUGAL)",14,-87,-98,-121},
      { "ED-50",                 14,    -87,    -98,   -121 },
      { "EUROPEAN",              14,    -87,    -98,   -121 },
      { "POTSDAM",               14,    -87,    -98,   -121 },
      { "GRACIOSA SW BASE DATUM",14,   -104,    167,    -38 }, 
      { "WGS 1984",              20,      0,      0,      0 },
      { "WGS 1972",              19,      0,      0,      5 },
      { "WGS",                   19,      0,      0,      5 }

      
};

struct ELLIPSOID const gEllipsoid[] = {
//      name                               a        1/f
      {  "Airy 1830",                  6377563.396, 299.3249646   },    // 0
      {  "Modified Airy",              6377340.189, 299.3249646   },    // 1
      {  "Australian National",        6378160.0,   298.25        },    // 2
      {  "Bessel 1841",                6377397.155, 299.1528128   },    // 3
      {  "Clarke 1866",                6378206.4,   294.9786982   },    // 4
      {  "Clarke 1880",                6378249.145, 293.465       },    // 5
      {  "Everest (India 1830)",       6377276.345, 300.8017      },    // 6
      {  "Everest (1948)",             6377304.063, 300.8017      },    // 7
      {  "Modified Fischer 1960",      6378155.0,   298.3         },    // 8
      {  "Everest (Pakistan)",         6377309.613, 300.8017      },    // 9
      {  "Indonesian 1974",            6378160.0,   298.247       },    // 10
      {  "GRS 80",                     6378137.0,   298.257222101 },    // 11
      {  "Helmert 1906",               6378200.0,   298.3         },    // 12
      {  "Hough 1960",                 6378270.0,   297.0         },    // 13
      {  "International 1924",         6378388.0,   297.0         },    // 14
      {  "Krassovsky 1940",            6378245.0,   298.3         },    // 15
      {  "South American 1969",        6378160.0,   298.25        },    // 16
      {  "Everest (Malaysia 1969)",    6377295.664, 300.8017      },    // 17
      {  "Everest (Sabah Sarawak)",    6377298.556, 300.8017      },    // 18
      {  "WGS 72",                     6378135.0,   298.26        },    // 19
      {  "WGS 84",                     6378137.0,   298.257223563 },    // 20
      {  "Bessel 1841 (Namibia)",      6377483.865, 299.1528128   },    // 21
      {  "Everest (India 1956)",       6377301.243, 300.8017      },    // 22
      {  "Struve 1860",                6378298.3,   294.73        }     // 23
      
};

short nDatums = sizeof(gDatum)/sizeof(struct DATUM);



/* define constants */

void datumParams(short datum, double *a, double *es)
{
    extern struct DATUM const gDatum[];
    extern struct ELLIPSOID const gEllipsoid[];

    
    if( datum < nDatums){
        double f = 1.0 / gEllipsoid[gDatum[datum].ellipsoid].invf;    // flattening
        if(es)
            *es = 2 * f - f * f;                                          // eccentricity^2
        if(a)
            *a = gEllipsoid[gDatum[datum].ellipsoid].a;                   // semimajor axis
    }
    else{
        double f = 1.0 / 298.257223563;    // WGS84
        if(es)
            *es = 2 * f - f * f;              
        if(a)
            *a = 6378137.0;                   
    }
}

static int datumNameCmp(const char *n1, const char *n2)
{
	while(*n1 || *n2)
	{
		if (*n1 == ' ')
			n1++;
		else if (*n2 == ' ')
			n2++;
		else if (toupper(*n1) == toupper(*n2))
			n1++, n2++;
		else
			return 1;	// No string match
	}
	return 0;	// String match
}

static int isWGS84(int i)
{
    // DATUM_INDEX_WGS84 is an optimization
    // but there's more than on in gDatum table
    if (i == DATUM_INDEX_WGS84)
        return i;

    if (gDatum[i].ellipsoid != gDatum[DATUM_INDEX_WGS84].ellipsoid)
        return i;

    if (gDatum[i].dx != gDatum[DATUM_INDEX_WGS84].dx)
        return i;

    if (gDatum[i].dy != gDatum[DATUM_INDEX_WGS84].dy)
        return i;

    if (gDatum[i].dz != gDatum[DATUM_INDEX_WGS84].dz)
        return i;
        
    return DATUM_INDEX_WGS84;
}

int GetDatumIndex(const char *str)
{
      int i = 0;
      while (i < (int)nDatums)
      {
            if(!datumNameCmp(str, gDatum[i].name))
            {
                  return isWGS84(i);
            }
            i++;
      }

      return -1;
}


/* --------------------------------------------------------------------------------- *

 *Molodensky
 *In the listing below, the class GeodeticPosition has three members, lon, lat, and h.
 *They are double-precision values indicating the longitude and latitude in radians,
 * and height in meters above the ellipsoid.

 * The source code in the listing below may be copied and reused without restriction,
 * but it is offered AS-IS with NO WARRANTY.

 * Adapted for opencpn by David S. Register

 * --------------------------------------------------------------------------------- */

void MolodenskyTransform (double lat, double lon, double *to_lat, double *to_lon, int from_datum_index, int to_datum_index)
{
    double dlat = 0;
    double dlon = 0;
    
    if( from_datum_index < nDatums){
      const double from_lat = lat * DEGREE;
      const double from_lon = lon * DEGREE;
      const double from_f = 1.0 / gEllipsoid[gDatum[from_datum_index].ellipsoid].invf;    // flattening
      const double from_esq = 2 * from_f - from_f * from_f;                               // eccentricity^2
      const double from_a = gEllipsoid[gDatum[from_datum_index].ellipsoid].a;             // semimajor axis
      const double dx = gDatum[from_datum_index].dx;
      const double dy = gDatum[from_datum_index].dy;
      const double dz = gDatum[from_datum_index].dz;
      const double to_f = 1.0 / gEllipsoid[gDatum[to_datum_index].ellipsoid].invf;        // flattening
      const double to_a = gEllipsoid[gDatum[to_datum_index].ellipsoid].a;                 // semimajor axis
      const double da = to_a - from_a;
      const double df = to_f - from_f;
      const double from_h = 0;


      const double slat = sin (from_lat);
      const double clat = cos (from_lat);
      const double slon = sin (from_lon);
      const double clon = cos (from_lon);
      const double ssqlat = slat * slat;
      const double adb = 1.0 / (1.0 - from_f);  // "a divided by b"

      const double rn = from_a / sqrt (1.0 - from_esq * ssqlat);
      const double rm = from_a * (1. - from_esq) / pow ((1.0 - from_esq * ssqlat), 1.5);

      dlat = (((((-dx * slat * clon - dy * slat * slon) + dz * clat)
                  + (da * ((rn * from_esq * slat * clat) / from_a)))
                  + (df * (rm * adb + rn / adb) * slat * clat)))
            / (rm + from_h);

      dlon = (-dx * slon + dy * clon) / ((rn + from_h) * clat);

    } 
    
    *to_lon = lon + dlon/DEGREE;
    *to_lat = lat + dlat/DEGREE;
//
      return;
}

