prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=${prefix}
libdir=${exec_prefix}/lib@LIB_SUFFIX@
includedir=${prefix}/include/pixel

Name: gfxbox2
Description: Graphics Audio Multimedia and Processing Library
Version: @gfxbox2_VERSION_LONG@

Libs: -L${libdir} -lgfxbox2
Cflags: -I${includedir}
