/*! \page page_install ObjCryst++ Download & Installation Notes
*
*	This project is still in active development. If you encounter some problem
*  with part of the library, please contact me at vincefn@users.sourceforge.net
*  and I'll try to help.
*
*  Also note that if you want to develop something using ObjCryst++, I recommend 
*  to contact me so that I can help you and avoid possible incompatibilities.
*  There are numerous things I want to implement, so an incompatibility is always
*  possible...
*
*  YOu should also subscribe to the mailing-lists at: http://sourceforge.net/mail/?group_id=27546
*  and most especially the "objcryst-devel" and "objcryst-announce".
*
*\section download Download
*     You can download the ObjCryst library on the 
*     <a href="http://www.sourceforge.net/projects/objcryst/">ObjCryst project page</a>
*     on SourceForge. You will need to download the ObjCryst tar.gz, as well as
*     the 'external libraries' tar.gz archives (the latter include libraries which are
*     used by this project, but which are not part of it). Simply uncompress both archives
*     in the same directory. This will create the atominfo, sglite and newmat directories,
*     as well as the ObjCryst directory. This documentation is in ObjCryst/doc/html/
*		Note that currently the core ObjCryst++ library cannot be downloaded from an
*		archive, until the next version is available. You can use the CVS (see next section)
*		to get the current version, or contact me.
*
*		If you want to use the Graphical Interface part of the library (wxCryst), e.g.
*	   if you want to compile Fox, you will also need to download wxWindows
*		from http://www.wxwindows.org. Depending on your OS, you will need wxGTK-2.2.7
*		(for Linux), or wxMSW-2.2.7 (for Windows).
*
*\section cvs CVS Repository
*		The last version of the library can be obtained through its CVS repository.
*		Under linux, to download the first time the ObjCryst++ directory, first
*		login in the CVS repository (give a blank password when asked):
*
*		cvs -d:pserver:%anonymous@cvs.objcryst.sourceforge.net:/cvsroot/objcryst login
*
*		then download ObjCryst:
*
*		cvs -d:pserver:%anonymous@cvs.objcryst.sourceforge.net:/cvsroot/objcryst checkout ObjCryst
*
*		once you have done this once, you can update to the current version by typing
*		"cvs update" (with a blank password) at the root of the ObjCryst directory.
*
*		For further questions about cvs: http://www.cvshome.org/
*
*\section compile Compiling
*     
*\par ANSI C++ Compliance
*     The ObjCryst++ library makes use of some (relatively) recent C++ features, such as
*     templates, exceptions, mutable members. This implies that you need a compiler with
*     reasonable C++ compliance in order to use this library. Currently tested are the
*		GNU Compiler gcc on Linux and the Free Borland C++ compiler v5.5 under Windows
*		(Metrowerks Codewarrior Pro 6 on MacOS should work, too, but has not been
*      tested recently).
*\par Makefiles
*     Under linux (resp. windows), copy the rules-gnu.mak (resp. rules-bc32.mak)
*  	to rules.mak at the root of the ObjCryst directory. You can edit this file
*		to check the paths and (for Windows) some compiling options.
*
*\section platform Platforms
*\par Linux
*     The library is developped on a Linux computer, using the <a href="http://gcc.gnu.org">
*     GNU compiler gcc </a>, version 2.95.3 (release) or more recent (not tested yet on
*		gcc 3.0+).
*\par Windows
*     The library has been tested under windows (NT,98) using the free Borland C++
*		compiler.
*\par Macintosh
*     The library can also be compiled on MacOS, using <a href="http://www.metrowerks.com">
*     Metrowerks Codewarrior Pro 6 </a> (version 5 should work, too). Note that no tests have
*		been made recently on the macintosh.
*
*\par wxWindows
*		To use the graphical part of ObjCryst++ (wxCryst), you must first compile
*		wxWindows. To do this under linux, in the wxWindows directory:
*
*		./configure --with-opengl --disable-shared
*		make FINAL=1    (this will take some time)
*
*		Under windows, follow the compile instruction in wxWindows doc, and make sure
*  	you (i) activate opengl support and (ii) deactivate debug-context in setup.h.
*     you can leave all other options at default values.
*		
*\section example Example
*		There is a short example in the ObjCryst/example/pbso4 directory. 
*\par Linux
*     Under linux, change to the ObjCryst/example/pbso4 directoryand simply type:
*
*		'make -f gnu.mak wxcryst=1 opengl=1 debug=0 all'.
*
*		(use wxCryst=0 if you do not want the Graphical interface, and debug=1 for
*		faster compiling and debugging messages. Not that anytime you change one of
*		these options, you need to clean up all directories: type make -f gnu.mak clean
*		at the root of the ObjCryst directory)
*
*		That should compile all necessary libraries and programs.
*     You can try the different example programs, running on PbSO4, on neutron
*		and X-Ray powder diffraction, as well as X-Ray single crystal 
*		(in fact, extracted intensities)
*\par Windows
*     Under Windows (a DOS command prompt), change to the ObjCryst/example/pbso4 and do:
*
*		make -f bc32.mak all'.
*
*     \subsection example_result
*     On a 500 Mhz linux box, pbso4-xray2 (using only the first 80 reflections and no
*     dynamical occupancy correction),
*     50000 trials are done in less than 30 s (for 5 independent atoms, each with 4
*     symetrics (not counting the center of symmetry): that's 
*     80*50000*5*4/27.2 = 2.9 10^6 reflection.atom per second).
*     For pbso4-xray and pbso4-neutron, it takes 2 to 3 more time, using only the first
*     90 degrees of the powder pattern (about 80 reflections again), because a full
*     powder spectrum is generated (applying profiles is slow), and dynamical occupancy
*     correction is used to take special positions into account (computing distances
*     takes a lot of time). On the final structure output, note that the population consists
*     of two factor, the first (1.0 here) is the 'real' occupancy of the site, and the
*     second is the dynamical correction due to the fact that several identical atoms
*     overlap (For PbSo4, all atoms should be at 0.5 dyn corr).
*
*\section fox Fox
*		To compile Fox, simply go to the ObjCryst/wxCryst directory and do:
*		- under Linux  : make -f gnu.mak wxcryst=1 opengl=1 debug=0 all
*		- under windows: make -f bc32.mak all
*
*		For both, you will of course need to have compiled wxWindows beforehand.
*/
