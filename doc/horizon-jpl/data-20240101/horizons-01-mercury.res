API VERSION: 1.0
API SOURCE: NASA/JPL Horizons API

*******************************************************************************
 Revised: April 12, 2021             Mercury                            199 / 1

 PHYSICAL DATA (updated 2024-Mar-04):
  Vol. Mean Radius (km) =  2439.4+-0.1    Density (g cm^-3)     = 5.427
  Mass x10^23 (kg)      =     3.302       Volume (x10^10 km^3)  = 6.085  
  Sidereal rot. period  =    58.6463 d    Sid. rot. rate (rad/s)= 0.00000124001
  Mean solar day        =   175.9421 d    Core radius (km)      = ~1600 
  Geometric Albedo      =     0.106       Surface emissivity    = 0.77+-0.06
  GM (km^3/s^2)         = 22031.86855     Equatorial radius, Re = 2440.53 km
  GM 1-sigma (km^3/s^2) =                 Mass ratio (Sun/plnt) = 6023682
  Mom. of Inertia       =     0.33        Equ. gravity  m/s^2   = 3.701     
  Atmos. pressure (bar) = < 5x10^-15      Max. angular diam.    = 11.0"   
  Mean Temperature (K)  = 440             Visual mag. V(1,0)    = -0.42 
  Obliquity to orbit[1] =  2.11' +/- 0.1' Hill's sphere rad. Rp = 94.4 
  Sidereal orb. per.    =  0.2408467 y    Mean Orbit vel.  km/s = 47.362 
  Sidereal orb. per.    = 87.969257  d    Escape vel. km/s      =  4.435
                                 Perihelion  Aphelion    Mean
  Solar Constant (W/m^2)         14462       6278        9126
  Maximum Planetary IR (W/m^2)   12700       5500        8000
  Minimum Planetary IR (W/m^2)   6           6           6
*******************************************************************************


*******************************************************************************
Ephemeris / WWW_USER Sat Jul  6 18:19:24 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Mercury (199)                   {source: DE441}
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
 X =-4.108411877039495E+07 Y = 2.997375954154480E+07 Z = 6.217890408222714E+06
 VX=-3.865743010383652E+01 VY=-3.733889075044869E+01 VZ= 4.944436024774976E-01
 LT= 1.709005228970273E+02 RG= 5.123468783278508E+07 RR= 9.214341551495892E+00
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
