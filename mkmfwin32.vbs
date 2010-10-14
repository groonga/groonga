'---------------------------------------------------------------
'  mkmfwin32.vbs
'---------------------------------------------------------------
option explicit
dim shell, strarch
set shell = createobject("wscript.shell")
strarch = shell.expandenvironmentstrings("%PROCESSOR_ARCHITECTURE%")
msgbox strarch


dim MODULES_DIR, MODULES_DIR2, INSTALL_DIR, LOG_PATH, MECAB_LIB, CONFIG_PATH,DOCUMENT_ROOT
MODULES_DIR = "c:\\groonga\\modules"
MODULES_DIR2 = "c:\groonga\modules"
INSTALL_DIR = "c:\windows\system32"
LOG_PATH = "c:\\groonga\\log\\groonga.log"
MECAB_LIB = "c:\program files\mecab\sdk\libmecab.lib"
CONFIG_PATH ="c:\\groonga\\etc\\config"
DOCUMENT_ROOT ="c:\\groonga\\data\\admin_html"
'config options
'
dim use_debug, use_64bit, use_mecab, make_lib
use_debug = 1
if strarch = "x86" then
  use_64bit = 0
else
  use_64bit = 1
end if
use_mecab = 1

'object files
dim objs
objs = array("com.obj", "ctx.obj", "db.obj", "hash.obj", "ii.obj", "io.obj", "nfkc.obj", "pat.obj", "ql.obj", "query.obj", "scm.obj", "snip.obj", "store.obj", "str.obj", "token.obj", "proc.obj", "module.obj", "util.obj", "expr.obj",  "geo.obj", "output.obj")

dim fs
set fs = wscript.createobject("scripting.filesystemobject")
dim ts
set ts = fs.opentextfile("configure.ac", 1)

'get version
dim sline, sarray, package, version, slen
do until ts.atendofstream
  sline = ts.readline
  if instr(sline, "AC_INIT(") <> 0 then
    sarray = split(sline, ",")
  end if
loop
ts.close
package = sarray(0)
version = sarray(1)
slen = len(package)
package = mid(package, 10, slen - 10)

'get_revision
dim revision, objwshshell, objexec

set objwshshell = createobject("wscript.shell")
set objexec = objwshshell.exec("git describe --abbrev=7 HEAD")
revision = objexec.stdout.readline
revision = mid(revision, 2)

sub common_header()
  ts.write "CC = cl.exe" + vbLf
  ts.write "LINK=link.exe" + vbLf
  ts.write vbLf
  if use_debug = 1 then
    ts.write "CFLAGS = /nologo /Od /W3 /MT /Zi -I../" + vbLf
  else
    ts.write "CFLAGS = /nologo /Ox /W3 /MT /Zi -I../" + vbLf
  end if

  ts.write "LDFLAGS = /nologo "
  if use_64bit = 1 then
    ts.write "/MACHINE:X64 "
  else
    ts.write "/MACHINE:X86 "
  end if
  if use_debug = 1 then
    ts.write "/debug "
  end if
  ts.write "/STACK:10240000 /DYNAMICBASE /OPT:REF /OPT:ICF /NXCOMPAT advapi32.LIB ws2_32.lib "
  if use_mecab = 1 then
    ts.write "libmecab.lib"
  end if
  ts.write vbLf

  ts.write "DEFS =  -D_CRT_SECURE_NO_DEPRECATE \" + vbLf
  ts.write "        -DWIN32 \" + vbLf
  if use_mecab = 0 then
    ts.write "        -DNO_MECAB \" + vbLf
  end if
  ts.write "        -DDLL_EXPORT \" + vbLf
  ts.write "        -DNO_LZO \" + vbLf
  ts.write "        -DNO_ZLIB \" + vbLf
  ts.write "        -DUSE_SELECT \" + vbLf
  ts.write "        -DGROONGA_DEFAULT_ENCODING=""\""utf-8\"""" \" + vbLf
  ts.write "        -DGRN_MODULE_SUFFIX=""\"".dll\"""" \" + vbLf
  ts.write "        -DMODULES_DIR=""\"""
  ts.write MODULES_DIR
  ts.write "\"""" \"  + vbLf

  ts.write "        -DGROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD=""0"" \" + vbLf

  ts.write "        -DGROONGA_LOG_PATH=""\"""
  ts.write LOG_PATH
  ts.write "\"""" \"  + vbLf

  ts.write "        -DGRN_CONFIG_PATH=""\"""
  ts.write CONFIG_PATH
  ts.write "\"""" \"  + vbLf

  ts.write "        -DDEFAULT_DOCUMENT_ROOT=""\"""
  ts.write DOCUMENT_ROOT
  ts.write "\"""" \"  + vbLf

  ts.write "        -DPACKAGE=""\"""
  ts.write package
  ts.write "\"""" \" + vbLf

  ts.write "        -DPACKAGE_VERSION=""\"""
  ts.write version
  ts.write "\"""" \" + vbLf

  if make_lib = 1 then
  ts.write "        -DGROONGA_VERSION=""\"""
  ts.write revision
  ts.write "\"""" \" + vbLf
  end if

  ts.write "        -DPACKAGE_STRING=""\"""
  ts.write version
  ts.write "\"""" " + vbLf

  ts.write "DEL = del" + vbLf
  ts.write vbLf
end sub

'Makefile for lib
set ts = fs.opentextfile("lib/Makefile.msvc", 2, True)
make_lib = 1
common_header

dim i
ts.write "OBJ = "
for each i in objs
  ts.write i + " "
next
ts.write vbLf

ts.write ".c.obj:" + vbLf
ts.write "        $(CC) $(CFLAGS) $(DEFS) -c $<" + vbLf
ts.write vbLf

ts.write "libgroonga: $(OBJ) libgroonga.obj" + vbLf
ts.write "        $(LINK) $(LDFLAGS) /out:$@.dll $(OBJ) libgroonga.obj /dll" + vbLf
ts.write vbLf

ts.write "install:" + vbLf
ts.write "        copy libgroonga.dll "
ts.write INSTALL_DIR + vbLf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp *.lib *.i" + vbLf

ts.close
msgbox "lib/Makefile.msvc updated"

'Makefile for src
set ts = fs.opentextfile("src\Makefile.msvc", 2, True)

make_lib = 0
common_header

ts.write "OBJ = "
for each i in objs
  ts.write "..\lib\" + i + " "
next
ts.write vbLf

ts.write ".c.obj:" + vbLf
ts.write "        $(CC) $(CFLAGS) $(DEFS) -c $<" + vbLf
ts.write vbLf

ts.write "all: groonga grntest" + vbLf

ts.write "groonga: $(OBJ) groonga.obj" + vbLf
ts.write "        $(LINK) $(LDFLAGS) /out:$@.exe groonga.obj ../lib/libgroonga.lib" + vbLf
ts.write vbLf

ts.write "grntest: $(OBJ) grntest.obj" + vbLf
ts.write "        $(LINK) $(LDFLAGS) /out:$@.exe grntest.obj ../lib/libgroonga.lib " + vbLf
ts.write vbLf

ts.write "install:" + vbLf
ts.write "        copy groonga.exe "
ts.write INSTALL_DIR + vbLf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp  *.i" + vbLf

ts.close
msgbox "src/Makefile.msvc updated"

'Makefile for modules\suggest
set ts = fs.opentextfile("modules\suggest\Makefile.msvc", 2, True)
ts.write "DEL = del" + vbLf
ts.write "CC = cl.exe" + vbLf
ts.write "LINK=link.exe" + vbLf
ts.write vbLf
if use_debug = 1 then
  ts.write "CFLAGS = /nologo /Od /W3 /MT /Zi -DWIN32 -I../../ -I../../lib/" + vbLf
else
  ts.write "CFLAGS = /nologo /Ox /W3 /MT /Zi -DWIN32 -I../../ -I../../lib/" + vbLf
end if

ts.write "LDFLAGS = /nologo "
if use_64bit = 1 then
  ts.write "/MACHINE:X64 "
else
  ts.write "/MACHINE:X86 "
end if
if use_debug = 1 then
  ts.write "/debug "
end if
ts.write vbLf

ts.write "DEFS =  -D_CRT_SECURE_NO_DEPRECATE \" + vbLf
ts.write "        -DDLL_EXPORT \" + vbLf
ts.write "        -DNO_LZO \" + vbLf
ts.write "        -DNO_ZLIB \" + vbLf
ts.write "        -DUSE_SELECT" + vbLf

ts.write ".c.obj:" + vbLf
ts.write "        $(CC) $(CFLAGS) $(DEFS) -c $<" + vbLf
ts.write vbLf

ts.write "suggest: $(OBJ) suggest.obj" + vbLf
ts.write "        $(LINK) $(LDFLAGS) /out:$@.dll suggest.obj ../../lib/libgroonga.lib /dll" + vbLf
ts.write vbLf

ts.write "install:" + vbLf
ts.write "        if not exist "
ts.write MODULES_DIR2
ts.write "\suggest "
ts.write "md "
ts.write MODULES_DIR2
ts.write "\suggest" + vblf

ts.write "        copy suggest.dll "
ts.write MODULES_DIR2
ts.write "\suggest" + vblf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp  *.i" + vbLf

ts.close
msgbox "modules/suggest/Makefile.msvc updated"

'Makefile for modules\tokenizers
if use_mecab = 0 then
  wscript.quit
end if

set ts = fs.opentextfile("modules\tokenizers\Makefile.msvc", 2, True)
ts.write "DEL = del" + vbLf
ts.write "CC = cl.exe" + vbLf
ts.write "LINK=link.exe" + vbLf
ts.write vbLf
if use_debug = 1 then
  ts.write "CFLAGS = /nologo /Od /W3 /MT /Zi -DWIN32 -I../../ -I../../lib/" + vbLf
else
  ts.write "CFLAGS = /nologo /Ox /W3 /MT /Zi -DWIN32 -I../../ -I../../lib/" + vbLf
end if

ts.write "LDFLAGS = /nologo "
if use_64bit = 1 then
  ts.write "/MACHINE:X64 "
else
  ts.write "/MACHINE:X86 "
end if
if use_debug = 1 then
  ts.write "/debug "
end if
ts.write vbLf

ts.write "DEFS =  -D_CRT_SECURE_NO_DEPRECATE \" + vbLf
ts.write "        -DDLL_EXPORT \" + vbLf
ts.write "        -DNO_LZO \" + vbLf
ts.write "        -DNO_ZLIB \" + vbLf
ts.write "        -DUSE_SELECT" + vbLf

ts.write ".c.obj:" + vbLf
ts.write "        $(CC) $(CFLAGS) $(DEFS) -c $<" + vbLf
ts.write vbLf

ts.write "mecab: $(OBJ) mecab.obj" + vbLf
ts.write "        $(LINK) $(LDFLAGS) /out:$@.dll mecab.obj ../../lib/libgroonga.lib """
ts.write MECAB_LIB
ts.write """ /dll" + vbLf
ts.write vbLf

ts.write "install:" + vbLf
ts.write "        if not exist "
ts.write MODULES_DIR2
ts.write "\tokenizer "
ts.write "md "
ts.write MODULES_DIR2
ts.write "\tokenizer" + vblf

ts.write "        copy mecab.dll "
ts.write MODULES_DIR2
ts.write "\tokenizer" + vblf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp  *.i" + vbLf

ts.close
msgbox "modules/tokenizers/Makefile.msvc updated"
