AC_INIT([configure.ac])

m4_include([OpenSBE.ac])

OSBE_ENABLE_FEATURE([CmdDep])
OSBE_ENABLE_FEATURE([CXX/CXX])
OSBE_ENABLE_FEATURE([CXX/Ar])
OSBE_ENABLE_FEATURE([CXX/So])
OSBE_ENABLE_FEATURE([CXX/Ex])
OSBE_ENABLE_FEATURE([CXX/Inl])

OSBE_ENABLE_FEATURE([CXX/Def])
OSBE_ENABLE_FEATURE([CXX/AutoDep])

osbe_ask_for_ne_string "Please enter gears root path" \
  "gears root path" \
  gears_root \
  "gears_root"

osbe_ask_for_ne_string "Please enter gears include path(s)" \
  "gears include path(s)" \
  gears_include \
  "gears_include"

osbe_ask_for_ne_string "Please enter gears library path(s)" \
  "gears library path(s)" \
  gears_lib \
  "gears_lib"

osbe_ask_for_ne_string "Please enter gears bin path(s)" \
  "gears bin path(s)" \
  gears_bin \
  "gears_bin"

osbe_ask_for_ne_string "Please enter gears rules path(s)" \
  "gears rules path(s)" \
  gears_rules \
  "gears_rules"

gears_def_initialize()
{
  root="$gears_root"
  include="$gears_include"
  lib="$gears_lib"
}

gears_def_initializer=gears_def_initialize

gears_rules="$gears_root/$gears_rules"
gears_bin_path="$gears_root/$gears_bin"
gears_lib_path="$gears_root/$gears_lib"
gears_include_path="$gears_root/$gears_include"

AC_SUBST(gears_bin_path)
AC_SUBST(gears_lib_path)
AC_SUBST(gears_include_path)

AC_CONFIG_FILES([rules/Plain.post.rules])

m4_include([dir.ac])

OSBE_OUTPUT
