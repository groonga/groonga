'---------------------------------------------------------------
'  mkmfwin32.vbs
'---------------------------------------------------------------
option explicit
dim shell, strarch
set shell = createobject("wscript.shell")
strarch = shell.expandenvironmentstrings("%PROCESSOR_ARCHITECTURE%")
msgbox strarch


dim PLUGINS_DIR, PLUGINS_DIR2, INSTALL_DIR, LOG_PATH, MECAB_LIB, CONFIG_PATH, RELATIVE_DOCUMENT_ROOT, RELATIVE_PLUGINS_DIR
INSTALL_DIR = "c:\groonga"
PLUGINS_DIR = "c:\\groonga\\plugins"
PLUGINS_DIR2 = "c:\groonga\plugins"
LOG_PATH = "c:\\groonga\\log\\groonga.log"
MECAB_LIB = "c:\program files\mecab\sdk\libmecab.lib"
CONFIG_PATH ="c:\\groonga\\etc\\config"
RELATIVE_DOCUMENT_ROOT ="share/groonga/html/admin"
RELATIVE_PLUGINS_DIR = "plugins"

'control warning
dim no_warning
no_warning = "/wd4819 /wd4127 /wd4706 /wd4100 /wd4057 /wd4244 /wd4204 /wd4389 /wd4146 /wd4305 /wd4018 /wd4047 /wd4013 /wd4245 /wd4701"


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
objs = array("com.obj", "ctx.obj", "db.obj", "hash.obj", "ii.obj", "io.obj", "nfkc.obj", "pat.obj", "ql.obj", "query.obj", "scm.obj", "snip.obj", "store.obj", "str.obj", "token.obj", "proc.obj", "plugin.obj", "util.obj", "expr.obj",  "geo.obj", "output.obj")

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
    ts.write "CFLAGS = /nologo /Od /W4 /MD /Zi -I../include " + no_warning + vbLf
  else
    ts.write "CFLAGS = /nologo /Ox /W4 /MD /Zi -I../include " + no_warning + vbLf
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
  if use_mecab = 1 then
    ts.write "        -DWITH_MECAB \" + vbLf
  end if
  ts.write "        -DDLL_EXPORT \" + vbLf
  ts.write "        -DNO_LZO \" + vbLf
  ts.write "        -DNO_ZLIB \" + vbLf
  ts.write "        -DUSE_SELECT \" + vbLf
  ts.write "        -DGRN_DEFAULT_ENCODING=""\""utf-8\"""" \" + vbLf
  ts.write "        -DGRN_PLUGIN_SUFFIX=""\"".dll\"""" \" + vbLf
  ts.write "        -DGRN_DLL_FILENAME=""L\""libgroonga.dll\"""" \" + vbLf
  ts.write "        -DGRN_PLUGINS_DIR=""\"""
  ts.write PLUGINS_DIR
  ts.write "\"""" \"  + vbLf

  ts.write "        -DGRN_RELATIVE_PLUGINS_DIR=""\"""
  ts.write RELATIVE_PLUGINS_DIR
  ts.write "\"""" \"  + vbLf

  ts.write "        -DGRN_DEFAULT_MATCH_ESCALATION_THRESHOLD=""0"" \" + vbLf

  ts.write "        -DGRN_LOG_PATH=""\"""
  ts.write LOG_PATH
  ts.write "\"""" \"  + vbLf

  ts.write "        -DGRN_CONFIG_PATH=""\"""
  ts.write CONFIG_PATH
  ts.write "\"""" \"  + vbLf

  ts.write "        -DGRN_DEFAULT_RELATIVE_DOCUMENT_ROOT=""\"""
  ts.write RELATIVE_DOCUMENT_ROOT
  ts.write "\"""" \"  + vbLf

  ts.write "        -DPACKAGE=""\"""
  ts.write package
  ts.write "\"""" \" + vbLf

  ts.write "        -DPACKAGE_VERSION=""\"""
  ts.write version
  ts.write "\"""" \" + vbLf

  if make_lib = 1 then
  ts.write "        -DGRN_VERSION=""\"""
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
ts.write "        copy *.exe "
ts.write INSTALL_DIR + vbLf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp  *.i" + vbLf

ts.close
msgbox "src/Makefile.msvc updated"

'Makefile for plugins\suggest
set ts = fs.opentextfile("plugins\suggest\Makefile.msvc", 2, True)
ts.write "DEL = del" + vbLf
ts.write "CC = cl.exe" + vbLf
ts.write "LINK=link.exe" + vbLf
ts.write vbLf
if use_debug = 1 then
  ts.write "CFLAGS = /nologo /Od /W4 /MT /Zi -DWIN32 -I../../include -I../../lib/ " + no_warning + vbLf
else
  ts.write "CFLAGS = /nologo /Ox /W4 /MT /Zi -DWIN32 -I../../include -I../../lib/ " + no_wwarning+ vbLf
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
ts.write PLUGINS_DIR2
ts.write "\suggest "
ts.write "md "
ts.write PLUGINS_DIR2
ts.write "\suggest" + vblf

ts.write "        copy suggest.dll "
ts.write PLUGINS_DIR2
ts.write "\suggest" + vblf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp  *.i" + vbLf

ts.close
msgbox "plugins/suggest/Makefile.msvc updated"

'Makefile for plugins\tokenizers
if use_mecab = 0 then
  wscript.quit
end if

set ts = fs.opentextfile("plugins\tokenizers\Makefile.msvc", 2, True)
ts.write "DEL = del" + vbLf
ts.write "CC = cl.exe" + vbLf
ts.write "LINK=link.exe" + vbLf
ts.write vbLf
if use_debug = 1 then
  ts.write "CFLAGS = /nologo /Od /W3 /MT /Zi -DWIN32 -I../../include -I../../lib/ " + no_warning + vbLf
else
  ts.write "CFLAGS = /nologo /Ox /W3 /MT /Zi -DWIN32 -I../../include -I../../lib/ " + no_warning + vbLf
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
ts.write PLUGINS_DIR2
ts.write "\tokenizer "
ts.write "md "
ts.write PLUGINS_DIR2
ts.write "\tokenizer" + vblf

ts.write "        copy mecab.dll "
ts.write PLUGINS_DIR2
ts.write "\tokenizer" + vblf

ts.write "clean:" + vbLf
ts.write "        $(DEL) *.obj *.dll *.pdb *.exp  *.i" + vbLf

ts.close
msgbox "plugins/tokenizers/Makefile.msvc updated"
