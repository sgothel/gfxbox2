API VERSION: 1.0
API SOURCE: NASA/JPL Horizons API

*******************************************************************************
 Revised: April 22, 2021              Neptune                               899
 
 PHYSICAL DATA (update 2021-May-03):
  Mass x10^24 (kg)      = 102.409         Density (g/cm^3)       =  1.638
  Equat. radius (1 bar) = 24766+-15 km    Volume, 10^10 km^3     = 6254     
  Vol. mean radius (km) = 24624+-21       Polar radius (km)      = 24342+-30
  Geometric Albedo      = 0.41            Flattening             =  0.0171
  Sid. rot. period (III)= 16.11+-0.01 hr  Sid. rot. rate (rad/s) =  0.000108338 
  Mean solar day, h     =~16.11 h         
  GM (km^3/s^2)         = 6835099.97      GM 1-sigma (km^3/s^2)  = +-10 
  Equ. grav, ge (m/s^2) = 11.15           Pol. grav, gp (m/s^2)  = 11.41+-0.03
  Visual magnitude V(1,0)= -6.87
  Vis. mag. (opposition)=  +7.84          Obliquity to orbit     = 28.32 deg
  Sidereal orbit period = 164.788501027 y Sidereal orbit period  = 60189 d
  Mean daily motion     = 0.006020076dg/d Mean orbit velocity    =  5.43 km/s 
  Atmos. temp. (1 bar)  =  72+-2 K        Escape speed (1 bar)  =  23.5 km/s     
  Aroche(ice)/Rp        =  2.98           Hill's sphere rad., Rp = 4700
                                 Perihelion  Aphelion    Mean
  Solar Constant (W/m^2)         1.54        1.49        1.51
  Maximum Planetary IR (W/m^2)   0.52        0.52        0.52
  Minimum Planetary IR (W/m^2)   0.52        0.52        0.52
*******************************************************************************


*******************************************************************************
Ephemeris / WWW_USER Sat Jul  6 18:19:28 2024 Pasadena, USA      / Horizons
*******************************************************************************
Target body name: Neptune (899)                   {source: nep102_merged}
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
 X = 4.464446647141849E+09 Y =-2.679158335073845E+08 Z =-9.736583677508335E+07
 VX= 2.818440617089212E-01 VY= 5.469942022851473E+00 VZ=-1.190017755456774E-01
 LT= 1.492211678708158E+04 RG= 4.473538070162248E+09 RR=-4.372816089956009E-02
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
