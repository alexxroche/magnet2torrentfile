Magnet2torrentFile
==================

A small bridge program to convert magnet links to .torrent files
-----------------------------------------------------------------

Usage
-----
Download https://github.com/alexxroche/magnet2torrentfile/archive/0.3.zip
and extract (I used 7zip). Its 6 files in their own directory.

Edit (notepad++) m2tf.cfg to match your needs (see User Configuration)

Move the magnet2torrentfile directory where you can execute it,
(e.g. C:\Program Files\)

Optional: Create %APPDATA%\magnet2torrentfile\ and put your m2tf.cfg there.
(I really need to get started on an NSIS installer!)

Direct your browser to use magnet2torrentfile.exe for the magnet protocol.

User Configuration
------------------
By default it will look for %APPDATA%\magnet2torrentfile\m2tf.cfg
(On Win 8.1 %APPDATA% is %USERPROFILE%\AppData\Roaming\
and if your system is in English %USERPROFILE% could be C:\Users\%USERNAME%\).
This should be in .ini format (example config file is provided).
 magnet2torrentfile will fall back to looking for the config file in the
 same directory from which it was run. (i.e. pwd/%CD%) but ONLY if the
default %APPDATA%\magnet2torrentfile directory is missing or inaccessible. 

The settings in the example m2tf.cfg are hard-coded as fall-back settings 
within the binary, (so you only need the settings that you want changed).

If you are using this to upload the magnet to Transmission,
 you SHOULD at least verify the nas_ip and nas_download_dir

If tf_enabled = 1 and m2tf_site1_enabled = 1 (write the downloaded .torrent file)
AND nas_enabled = 1 then we will try to do BOTH. The local magnet upload is NOT
(at this time) a fall back, but a function that WILL be tried AFTER the attempt
to download the .torrent, (if that is enabled).

If tf_enabled = 0 and nas_enabled = 1 then no remote traffic will be generated, 
and no Internet access will be required.

Firewalls
---------
If you use tinywall (or any firewall) you will have to let it know that this
can make HTTPS requests, (if you are just using the transmission function
you will still have to check the LAN traffic is ok.)
 
Updates
-------
This program does not check for updates. It lets you do that when YOU chose, (by
checking the site of origin.) 

Hacking the Registry
--------------------
If that sounds scary you have the correct reaction. It is easy! And its easy to get wrong.
Some browsers do not maintain their own protocol preference and fall back to the system default.
You can set/edit that, (if you know what you are doing) using 

regedit:HKEY_CLASSES_ROOT\magnet\shell\open\command

Under-the-hood
--------------

At the moment the program expects to be fed a magnet.
If Torrent File is enabled (tf_enabled=1) it uses libcurl to talk to 
https://torcache.net/ to download the .torrent file.

If uploading the magnet directly to Transmission is enabled (nas_enabled=1)
then we also use libcurl. 

**libcurl** (http://curl.haxx.se/libcurl/) was downloaded from 
http://curl.haxx.se/download.html and we are using the precompiled
binaries (libcurl.dll libidn-11.dll zlib1.dll) found within
http://curl.haxx.se/gknw.net/7.40.0/dist-w32/renamed-curl-7.40.0-devel-mingw32.zip

https://github.com/compuphase/minIni is used to parse the config file

TODO
----

	* NSIS installer: uninstaller; registry entry (optional); %APPDATA%\magnet2torrentfile\m2tf.cfg; 
	* GUI to view/edit the m2tf.cfg (This should probably be the default if the program is run without arguments.)
	* get main.rc to add details to the binary and a nice ICO, (do we really want to bloat this with an icon?)
	* daemon.m2tf where it can maintain a connection the DHT swarm and create .torrent files from links that it is fed and ones that it finds
	* Linux version
    * -v (output version) and -h (say something helpful)
	* batch file that downloads m2tf,libcurl,minIni source and compiles all of them into one tidy .exe ?
	* check IPv6 works
	* use the system certificates to check remote https certificates, (right now libcurl bypasses cert verification).
	
daemon
-----------
I come from a unix world, so I like my programs small, fast and doing one job, (and doing it well hopefully.)
m2tfd should be a separate program.
	
Licence
-------
Unless stated otherwise: Copyright 2015 Alexx Roche, Released under the MIT Version 1.0 Licence
	
Source code
-----------
the ./src dir has all of the code that I used, (and some that I didn't) to compile this using
Code::Blocks ver 13.12 rev 9501 on Windows 8.1 with
GNU GCC Compiler 

Running project pre-build steps
gcc  -DINI_NOBROWSE=1 -o obj/minIni.o -c lib/minini/minIni.c

-------------- Build: Release in magnet2torrentfile (compiler: GNU GCC Compiler)---------------

mingw32-g++.exe  -o bin\Release\magnet2torrentfile.exe obj\Release\main.o  -s  -lgdi32 -luser32 -lkernel32 -lcomctl32 obj\minIni.o obj\libcurldll.a obj\libcurl.a -mwindows
Output file is bin\Release\magnet2torrentfile.exe with size 520.50 KB
Process terminated with status 0 (0 minute(s), 0 second(s))
0 error(s), 0 warning(s) (0 minute(s), 0 second(s))
 
	
If you want a simple example of using libcurl with C++ to download a file have a look at 4a75dab:src/main.cpp 
!Disclaimer! I an NOT a C++ expert, so my use of c_str in C++ code is probably reprehensible.
Thankfully you can fork this and improve it.	
	
History
-------

### Situation: 
I had a lovely QNAP NAS running transmission on my LAN. It seemed dull to have to:
 - Right click on the magnet
 - select "Copy Link Location"
 - Alt-Tab to the Transmission Web interface, (if it was open, or open it and login.)
 - click the "Add torrent"
 - click the text box
 - Ctrl+v the magnet into the box
 - Tab to the "Auto Start" (which was always off by default, and I'm yet to find a way to change that.)
 - Press space to toggle to "yes start when added!" (Nice option to have, but should be on by default.)
 - Tab + {Enter} to submit

### A solution: 
 So I wrote this C++ program so that Firefox would have something to hand-off the magnet links.
I used Code::Blocks "turning C++ from Technic to Duplo" to write this and g++ to compile.
libcurl is the wheel that I refuse to re-invent, at the core and the site, (for which this is a
glorified wrapper,) to https://torcache.net/ 
Also tested with Chromium, (but for that I had to hack my registry. Thanks to qBittorrent for the entry.)
 
### Alpha attempts "the script debacles"
 I first wrote a .bat that used a small bittorrent client to do the conversion, but Firefox
 was only interested in .exe (possible .com). So I tried to "compile .bat2.exe" but the resulting .exe
 failed to convert the links.
 
 Then I tried to (ab)use qBittorrent with the "save .torrent file" and "start paused" options.
 (At this point the imaginary perfectionist techie in my head, (called Shish) was apoplectic. He did not approve.)
 Anyway it didn't work for the second magnet that I handed it: side-note; I thought about striping 
 qBittorrent to qBitMagnet. It would be a headless version that used DHT to convert magnets to .torrent files.

Name
-----
magnet2torrentfile.exe is a hideously long name, but at the time of writing there were already 
magnet2torrent website (.com .me - no affiliation), and a piece of python on github that didn't work for me.
 With the addition of the direct-to-transmission functionality the name is less indicative of the functionality,
 (I expect that more people will use the transmission function than the .torrent function.)
Its a case of, (be careful what your dev name is; you could be stuck on those horns for a long time.)

m2t would have been nicer, but I wanted to avoid conflict, (and to make it easier to talk about/share.)