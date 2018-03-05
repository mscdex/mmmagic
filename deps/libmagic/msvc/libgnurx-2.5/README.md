This version of libgnurx was taken from https://github.com/nscaife/file-windows/tree/master/libgnurx-2.5.

Only one modification has been made to the code taken from the above source: regexec.c contained a ternary expression with a missing expression, which is not supported by the Visual C compiler, so the missing expression has been re-added.

Original README.md below
========================

The contents of this directory are from http://ftp.gnome.org/pub/gnome/binaries/win32/dependencies/.

I have made a few modifications to the Makefile to disable creation of gnurx.lib. The creation of this file relies on lib.exe from Visual Studio (forcing a Windows build environment), and is not needed to build libmagic or file.
