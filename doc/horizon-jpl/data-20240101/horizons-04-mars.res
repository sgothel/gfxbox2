API VERSION: 1.0
API SOURCE: NASA/JPL Horizons API

*******************************************************************************
 Revised: June 21, 2016                 Mars                            499 / 4
 
 PHYSICAL DATA (updated 2019-Oct-29):
  Vol. mean radius (km) = 3389.92+-0.04   Density (g/cm^3)      =  3.933(5+-4)
  Mass x10^23 (kg)      =    6.4171       Flattening, f         =  1/169.779
  Volume (x10^10 km^3)  =   16.318        Equatorial radius (km)=  3396.19
  Sidereal rot. period  =   24.622962 hr  Sid. rot. rate, rad/s =  0.0000708822 
  Mean solar day (sol)  =   88775.24415 s Polar gravity m/s^2   =  3.758
  Core radius (km)      = ~1700           Equ. gravity  m/s^2   =  3.71
  Geometric Albedo      =    0.150                                              

  GM (km^3/s^2)         = 42828.375214    Mass ratio (Sun/Mars) = 3098703.59
  GM 1-sigma (km^3/s^2) = +- 0.00028      Mass of atmosphere, kg= ~ 2.5 x 10^16
  Mean temperature (K)  =  210            Atmos. pressure (bar) =    0.0056 
  Obliquity to orbit    =   25.19 deg     Max. angular diam.    =  17.9"
  Mean sidereal orb per =    1.88081578 y Visual mag. V(1,0)    =  -1.52
  Mean sidereal orb per =  686.98 d       Orbital speed,  km/s  =  24.13
  Hill's sphere rad. Rp =  319.8          Escape speed, km/s    =   5.027
                                 Perihelion  Aphelion    Mean
  Solar Constant (W/m^2)         717         493         589
  Maximum Planetary IR (W/m^2)   470         315         390
  Minimum Planetary IR (W/m^2)    30          30          30
*******************************************************************************


*******************************************************************************
Ephemeris / WWW_USER Sat Jul  6 18:19:26 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Mars (499)                      {source: mar097}
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
 X =-4.388577457378983E+07 Y =-2.170849264747524E+08 Z =-3.473007284583151E+06
 VX= 2.466191455128526E+01 VY=-2.722160161977370E+00 VZ=-6.619819103693254E-01
 LT= 7.388568065491527E+02 RG= 2.215036981454010E+08 RR=-2.207946052541310E+00
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
