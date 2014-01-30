Lwimager
========

This is the Lwimager, which used to be the casarest package.
The casarest package was the remainder of the AIPS++ libraries.

The home of Lwimager is:

https://github.com/ska-sa/lwimager

Installation
============

There are debian packages available from:

https://launchpad.net/~ska-sa/+archive/main/

Alternativly you can build from source. Lwimager depends on
cmake, casacore, boost, wcslib, cfitsio and gfortran.

You can install these on Debian (or similar):

 $ apt-get install cmake libcasacore-dev libboost-dev wcslib-dev \
    libcfitsio3-dev libboost-system-dev libboost-thread-dev gfortran
    
and then compile it with:

 $ mkdir build
 $ cd build
 $ cmake .. 
 $ make 

Questions or problems?
----------------------

Just e-mail Oleg Smirnov <osmirnov@gmail.com> or open an issue on Github.
