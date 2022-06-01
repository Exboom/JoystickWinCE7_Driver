*in process...*

## HID Joystick Driver for Windows CE 7
### Device: Toradex Colibri T20 (https://developer-archives.toradex.com/products/colibri-t20)
### OS: Winodws Embedded Compact 7 (https://developer-archives.toradex.com/software/windows-embedded-compact/t20-t30-wec-software)
### SDK: Toradex and Windows (https://www.microsoft.com/en-us/download/details.aspx?id=38794)
### IDE: Microsoft Visual Studio 2008
To build a project, you must specify the paths to the header files (prj Properties → Configuration Properties → C/C++ → General → Additional Include Directories):
1. *\WINCE700\public\common\sdk\inc
2. *\WINCE700\public\common\oak\inc

And specify the path to the hidparse.lib library (prj Properties → Configuration Properties → Linker → Input → Additional Dependencies):
1. *\WINCE700\public\common\oak\lib\ARMV7\retail\hidparse.lib
