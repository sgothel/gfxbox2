API VERSION: 1.0
API SOURCE: NASA/JPL Horizons API

*******************************************************************************
 Revised: January 26, 2022             Saturn                               699
 
 PHYSICAL DATA:
  Mass x10^26 (kg)      = 5.6834          Density (g/cm^3)       =  0.687+-.001
  Equat. radius (1 bar) = 60268+-4 km     Polar radius (km)      = 54364+-10
  Vol. Mean Radius (km) = 58232+-6        Flattening             =  0.09796
  Geometric Albedo      = 0.47            Rocky core mass (Mc/M) =  0.1027
  Sid. rot. period (III)= 10h 39m 22.4s   Sid. rot. rate (rad/s) =  0.000163785 
  Mean solar day, hrs   =~10.656         
  GM (km^3/s^2)         = 37931206.234    GM 1-sigma (km^3/s^2)  = +- 98
  Equ. grav, ge (m/s^2) = 10.44           Pol. grav, gp (m/s^2)  = 12.14+-0.01
  Vis. magnitude V(1,0) = -8.88          
  Vis. mag. (opposition)= +0.67           Obliquity to orbit     = 26.73 deg
  Sidereal orbit period = 29.447498 yr    Sidereal orbit period  = 10755.698 d
  Mean daily motion     = 0.0334979 deg/d Mean orbit velocity    =  9.68 km/s
  Atmos. temp. (1 bar)  = 134+-4 K        Escape speed, km/s    =  35.5          
  Aroche(ice)/Rp        =  2.71           Hill's sphere rad. Rp  = 1100
                                 Perihelion  Aphelion    Mean
  Solar Constant (W/m^2)         16.8        13.6        15.1
  Maximum Planetary IR (W/m^2)    4.7         4.5         4.6
  Minimum Planetary IR (W/m^2)    4.7         4.5         4.6
*******************************************************************************


*******************************************************************************
Ephemeris / WWW_USER Sat Jul  6 18:19:27 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Saturn (699)                    {source: sat441l}
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
 X = 1.345793242617223E+09 Y =-5.559294178115252E+08 Z =-4.389262609579784E+07
 VX= 3.146297313479314E+00 VY= 8.917916155362638E+00 VZ=-2.799382290475703E-01
 LT= 4.859221254410594E+03 RG= 1.456757883825595E+09 RR=-4.881930322645170E-01
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
