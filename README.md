Leanify [![Build Status](https://img.shields.io/travis/JayXon/Leanify.svg?style=flat)](https://travis-ci.org/JayXon/Leanify) [![Latest Release](https://img.shields.io/github/release/JayXon/Leanify.svg?style=flat)](https://github.com/JayXon/Leanify/releases)
=======

Leanify is a lightweight lossless file minifier/optimizer. It removes unnecessary data (debug information, comments, metadata, etc.) and recompress the file to reduce file size. It will not reduce image quality at all.


##Features

* Support recursive minifying. (e.g. a [PNG] inside an [APK] inside a [ZIP])
* Support a wide variety of [file formats](#file-formats).
* Lightweight, one file, under 1MB, no external dependency.
* Everything is done in memory, no temporary files.
* Cross-Platform, support Windows, Mac, Linux.
* Support traverse directory recursively.
* Identify file format by its data instead of name.


##Disclaimer

I'm not respossible for any consequence of using Leanify.

**PLEASE BACKUP THE FILE BEFORE USING LEANIFY!**


##File Formats


####APK file (.apk)

It is based on [ZIP].
  
Note that modifying files inside `APK` will break digital signature.
To install it, you'll have to sign it again.

If you don't want to modify any files inside `APK`, use `-d 1` option.


####Comic book archive (.cbt, .cbz)

`cbt` is based on [tar]. `cbz` is based on [ZIP].


####Microsoft Office document 2007-2013 (.docx, .xlsx, .pptx)

It is based on [XML] and [ZIP].

Office document 1997-2003 (.doc, .xls, .ppt) is not supported.


####EPUB file (.epub)

It is based on [ZIP].


####FictionBook (.fb2, .fb2.zip)

It is based on [XML].

Leanify embedded images.


####GFT file (.gft)

It's an image container format found in Tencent QQ.

Leanify the image inside.


####gzip file (.gz, .tgz)

Leanify file inside and recompress deflate stream.
  
Remove all optional section: `FEXTRA`, `FNAME`, `FCOMMENT`, `FHCRC`.


####Icon file (.ico)

Leanify [PNG] inside, if any.


####Java archive (.jar)

It is based on [ZIP].


####JPEG image (.jpeg, .jpg, .jpe, .jif, .jfif, .jfi, .thm)

Remove all application markers (e.g. `Exif`) and comments.

Optimize with `mozjpeg`.


####Lua object file (.lua, .luac)

Remove all debugging information:

* Source name
* Line defined and last line defined
* Source line position list
* Local list
* Upvalue list


####OpenDocument (.odt, .ods, .odp, .odb, .odg, .odf)

It is based on [XML] and [ZIP].


####PE file (.exe, .dll, .sys, .ocx, .scr, .cpl)

Leanify embedded resource.

Remove `Relocation Table` in executable file.

Remove undocumented `Rich Header`.

Overlap `PE Header` and `DOS Header`.


####PNG image (.png, .apng)

Remove all ancillary chunks except for:
  
* `tRNS`: transparent information
* `fdAT`, `fcTL`, `acTL`: These chunks are used by `APNG`
* `npTc`: Android 9Patch images (*.9.png)

Optimize with `ZopfliPNG`.


####RDB archive (.rdb)

It is an archive format found in Tencent QQ.

Leanify all files inside.


####Flash file (.swf)

Leanify embedded images.

Recompress it with `LZMA`.
  
Remove Metadata Tag.


####SVG image (.svg, .svgz)
 
It is based on [XML].
  
Remove metadata.

Remove empty attributes.

####tar archive (.tar)

Leanify all files inside.


####XML document (.xml, .xsl, .xslt)

Remove all comments, unnecessary spaces, tabs, line breaks.


####XPInstall (.xpi)

It is based on [ZIP].

Note that modifying files inside `xpi` will break digital signature if exist.
To install it, you'll have to either delete the META-INF folder inside `xpi` or sign it again.


####XPS document (.xps, .oxps)

It is based on [XML] and [ZIP].


####ZIP archive (.zip)

Leanify all files inside and recompress deflate stream.

Remove extra field in `Local file header`.

Remove `Data descriptor structure`, write those information to `Local file header`.

Remove extra field and file comment in `Central directory file header`.

Remove comment in `End of central directory record`.



##To do list


####BMP image (.bmp, .dib)


####Microsoft Compound File Binary


####PDF document (.pdf)


##Downloads

[Stable Releases](https://github.com/JayXon/Leanify/releases/)

[Linux Nightly Build](https://drone.io/github.com/JayXon/Leanify/files)



##Usage

```
Usage: Leanify [options] path1 path2 path3 ...
  -i <iteration>  More iterations means slower but better result. Default: 15.
  -d <max depth>  Maximum recursive depth.
  -f              Fast mode, no recompression.
  -q              Quiet mode, no output.
  -v              Verbose output.
```


##Compiling

####Windows

* Visual Studio

  Use Leanify.vcxproj

* gcc

  run build_gcc.bat


####Mac, Linux

```
make
```



##License

Leanify is released under the [MIT License](LICENSE).


[APK]: #apk-file-apk
[PNG]: #png-image-png-apng
[tar]: #tar-archive-tar
[XML]: #xml-document-xml-xsl-xslt
[ZIP]: #zip-archive-zip
