API VERSION: 1.0
API SOURCE: NASA/JPL Horizons API

*******************************************************************************
 Revised: Apr 03, 2024              134340 Pluto                            999
 
 Pre-computed solution PLU060/DE440. Fit to post New Horizons encounter and
 Gaia data through 2023. For discussion, see ...

 M. Brozovic, R. A. Jacobson (2024) "Post-New Horizons orbits and masses
 for the satellites of Pluto". AJ (in press)

 PHYSICAL DATA (updated 2021-Jun-07; Mc= Charon mass, radius is IAU 2015):
  Mass x10^22 (kg)      = 1.307+-0.018    Volume, 10^10 km^3    = 0.697
  GM (planet) km^3/s^2  = 869.326         Density (R=1195 km)   = 1.86 g/cm^3
  GM 1-sigma, km^3/s^2  = 0.4             Surface gravity       = 0.611 m/s^2 
  Vol. mean radius (km) = 1188.3+-1.6     Mass ratio (Mc/Mp)    = 0.122
  Sidereal rot. period  = 153.29335198 h  Sid. rot. rat, rad/s  = 0.0000113856
  Mean solar day, h     = 153.2820        Mean orbit velocity   = 4.67 km/s
  Sidereal orbit period = 249.58932 yr    Escape speed, km/s    = 1.21            
                                 Perihelion  Aphelion    Mean
  Solar Constant (W/m^2)         1.56        0.56        0.88
  Maximum Planetary IR (W/m^2)   0.8         0.3         0.5
  Minimum Planetary IR (W/m^2)   0.8         0.3         0.5
*******************************************************************************


*******************************************************************************
Ephemeris / WWW_USER Sat Jul  6 18:19:29 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Pluto (999)                     {source: plu060_merged}
Center body name: Sun (10)                        {source: plu060_merged}
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
 X = 2.574575382744127E+09 Y =-4.538596532403562E+09 Z =-2.587050606457522E+08
 VX= 4.844463314470517E+00 VY= 1.482549367525443E+00 VZ=-1.568756846977261E+00
 LT= 1.742668103564216E+04 RG= 5.224387542457147E+09 RR= 1.177092598650932E+00
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
