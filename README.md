# mbmp

NOTE: At the moment (December 2018) this program is for personal use only.  I had originally intended to release this under an MIT license.  I am not entirely sure that can be done between my stuff, the QT stuff, and the GStreamer stuff.  First two I think are okay because QT is LGPL and I only link to those libraries.  The third I am not sure on and quite honestly I do not intend to spend even one minute of my life researching this.  I won't (ever again) release anything under GPL so this program will probably never see the light of day.  If anyone wishes to build and use for personal use you should be fine, and I also think that anyone who wants to fork it and release under GPL would be fine, but I'm not a lawyer.  

A QT5 and GStreamer 1.0 series Media Player

MBMP is a fairly conventional media player using QT5 for the user interface and the GStreamer 1.0 series multi-media framework to do the heavy lifting.  MBMP interfaces directly with GStreamer so there is no need to have the QT Multi-Media packages.  

Becasue we use GStreamer most any media format can be played provided the GStreamer plugins are installed on your system. Additionally audio CD's and DVD's can be played, and for DVD's interaction with onscreen menus is implemented with either mouse or keyboard control. There is a basic playlist available and it is also possible to play media from an internet URL.

There are three modes available to control the player.  The basic interface is an MPlayer like one with keyboard control. Key bindings are completely cusomizable for each user.  There is also a GUI built in (default keybind 'g' to raise it). The third mode is an extensive popup menu system similar to ROX-Filer, with some menus tearoff enabled.  Activate the menus by a right mouse click anywhere in the player window.

If you have a notification daemon running it is possible to send playlist track changes to it.  This is controlled from the settings dialog.

A DBUS based interprocess communication (IPC) interface has also been implemented.  Documentation is in the wiki.

Update (September 2015)
The interface is essentially complete.  There are still a few advanced items to implement (color balance, av sync, etc), but all the main parts are there and working.

Update (January 2016) Playlist has been heavily updated.  Future development effort will be aimed at improving the playlist or creating a separate media manager. 
