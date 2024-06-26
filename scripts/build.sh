#!/bin/sh
export SKNANO_BUILDDIR=$SKNANO_HOME/build

echo @@@@ Prepare to build SKNanaAnalyzer in $SKNANO_BUILDDIR
rm -rf $SKNANO_BUILDDIR $SKNANO_LIB 
mkdir $SKNANO_BUILDDIR && cd $SKNANO_BUILDDIR

export CORRECTION_CMAKE_PREFIX=`correction config --cmake`
echo @@@@ camke -DCMAKE_INSTALL_PREFIX=$SKNANO_HOME $CORRECTION_CMAKE_PREFIX $SKNANO_HOME
cmake -DCMAKE_INSTALL_PREFIX=$SKNANO_HOME $CORRECTION_CMAKE_PREFIX $SKNANO_HOME

echo @@@@ make -j6
make -j6

echo @@@@ install to $SKNANO_LIB
make install
