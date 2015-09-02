
Description
===========

An async libmagic binding for [node.js](http://nodejs.org/) for detecting content types by data inspection.

[![Build Status](https://travis-ci.org/mscdex/mmmagic.svg?branch=master)](https://travis-ci.org/mscdex/mmmagic)
[![Build status](https://ci.appveyor.com/api/projects/status/mva462lka1ap5a3t)](https://ci.appveyor.com/project/mscdex/mmmagic)


Requirements
============

* [node.js](http://nodejs.org/) -- v0.10.0 or newer


Install
=======

    npm install mmmagic


Examples
========

* Get general description of a file:
```javascript
  var Magic = require('mmmagic').Magic;

  var magic = new Magic();
  magic.detectFile('node_modules/mmmagic/build/Release/magic.node', function(err, result) {
      if (err) throw err;
      console.log(result);
      // output on Windows with 32-bit node:
      //    PE32 executable (DLL) (GUI) Intel 80386, for MS Windows
  });
```
* Get mime type for a file:
```javascript
  var mmm = require('mmmagic'),
      Magic = mmm.Magic;

  var magic = new Magic(mmm.MAGIC_MIME_TYPE);
  magic.detectFile('node_modules/mmmagic/build/Release/magic.node', function(err, result) {
      if (err) throw err;
      console.log(result);
      // output on Windows with 32-bit node:
      //    application/x-dosexec
  });
```
* Get mime type and mime encoding for a file:
```javascript
  var mmm = require('mmmagic'),
      Magic = mmm.Magic;

  var magic = new Magic(mmm.MAGIC_MIME_TYPE | mmm.MAGIC_MIME_ENCODING);
  // the above flags can also be shortened down to just: mmm.MAGIC_MIME
  magic.detectFile('node_modules/mmmagic/build/Release/magic.node', function(err, result) {
      if (err) throw err;
      console.log(result);
      // output on Windows with 32-bit node:
      //    application/x-dosexec; charset=binary
  });
```
* Get general description of the contents of a Buffer:
```javascript
  var Magic = require('mmmagic').Magic;

  var magic = new Magic(),
        buf = new Buffer('import Options\nfrom os import unlink, symlink');
  
  magic.detect(buf, function(err, result) {
      if (err) throw err;
      console.log(result);
      // output: Python script, ASCII text executable
  });
```

API
===

Magic methods
-------------

* **(constructor)**([< _String_ >magicPath][, < _Integer_ >flags]) - Creates and returns a new Magic instance. magicPath is an optional path string that points to a particular magic file to use. If magicPath is not a string and not false, the bundled magic file will be used. If magicPath is false, mmmagic will default to searching for a magic file to use (order of magic file searching: `MAGIC` env var -> various file system paths (see `man file`)). flags is a bitmask with the following valid values (available as constants on require('mmmagic')):

    * **MAGIC\_NONE** - No flags set
    * **MAGIC\_DEBUG** - Turn on debugging
    * **MAGIC\_SYMLINK** - Follow symlinks **(default for non-Windows)**
    * **MAGIC\_DEVICES** - Look at the contents of devices
    * **MAGIC\_MIME_TYPE** - Return the MIME type
    * **MAGIC\_CONTINUE** - Return all matches (returned as an array of strings)
    * **MAGIC\_CHECK** - Print warnings to stderr
    * **MAGIC\_PRESERVE\_ATIME** - Restore access time on exit
    * **MAGIC\_RAW** - Don't translate unprintable chars
    * **MAGIC\_MIME\_ENCODING** - Return the MIME encoding
    * **MAGIC\_MIME** - (**MAGIC\_MIME\_TYPE** | **MAGIC\_MIME\_ENCODING**)
    * **MAGIC\_APPLE** - Return the Apple creator and type
    * **MAGIC\_NO\_CHECK\_TAR** - Don't check for tar files
    * **MAGIC\_NO\_CHECK\_SOFT** - Don't check magic entries
    * **MAGIC\_NO\_CHECK\_APPTYPE** - Don't check application type
    * **MAGIC\_NO\_CHECK\_ELF** - Don't check for elf details
    * **MAGIC\_NO\_CHECK\_TEXT** - Don't check for text files
    * **MAGIC\_NO\_CHECK\_CDF** - Don't check for cdf files
    * **MAGIC\_NO\_CHECK\_TOKENS** - Don't check tokens
    * **MAGIC\_NO\_CHECK\_ENCODING** - Don't check text encodings

* **detectFile**(< _String_ >path, < _Function_ >callback) - _(void)_ - Inspects the file pointed at by path. The callback receives two arguments: an < _Error_ > object in case of error (null otherwise), and a < _String_ > containing the result of the inspection.

* **detect**(< _Buffer_ >data, < _Function_ >callback) - _(void)_ - Inspects the contents of data. The callback receives two arguments: an < _Error_ > object in case of error (null otherwise), and a < _String_ > containing the result of the inspection.
