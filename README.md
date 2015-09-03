# mbmp
A QT5 and GStreamer 1.0 series Media Player

MBMP is a fairly conventional media player using QT5 for the user interface and the GStreamer 1.0 series multi-media framework to do the heavy lifting.  MBMP interfaces directly with GStreamer so there is no need to have the QT Multi-Media packages.  

Becasue we use GStreamer most any media format can be played provided the GStreamer plugins are installed on your system. Additionally audio CD's and DVD's can be played, and for DVD's interaction with onscreen menus is implemented with either mouse or keyboard control. There is a basic playlist available and it is also possible to play internet media streams.

There are three modes available to control the player.  The basic interface is an MPlayer like one with keyboard control. Key bindings are completely cusomizable for each user.  There is also a GUI built in (default keybind 'g' to raise it).
Third mode is an extensive popup menu system similar to ROX-Filer, with some menus tearoff enabled.  Activate the menus by a right mouse click anywhere in the player window.

If you have a notification daemon running it is possible to send playlist track changes to it.  This is controlled from the settings dialog.

A DBUS based interprocess communication (IPC) interface has also been implemented.  Documentation is in the wiki.

Update (September 2015)
The interface is essentially complete.  There are still a few advanced items to implement (color balance, av sync, etc), but all the main parts are there and working.  Future development effort will be aimed at improving the playlist and eventually downloading metadata from online sources.
