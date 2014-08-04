Leanify
=======

Leanify is a lightweight lossless file minifier. It removes unnecessary information (debug information, comments, metadata, etc.) and recompress deflate stream to reduce file size.


Features
========

* Support recursive minifying. (Eg. a PNG inside an APK inside a ZIP)
* Everything is done in memory, no temporary files.
* Support traverse directory recursively.
* Identify file format by its data instead of name.

Disclaimer
==========

I'm not respossible for any consequence of using Leanify.

**PLEASE BACKUP THE FILE BEFORE USING LEANIFY!**

File Formats
=======

###Supported

* **GFT file** (.gft, image container format found in Tencent QQ)

  Leanify image inside.
  
* **gzip file** (.gz)

  Leanify file inside and recompress its deflate stream.
  
  Remove all optional section: FEXTRA, FNAME, FCOMMENT, FHCRC.
  
* **Icon file** (.ico) 

  Leanify PNG inside.
  
* **Lua object file** (.lua, .luac)

  Remove all debug information.
  
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

  Leanify files inside.
  
* **XML document** (.xml, .xsl, .xslt)

  Remove all comments, unnecessary spaces, tabs, enters.
  
* **ZIP archieve** (.apk, .crx, .docx, .jar, .odt, .ods, .odp, .pptx, .xlsx, .xpi, .zip)

  Leanify files inside.
  
  Remove extra field in Local file header.
  
  Remove data descriptor structure, write these information to Local file header.
  
  Remove extra field and file comment in Central directory file header.
  
  Remove comment in End of central directory record.
  

###WIP

* **JPEG image** (.jpeg, .jpg)

* **BMP image** (.bmp)


Usage
--------------

```
Usage: Leanify [options] path
  -i iteration  More iterations means slower but better result. Default: 15.
  -f            Fast mode, no recompression.
  -q            Quiet mode, no output.
  -v            Verbose output.
```


