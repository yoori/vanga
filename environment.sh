
gears_root=/home/jurij_kuznecov/Vanga.cmake/
export gears_root

vanga_root=/home/jurij_kuznecov/Vanga.cmake/
export vanga_root

PATH=$PATH:$gears_root/bin
PATH=$PATH:$gears_root/build/bin
PATH=$PATH:$vanga_root/bin
PATH=$PATH:$vanga_root/build/bin
PATH=$PATH:$vanga_root/utils/Scripts
export PATH

PERL5LIB=$PERL5LIB:$gears_root/bin
PERL5LIB=$PERL5LIB:$vanga_root/utils/Scripts/
export PERL5LIB

LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$gears_root/build/lib
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$vanga_root/lib
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$vanga_root/build/lib
export LD_LIBRARY_PATH

NLS_LANG=.AL32UTF8
export NLS_LANG

NLS_NCHAR=AL32UTF8
export NLS_NCHAR

