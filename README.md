Leanify [![Build Status](https://travis-ci.org/JayXon/Leanify.svg)](https://travis-ci.org/JayXon/Leanify)
=======

Leanify is a lightweight lossless file minifier. It removes unnecessary data (debug information, comments, metadata, etc.) and recompress deflate stream to reduce file size. It will not reduce image quality at all.


Features
========

* Support recursive minifying. (e.g. a PNG inside an APK inside a ZIP)
* Everything is done in memory, no temporary files.
* Support traverse directory recursively.
* Identify file format by its data instead of name.


Disclaimer
==========

I'm not respossible for any consequence of using Leanify.

**PLEASE BACKUP THE FILE BEFORE USING LEANIFY!**


Downloads
=========

https://github.com/JayXon/Leanify/releases/


File Formats
============

###Supported:

* **Flash file** (.swf)

  Leanify embedded images.

  Recompress it with LZMA.
  
  Remove Metadata Tag.

* **GFT file** (.gft, image container format found in Tencent QQ)

  Leanify the image inside.
  
* **gzip file** (.gz)

  Leanify file inside and recompress deflate stream.
  
  Remove all optional section: FEXTRA, FNAME, FCOMMENT, FHCRC.
  
* **Icon file** (.ico) 

  Leanify PNG inside.

* **JPEG image** (.jpeg, .jpg)

  Remove all Application Marker (e.g. Exif) and comments.

  Optimize with mozjpeg.

* **Lua object file** (.lua, .luac)

  Remove all debugging information:
  
  * Source name
  * Line defined and last line defined
  * Source line position list
  * Local list
  * Upvalue list


* **PNG image** (.png, .apng)

  Remove all ancillary chunks except for:
  
  * tRNS: transparent information
  * fdAT, fcTL, acTL: These chunks are used by APNG
  * npTc: Android 9Patch images (*.9.png)

  Optimize with ZopfliPNG.

* **RDB archive** (.rdb, archive format found in Tencent QQ)

  Leanify all files inside.
  
* **XML document** (.xml, .xsl, .xslt)

  Remove all comments, unnecessary spaces, tabs, enters.
  
* **ZIP archive** (.apk, .cbz, .docx, .epub, .jar, .odt, .ods, .odp, .pptx, .xlsx, .xpi, .zip)

  Leanify all files inside and recompress deflate stream.
  
  Remove extra field in Local file header.
  
  Remove data descriptor structure, write these information to Local file header.
  
  Remove extra field and file comment in Central directory file header.
  
  Remove comment in End of central directory record.
  

###TODO:

* **BMP image** (.bmp)

* **PE file** (.exe, .dll)


Usage
=====

```
Usage: Leanify [options] path1 path2 path3 ...
  -i iteration  More iterations means slower but better result. Default: 15.
  -f            Fast mode, no recompression.
  -q            Quiet mode, no output.
  -v            Verbose output.
```


