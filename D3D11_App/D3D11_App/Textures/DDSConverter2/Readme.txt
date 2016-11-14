--------------------------------------------------------------------
DDS Converter 2.1 | Release Notes | March 18th, 2005
Copyright (c) 2004-2005 Yannick 'Bluehair' Leon. All rights reserved
--------------------------------------------------------------------
Contact: bluehair@d3files.com or bluehair@wanadoo.fr
--------------------------------------------------------------------


|---- Manifest ----|

The setup program has installed the following files:

|- Readme.txt
|- DDS Converter 2.exe
|---------------------
|- DevIL.dll			; open source image library used to load images in the previewer and
|- ILU.dll			; perform generic conversions such as PNG -> JPG, DDS -> TGA etc
|- ILUT.dll
|---------------------
|- nvdxt.exe			; nVidia's command line tool used to create DDS files
|---------------------
|- s3tc.exe			; older tool from S3 Graphics included as an alternative to nVidia's
|- vic32.dll

The latest version of nvDXT can be obtained from this link:  http://developer.nvidia.com/object/nv_texture_tools.html

As concerns the program itself, it can be downloaded from the following places:
|- http://doom3.filefront.com/file/;29052
|- http://eliteforce2.filefront.com/file/;29412


|---- Intro ----|

This program was initially designed to batchly generate DXT-compressed textures out of common image files. Since nVidia's tool is used to achieve this goal, this program mostly acts as a front-end to this tool.


|---- Changes since v2.0----|

- Various improvements on the user interface:
+ 'Options' dialog redone/recoded
+ ESC key disabled in child dialogs
+ added dragging support

- Improved DDS Support:
+ Alpha channel will now be saved if the sources have one
+ Number of mipmaps + their filtering method can be specified
+ Any command-line switches supported by nvDXT can be specified


|---- Notes ----|

- The small button beside 'Reset', '.\', makes the output folder match the input's
- The 'Convert' button will remain disabled until you select an output folder
- The 'Opacity' slider won't have any effect on Win9x systems
- The number of mipmaps reported doesn't include the image displayed in the previewer. The DirectX Texture Tool (included in the DirectX SDK) will add one to this number

- Problems and limitations:
+ nvDXT v6.60 fails to read BMP/PNG files; PSD support doesn't seem to be great either
+ S3TC only reads TGA files

To bypass these limitations, DDS Converter 2 generates a temporary TGA file using DevIL to eventually have any file types converted into DDS. This means that nvDXT and S3TC will work faster when the sources are:
++ JPG/TGA and PSD files (not recommended) for nvDXT
++ TGA files for S3TC

+ DevIL ain't a flawless library and may brutally fail to read or otherwise convert some files sometimes. DDS Converter 2 cannot predict those unreported failures and they could cause it to crash as well.

Take notice that I *rarely* have experienced such problems in practice. If it was the case, I wouldn't have released this program!

- Random thoughts if you experience trouble:
+ delete the INI file to revert to the program's default settings
+ disable the previewer; by the way, it is not recommended to preview hi-res images (1024x768 and above) as it would take some time to read them
+ disable the transparency feature by setting "$default window transparency:" to 0 in the INI file


|---- Outro ----|

This will likely be the last update to this program, I think it is quite good as it stands currently. This doesn't mean I won't think about a v3 at some point though, as I like creating MFC tools :) I'm currently working on a long-term game-related project using the Power Render 3D engine; I did and still learn a lot about maths/game design/programming/modeling etc and it's now high time for me to put all that knowledge into practice!

Thank you for your interest in this modest program :)

-Bluehair