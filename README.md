BRZDM
=====
 
  1. INTRODUCTION
  2. PREREQUISITES
  3. USAGE
  4. THEMES
  5. BUILDING
  6. INSTALLATION
  7. KNOWN BUGS
  8. BUGS
  9. COPYRIGHT


INTRODUCTION
============

  Brzdm is a simple graphical login manager for X11. It aims
  to be simple, fast and independent from the various desktop
  environments. Text and image display is handled by imlib2.

  It is derived from SLim by Nobuhiro Iwamatsu,
  which was based on the latest release of Login.app by Per Lidén.

  Features:

   - Themes with INI-like configuration
   - PNG support with alpha transparency
   - PNG / JPEG support for backgrounds
   - Wallpaper selection for backgrounds
   - XFT / freetype support
   - Double inputbox support
   - Time Clock Panel support
   - CMake build procedure
   - Wayland support (planned)
   - Kiosk support (planned)
   - Keyutils support (planned)
   - User Photo Panel support (planned)
   - Secure auto-login support (planned)
   - ELogind support (planned)


PREREQUISITES
=============

 This project requires:
 
  * Cross-platform Make (CMake) v2.8.2+
  * GNU Make or equivalent.
  * GCC or an alternative, conformant C++ compiler.
  * Xorg/X11 libraries
  * Imlib2 image library
  * Freetype font library
  * ConsoleKit2 library
  * Nettle encryption library
  * Keyutils keyring library (Upcoming version)
  * ELogind library (Upcoming version)
  


USAGE
=====

  To launch brzdm, execute the brzdm binary, Use the -d option
  if you want it to run as a daemon in the background.

  Enter username and password to login. The desktop file can be
  either /etc/X11/xinit/xinitrc.%desktop or /etc/X11/Sessions/%desktop;
  and, is executed by default. The folder /usr/share/xsessions is
  used by default; and, the sessions entry is used for ordering the
  list of desktops.

  Special usernames (commands configurable in the config file):
  
  - F5/halt: halt the system
  - F6/reboot: reboot the system
  - F7/suspend: suspend the system (unsafe)
  - F8/kiosk: invoke the kiosk app  
  
  Pressing the F11 key executes a user-specified command, see the
  configuration file; the default is to take a screenshot if the
  ImageMagick/GraphicsMagik 'import' program is available.


THEMES
======

  See file THEMES


BUILDING
========

  This project uses the Cross-platform Make (CMake) build system. However, we
  have conveniently provided a wrapper configure script and Makefile so that
  the typical build invocation of "./configure" followed by "make" will work.
  For a list of all possible build targets, use the command "make help".

  NOTE: Please, do not delete the top-level Makefile.


INSTALLATION
============

  Once the project has been built (see "BUILDING"),
  execute "sudo make install". See the INSTALL file


BUGS
====

  Debian-based installations may run into problems when configuring.  
  An unresolved font access problem, when running on 64-bit machines.  
  Bug reports, patches and suggestions are much appreciated.  
  See the GitHub account https://dev-breeze-com.github.io/brzdm


COPYRIGHT
=========

  Brzdm is copyright (c) 2015 Pierre Innocent, All rights reserved  
  licensed through the GNU General Public License V3.0. 

  Slim is copyright (c) 2004-06 by Simone Rota, Johannes Winkelmann,
  Nobuhiro Iwamatsu and is available under the GNU General Public
  License. See the COPYING file for the complete license.

  Login.app is copyright (c) 1997, 1998 by Per Liden and is 
  licensed through the GNU General Public License. 

