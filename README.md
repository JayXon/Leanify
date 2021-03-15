Leanify [![GitHub Actions CI](https://github.com/JayXon/Leanify/actions/workflows/ci.yml/badge.svg)](https://github.com/JayXon/Leanify/actions) [![Windows Build status](https://ci.appveyor.com/api/projects/status/2p9i3ru8apwq2uic?svg=true)](https://ci.appveyor.com/project/JayXon/leanify) [![Download](https://img.shields.io/github/downloads/JayXon/Leanify/total.svg)](https://github.com/JayXon/Leanify/releases) [![Latest Release](https://img.shields.io/github/release/JayXon/Leanify.svg)](https://github.com/JayXon/Leanify/releases/latest) [![GitHub license](https://img.shields.io/github/license/JayXon/Leanify.svg)](LICENSE)
=======

Leanify is a lightweight lossless file minifier/optimizer. It removes unnecessary data (debug information, comments, metadata, etc.) and recompress the file to reduce file size. It will not reduce image quality at all.


## Features

* Support recursive minifying. (e.g. a [PNG] inside an [APK] inside a [ZIP])
* Support a wide variety of [file formats](#file-formats).
* Lightweight, one file, under 1MB, no external dependency.
* Everything is done in memory, no temporary files.
* Cross-Platform, support Windows, Linux, Mac.
* Support traverse directory recursively.
* Ability to identify file format by its data instead of name.


## Disclaimer

I'm not respossible for any consequence of using Leanify.

**PLEASE BACKUP THE FILE BEFORE USING LEANIFY!**


## File Formats


#### APK file (.apk)

It is based on [ZIP].

Note that modifying files inside `APK` will break digital signature.
To install it, you'll have to sign it again.

If you don't want to modify any files inside `APK`, use `-d 1` option.


#### Comic book archive (.cbt, .cbz)

`cbt` is based on [tar]. `cbz` is based on [ZIP].


#### Microsoft Office document 2007-2013 (.docx, .xlsx, .pptx)

It is based on [XML] and [ZIP].

Office document 1997-2003 (.doc, .xls, .ppt) is not supported.


#### Data URI (.html .htm .js .css)

Looks for `data:image/*;base64` and leanify base64 encoded embedded image.


#### Design Web Format (.dwf, dwfx)

It is based on [ZIP].


#### EPUB file (.epub)

It is based on [ZIP].


#### FictionBook (.fb2, .fb2.zip)

It is based on [XML].

Leanify embedded images.


#### GFT file (.gft)

It's an image container format found in Tencent QQ.

Leanify the image inside.


#### gzip file (.gz, .tgz)

Leanify file inside and recompress deflate stream.

Remove all optional section: `FEXTRA`, `FNAME`, `FCOMMENT`, `FHCRC`.


#### Icon file (.ico)

Convert 256x256 BMP to [PNG].

Leanify [PNG] inside, if any.


#### Java archive (.jar)

It is based on [ZIP].


#### JPEG image (.jpeg, .jpg, .jpe, .jif, .jfif, .jfi, .thm)

Remove all application markers (e.g. `Exif` (use `--keep-exif` to keep it), `ICC profile`, `XMP`) and comments.

Optimize with `mozjpeg`.


#### Lua object file (.lua, .luac)

Remove all debugging information:

* Source name
* Line defined and last line defined
* Source line position list
* Local list
* Upvalue list


#### OpenDocument (.odt, .ods, .odp, .odb, .odg, .odf)

It is based on [XML] and [ZIP].


#### OpenRaster (.ora)

It is based on OpenDocument and [PNG].


#### PE file (.exe, .dll, .ocx, .scr, .cpl)

Leanify embedded resource.

Remove `Relocation Table` in executable file.

Remove undocumented `Rich Header`.

Overlap `PE Header` and `DOS Header`.


#### PNG image (.png, .apng)

Remove all ancillary chunks except for:

* `tRNS`: transparent information
* `fdAT`, `fcTL`, `acTL`: These chunks are used by `APNG`
* `npTc`: Android 9Patch images (*.9.png)

Optimize with `ZopfliPNG`.


#### RDB archive (.rdb)

It is an archive format found in Tencent QQ.

Leanify all files inside.


#### Flash file (.swf)

Leanify embedded images.

Recompress it with `LZMA`.

Remove Metadata Tag.


#### SVG image (.svg, .svgz)

It is based on [XML].

Remove metadata.

Shrink spaces in attributes.

Remove empty attributes.

Remove empty text element and container element.

#### tar archive (.tar)

Leanify all files inside.


#### XML document (.xml, .xsl, .xslt)

Remove all comments, unnecessary spaces, tabs, line breaks.


#### XPInstall (.xpi)

It is based on [ZIP].

Note that modifying files inside `xpi` will break digital signature.
To install it, you'll have to sign it again.


#### XPS document (.xps, .oxps)

It is based on [XML] and [ZIP].


#### ZIP archive (.zip)

Leanify all files inside and recompress deflate stream using [Zopfli](https://github.com/google/zopfli).

Use `STORE` method if `DEFLATE` makes file larger.

Remove extra field in `Local file header`.

Remove `Data descriptor structure`, write those information to `Local file header`.

Remove extra field and file comment in `Central directory file header`.

Remove comment in `End of central directory record`.



## Downloads

[Stable Releases](https://github.com/JayXon/Leanify/releases/)

[Windows Nightly Build](https://ci.appveyor.com/project/JayXon/leanify)



## Usage

```
Usage: leanify [options] paths
  -i, --iteration <iteration>   More iterations produce better result, but
                                  use more time, default is 15.
  -d, --max_depth <max depth>   Maximum recursive depth, unlimited by default.
                                  Set to 1 will disable recursive minifying.
  -f, --fastmode                Fast mode, no recompression.
  -q, --quiet                   No output to stdout.
  -v, --verbose                 Verbose output.
  -p, --parallel                Distribute all tasks to all CPUs.
  --keep-exif                   Do not remove Exif.
```


## Compiling

#### Windows

* Visual Studio 2015+

  Use Leanify.vcxproj

* gcc 5+

  `build_gcc.bat` or `mingw32-make`


#### Linux, Mac

  gcc 5+ or clang 3.6+
```
make
```



[APK]: #apk-file-apk
[PNG]: #png-image-png-apng
[tar]: #tar-archive-tar
[XML]: #xml-document-xml-xsl-xslt
[ZIP]: #zip-archive-zip
