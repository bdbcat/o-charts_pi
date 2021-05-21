## INSTALL: Building Plugins generic README.

Install build dependencies as described in the
[wiki](https://github.com/Rasbats/managed_plugins/wiki/Local-Build)
Then clone this repository, enter it and make
`rm -rf build; mkdir build; cd build`.

A "normal" (not flatpak) tar.gz tarball which can be imported directly
into opencpn:

    $ cmake ..
    $ make tarball

To build the flatpak tarball which also can be imported:

    $ cmake ..
    $ make flatpak


On most platforms besides flatpak: build a platform-dependent legacy
installer like a NSIS .exe on Windows, a Debian .deb package on Linux
and a .dmg image for MacOS:

    $ cmake ..
    $ make pkg

#### Building on windows (MSVC command line)
On windows, a different workflow is used:

    > cmake -T v141_xp -G "Visual Studio 15 2017" --config RelWithDebInfo  ..
    > cmake --build . --target tarball --config RelWithDebInfo

This is to build the installer tarball. Use _--target pkg_ to build the
legacy NSIS installer.

#### NMake package
  1. Open the *x86 Native Tools Command Prompt for VS 2017*, and navigate
     to the Builds directory e.g. `C:\Builds\`.
  2. Clone the source code:

          git clone https://github.com/bdbcat/oesenc_pi.git

  3. CD into the new directory `C:\Builds\oesenc_pi\` and create a new
     directory \build\ `mkdir build`
  4. CD into the \build\ directory and enter: (Mind the dots!)

            > cmake -T v141_xp ..

  5. Open the oesenc\_pi.sln in VS2017 to build for Release and make a
     package or build by cmake from the command line:

            > cmake --build .

     Build release version from the command line

            > cmake --build . --config release

     Build installer metadata/tarball from the command line:

           > cmake --build . --config release --target tarball

     Build legacy .deb package using

           > cmake --build . --config release --target pkg

     The Windows install package and the tarballs will given names like
     `oesenc_pi-X.X.xxxx-ov50-win32.exe` or `oesenc_pi-X.X.xxxx-ov50-win32.xml` 


