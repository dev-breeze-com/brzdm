# brzdm
Brzdm is a graphical display/login manager for X11, using Imlib2

README file for Brzdm

PREREQUISITES

 This project requires:
  * Cross-platform Make (CMake) v2.8.2+
  * GNU Make or equivalent.
  * GCC or an alternative, reasonably conformant C++ compiler.
  * Xorg/X11 libraries
  * Imlib2 image library
  * Nettle encryption library
  * Freetype font library
  * Keyutils keyring library
  * Log4Cxx v0.10.0+ (optional)
  * UnitTest++ (a.k.a. unittest-cpp) (optional)

INTRODUCTION
    Brzdm is a graphical display/login manager for X11. It aims
    to be simple, fast and independent from the various desktop
    environments. Text and image display is handled by Imlib2.

    It is derived from SLim by Nobuhiro Iwamatsu <iwamatsu@nigauri.org>,
    which was based on the latest release of Login.app by Per Lidén.

    Features:
    - External themes and INI-style configuration
    - PNG support with alpha transparency
    - PNG / JPEG support for backgrounds
    - XFT / freetype support
    - Double or single (GDM-style) inputbox support
    - Keyutils authentication support
    - Secure auto-login support
    - User Photo Panel support
    - Time Clock Panel support
    - CMake build procedure

    Planned Features:
    - Wayland support
    - Kiosk support

USAGE
    To launch brzdm, execute the brzdm binary, Use the -d option
    if you want it to run as a daemon in the background.

    Enter username and password to login. The desktop file must reside
    at /etc/X11/xinit/xinitrc.%desktop; and, is executed by default.
    The folder /usr/share/xsessions is accessed; and, the sessions entry
    is used for ordering the list of desktops.

    Special usernames (commands configurable in the config file):
    - F5/halt: halt the system
    - F6/reboot: reboot the system
    - F7/suspend: suspend the system (unsafe)
    - F8/kiosk: invoke the kiosk app
    - F9/console: start console login (optional)
    - ESC/exit: exit Brzdm

    Pressing the F11 key executes a user-specified command, see the
    configuration file; the default is to take a screenshot if the
    ImageMagick/GraphicsMagik 'import' program is available.

THEMES
    See THEMES

BUILDING

 This project uses the Cross-platform Make (CMake) build system. However, a
 conveniently wrapped configure script and Makefile are also provided, so that
 the typical build invocation of "./configure" followed by "make" will work.
 For a list of all possible build targets, use the command "make help".

 NOTE: Please do not delete the top-level Makefile.

INSTALLATION
 Once the project has been built (see "BUILDING"), execute "sudo make install".
 then, see the INSTALL file

COPYRIGHT
    Brzdm is copyright (c) 2015 Tsert.Com <dev@breezeos.com>

    Slim is copyright (c) 2004-06 by Simone Rota, Johannes Winkelmann,
    Nobuhiro Iwamatsu and is available under the GNU General Public
    License. See the COPYING file for the complete license.

    Login.app is copyright (c) 1997, 1998 by Per Liden and is
    licensed through the GNU General Public License.

