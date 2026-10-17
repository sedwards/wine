sudo apt install llvm
apt-cache policy clang lld llvm

orangepi@orangepirv:~/source/wine$ sudo apt install clang lld llvm
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
llvm is already the newest version (1:14.0-55.3).
The following additional packages will be installed:
  clang-14 libclang-common-14-dev libclang-cpp14 libclang1-14 libgc1 libobjc-12-dev libobjc4 lld-14
Suggested packages:
  clang-14-doc wasi-libc
Recommended packages:
  llvm-14-dev
The following NEW packages will be installed:
  clang clang-14 libclang-common-14-dev libclang-cpp14 libclang1-14 libgc1 libobjc-12-dev libobjc4 lld lld-14
0 upgraded, 10 newly installed, 0 to remove and 13 not upgraded.
Need to get 20.7 MB of archives.
After this operation, 128 MB of additional disk space will be used.
Do you want to continue? [Y/n] y


make -B dlls/ntdll/actctx.o \
    CFLAGS="-g -O2 -fshort-wchar" --verbose

CFLAGS="-g -O2 -fshort-wchar" ./configure --without-mingw && make clean && make -j3


orangepi@orangepirv:~/source/wine$ sudo apt install libc6-dev-riscv64-cross
[sudo] password for orangepi: 
Reading package lists... Done
Building dependency tree... Done
Reading state information... Done
The following additional packages will be installed:
  libc6-riscv64-cross linux-libc-dev-riscv64-cross
The following NEW packages will be installed:
  libc6-dev-riscv64-cross libc6-riscv64-cross linux-libc-dev-riscv64-cross
0 upgraded, 3 newly installed, 0 to remove and 13 not upgraded.
Need to get 5,544 kB of archives.
After this operation, 7,074 kB of additional disk space will be used.
Do you want to continue? [Y/n] y
Get:1 https://snapshot.debian.org/archive/debian-ports/20221225T084846Z unstable/main riscv64 libc6-riscv64-cross all 2.36-3cross1 [1,003 kB]
Get:2 https://snapshot.debian.org/archive/debian-ports/20221225T084846Z unstable/main riscv64 linux-libc-dev-riscv64-cross all 6.0.5-1cross1 [1,772 kB]
Get:3 https://snapshot.debian.org/archive/debian-ports/20221225T084846Z unstable/main riscv64 libc6-dev-riscv64-cross all 2.36-3cross1 [2,769 kB]
Fetched 5,544 kB in 7s (758 kB/s)                                                                                                                    
Selecting previously unselected package libc6-riscv64-cross.
(Reading database ... 98838 files and directories currently installed.)
Preparing to unpack .../libc6-riscv64-cross_2.36-3cross1_all.deb ...
Unpacking libc6-riscv64-cross (2.36-3cross1) ...
Selecting previously unselected package linux-libc-dev-riscv64-cross.
Preparing to unpack .../linux-libc-dev-riscv64-cross_6.0.5-1cross1_all.deb ...
Unpacking linux-libc-dev-riscv64-cross (6.0.5-1cross1) ...
Selecting previously unselected package libc6-dev-riscv64-cross.
Preparing to unpack .../libc6-dev-riscv64-cross_2.36-3cross1_all.deb ...
Unpacking libc6-dev-riscv64-cross (2.36-3cross1) ...
Setting up linux-libc-dev-riscv64-cross (6.0.5-1cross1) ...
Setting up libc6-riscv64-cross (2.36-3cross1) ...
Setting up libc6-dev-riscv64-cross (2.36-3cross1) ...
orangepi@orangepirv:~/source/wine$ 

ln -s ../winebuild/winebuild tools/winegcc/winebuild
