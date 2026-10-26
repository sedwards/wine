sudo apt install llvm strace libc6-dev-riscv64-cross gh
apt-cache policy clang lld llvm

make -B dlls/ntdll/actctx.o \
    CFLAGS="-g -O2 -fshort-wchar" --verbose

CFLAGS="-g -O2 -fshort-wchar" ./configure --without-mingw && make clean && make -j3

ln -s ../winebuild/winebuild tools/winegcc/winebuild

