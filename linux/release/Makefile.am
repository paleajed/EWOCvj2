VPATH=../../src
bin_PROGRAMS = EWOCvj2
EWOCvj2_SOURCES = start.cpp osc.cpp loopstation.cpp node.cpp bins.cpp mixer.cpp program.cpp tinyfiledialogs.c
abcdir=${prefix}/share/EWOCvj2
abc_DATA = ../../shader.fs ../../shader.vs
fontdir=${prefix}/share/fonts/EWOCvj2
font_DATA = ../../expressway.ttf
mimedir=${prefix}/share/mime/packages
mimeupdir = ${prefix}/share/mime
mime_DATA = ../../src/ewocvj2-layer.xml ../../src/ewocvj2-deck.xml ../../src/ewocvj2-mix.xml ../../src/ewocvj2-state.xml ../../src/ewocvj2-shelf.xml 

CFLAGS = -O3
CXXFLAGS = -std=c++17 -fpermissive -O3 -pthread -I/usr/include/rtmidi -I/usr/include/ffmpeg -I/usr/include/freetype2
AM_CPPFLAGS = -DDATADIR=\"$(abcdir)\" -DFONTDIR=\"$(fontdir)\"

paths.h : $(srcdir)/paths.h.in config.status

install-data-hook:
	update-mime-database $(mimeupdir)
	
uninstall-hook:
	update-mime-database $(mimeupdir)