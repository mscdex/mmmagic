
Description
===========

An async libmagic binding for [node.js](http://nodejs.org/) for detecting file content types.


Requirements
============

* [node.js](http://nodejs.org/) -- v0.4.0 or newer
* zlib


Install
============

npm install mmmagic


Examples
========

* Get general description of a file:
```javascript
  var Magic = require('mmmagic').Magic;

  var magic = new Magic();
  magic.detect(__dirname + '/node_modules/mmmagic/wscript', function(err, result) {
      if (err) throw err;
      console.log(result);
      // output: Python script, ASCII text executable
  });
```
* Get mime type for a file:
```javascript
  var mmm = require('mmmagic'),
        Magic = mmm.Magic;

  var magic = new Magic(mmm.MAGIC_MIME_TYPE);
  magic.detect(__dirname + '/node_modules/mmmagic/wscript', function(err, result) {
      if (err) throw err;
      console.log(result);
      // output: text/x-python
  });
```
* Get mime type and mime encoding for a file:
```javascript
  var mmm = require('mmmagic'),
        Magic = mmm.Magic;

  var magic = new Magic(mmm.MAGIC_MIME_TYPE | mmm.MAGIC_MIME_ENCODING);
  // the above flags can also be shortened down to just: mmm.MAGIC_MIME
  magic.detect(__dirname + '/node_modules/mmmagic/wscript', function(err, result) {
       if (err) throw err;
       console.log(result);
       // output: text/x-python; charset=us-ascii
  });


API
===

Magic methods
-------------

* **(constructor)**([<_String_>magicPath][, <_Integer_>flags]) - Creates and returns a new Magic instance. magicPath is an optional path string that points to a particular magic file to use (order of magic file searching: magicPath -> `MAGIC` env var -> various file system paths (see `man file`)). flags is a bitmask with the following valid values (available as constants on require('mmmagic')):

    * **MAGIC\_NONE** - No flags set
    * **MAGIC\_DEBUG** - Turn on debugging
    * **MAGIC\_SYMLINK** - Follow symlinks **(default)**
    * **MAGIC\_COMPRESS** - Check inside compressed files
    * **MAGIC\_DEVICES** - Look at the contents of devices
    * **MAGIC\_MIME_TYPE** - Return the MIME type
    * **MAGIC\_CONTINUE** - Return all matches
    * **MAGIC\_CHECK** - Print warnings to stderr
    * **MAGIC\_PRESERVE\_ATIME** - Restore access time on exit
    * **MAGIC\_RAW** - Don't translate unprintable chars
    * **MAGIC\_ERROR** - Handle ENOENT etc as real errors
    * **MAGIC\_MIME\_ENCODING** - Return the MIME encoding
    * **MAGIC\_MIME** - (**MAGIC\_MIME\_TYPE** | **MAGIC\_MIME\_ENCODING**)
    * **MAGIC\_APPLE** - Return the Apple creator and type
    * **MAGIC\_NO\_CHECK\_COMPRESS** - Don't check for compressed files
    * **MAGIC\_NO\_CHECK\_TAR** - Don't check for tar files
    * **MAGIC\_NO\_CHECK\_SOFT** - Don't check magic entries
    * **MAGIC\_NO\_CHECK\_APPTYPE** - Don't check application type
    * **MAGIC\_NO\_CHECK\_ELF** - Don't check for elf details
    * **MAGIC\_NO\_CHECK\_TEXT** - Don't check for text files
    * **MAGIC\_NO\_CHECK\_CDF** - Don't check for cdf files
    * **MAGIC\_NO\_CHECK\_TOKENS** - Don't check tokens
    * **MAGIC\_NO\_CHECK\_ENCODING** - Don't check text encodings

* **detect**(<_String_>path, <_Function_>callback) - _(void)_ - Inspects the file pointed at by path. The callback receives two arguments: an <_Error_> object in case of error (null otherwise), and a <_String_> containing the result of the inspection.
