API VERSION: 1.0
API SOURCE: NASA/JPL Horizons API

*******************************************************************************
 Revised: April 12, 2021                Venus                           299 / 2
 
 PHYSICAL DATA (updated 2020-Oct-19):
  Vol. Mean Radius (km) =  6051.84+-0.01 Density (g/cm^3)      =  5.204
  Mass x10^23 (kg)      =    48.685      Volume (x10^10 km^3)  = 92.843
  Sidereal rot. period  =   243.018484 d Sid. Rot. Rate (rad/s)= -0.00000029924
  Mean solar day        =   116.7490 d   Equ. gravity  m/s^2   =  8.870
  Mom. of Inertia       =     0.33       Core radius (km)      = ~3200
  Geometric Albedo      =     0.65       Potential Love # k2   = ~0.25
  GM (km^3/s^2)         = 324858.592     Equatorial Radius, Re = 6051.893 km
  GM 1-sigma (km^3/s^2) =    +-0.006     Mass ratio (Sun/Venus)= 408523.72
  Atmos. pressure (bar) =  90            Max. angular diam.    =   60.2"
  Mean Temperature (K)  = 735            Visual mag. V(1,0)    =   -4.40
  Obliquity to orbit    = 177.3 deg      Hill's sphere rad.,Rp =  167.1
  Sidereal orb. per., y =   0.61519726   Orbit speed, km/s     =   35.021
  Sidereal orb. per., d = 224.70079922   Escape speed, km/s    =   10.361
                                 Perihelion  Aphelion    Mean
  Solar Constant (W/m^2)         2759         2614       2650
  Maximum Planetary IR (W/m^2)    153         153         153
  Minimum Planetary IR (W/m^2)    153         153         153
*******************************************************************************


*******************************************************************************
Ephemeris / WWW_USER Sat Jul  6 18:19:25 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Venus (299)                     {source: DE441}
Center body name: Sun (10)                        {source: DE441}
Center-site name: BODY CENTER
*******************************************************************************
Start time      : A.D. 2024-Jan-01 00:00:00.0000 TDB
Stop  time      : A.D. 2024-Jan-01 00:00:01.0000 TDB
Step-size       : 60 minutes
*******************************************************************************
Center geodetic : 0.0, 0.0, 0.0                   {E-lon(deg),Lat(deg),Alt(km)}
Center cylindric: 0.0, 0.0, 0.0                   {E-lon(deg),Dxy(km),Dz(km)}
Center radii    : 695700.0, 695700.0, 695700.0 km {Equator_a, b, pole_c}
Output units    : KM-S
Calendar mode   : Mixed Julian/Gregorian
Output type     : GEOMETRIC cartesian states
Output format   : 3 (position, velocity, LT, range, range-rate)
Reference frame : Ecliptic of J2000.0
*******************************************************************************
JDTDB
   X     Y     Z
   VX    VY    VZ
   LT    RG    RR
*******************************************************************************
$$SOE
2460310.500000000 = A.D. 2024-Jan-01 00:00:00.0000 TDB 
 X =-1.069987422398024E+08 Y =-1.145572515113905E+07 Z = 6.016588327139664E+06
 VX= 3.513460276994624E+00 VY=-3.497755629371660E+01 VZ=-6.830913209445484E-01
 LT= 3.595097400571293E+02 RG= 1.077783086466678E+08 RR= 1.915743696730531E-01
$$EOE
*******************************************************************************
 
TIME

  Barycentric Dynamical Time ("TDB" or T_eph) output was requested. This
continuous coordinate time is equivalent to the relativistic proper time
of a clock at rest in a reference frame co-moving with the solar system
barycenter but outside the system's gravity well. It is the independent
variable in the solar system relativistic equations of motion.

  TDB runs at a uniform rate of one SI second per second and is independent
of irregularities in Earth's rotation.
 
CALENDAR SYSTEM

  Mixed calendar mode was active such that calendar dates after AD 1582-Oct-15
(if any) are in the modern Gregorian system. Dates prior to 1582-Oct-5 (if any)
are in the Julian calendar system, which is automatically extended for dates
prior to its adoption on 45-Jan-1 BC.  The Julian calendar is useful for
matching historical dates. The Gregorian calendar more accurately corresponds
to the Earth's orbital motion and seasons. A "Gregorian-only" calendar mode is
available if such physical events are the primary interest.

REFERENCE FRAME AND COORDINATES

  Ecliptic at the standard reference epoch

    Reference epoch: J2000.0
    X-Y plane: adopted Earth orbital plane at the reference epoch
               Note: IAU76 obliquity of 84381.448 arcseconds wrt ICRF X-Y plane
    X-axis   : ICRF
    Z-axis   : perpendicular to the X-Y plane in the directional (+ or -) sense
               of Earth's north pole at the reference epoch.

  Symbol meaning:

    JDTDB    Julian Day Number, Barycentric Dynamical Time
      X      X-component of position vector (km)
      Y      Y-component of position vector (km)
      Z      Z-component of position vector (km)
      VX     X-component of velocity vector (km/sec)                           
      VY     Y-component of velocity vector (km/sec)                           
      VZ     Z-component of velocity vector (km/sec)                           
      LT     One-way down-leg Newtonian light-time (sec)
      RG     Range; distance from coordinate center (km)
      RR     Range-rate; radial velocity wrt coord. center (km/sec)

ABERRATIONS AND CORRECTIONS

 Geometric state vectors have NO corrections or aberrations applied.

Computations by ...

    Solar System Dynamics Group, Horizons On-Line Ephemeris System
    4800 Oak Grove Drive, Jet Propulsion Laboratory
    Pasadena, CA  91109   USA

    General site: https://ssd.jpl.nasa.gov/
    Mailing list: https://ssd.jpl.nasa.gov/email_list.html
    System news : https://ssd.jpl.nasa.gov/horizons/news.html
    User Guide  : https://ssd.jpl.nasa.gov/horizons/manual.html
    Connect     : browser        https://ssd.jpl.nasa.gov/horizons/app.html#/x
                  API            https://ssd-api.jpl.nasa.gov/doc/horizons.html
                  command-line   telnet ssd.jpl.nasa.gov 6775
                  e-mail/batch   https://ssd.jpl.nasa.gov/ftp/ssd/hrzn_batch.txt
                  scripts        https://ssd.jpl.nasa.gov/ftp/ssd/SCRIPTS
    Author      : Jon.D.Giorgini@jpl.nasa.gov
*******************************************************************************
