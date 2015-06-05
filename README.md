# mbmp
A QT5 and GStreamer 1.0 series Media Player

MBMP is a fairly conventional media player using QT5 for the user interface and the
GStreamer 1.0 series multi-media framework to do the heavy lifting.  MBMP 
interfaces directly with GStreamer so there is no need to have the
QT Multi-Media packages.  

Becasue we use GStreamer most any media format can be played provided the GStreamer
plugins are installed on your system.  Additionally audio CD's and DVD's can be 
played, and for DVD's interaction with onscreen menus is implemented with either mouse
or keyboard control. There is a basic playlist available and it is also possible
to play internet media  streams.

There are three modes available to control the player.  The basic interface is
an MPlayer like one with keyboard control.  Key bindings are completely cusomizable
for each user.  There is also a GUI built in (default keybind 'g' to raise it).
Third mode is an extensive popup menu system similar to ROX-Filer, with some menus
tearoff enabled.  Activate the menus by a right mouse click anywhere in the player window.

As of this writing (June 2015) the interface is still in development, with 
development effort aimed at improving the playlist, saving and restoring
program settings and state, and eventually downloading metadata from online
sources.
