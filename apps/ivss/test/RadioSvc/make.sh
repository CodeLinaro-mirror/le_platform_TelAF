echo "CURDIR"
echo $(pwd)

# CommonAPI tools, change to commands on your own environment
capicxx-core-gen -sk ../../fidl/RadioSvc/RadioSvc.fidl
capicxx-core-gen -sk ../../fidl/RadioSvc/RadioSvc.fdepl
capicxx-someip-gen ../../fidl/RadioSvc/RadioSvc.fdepl

rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake -DCMAKE_INSTALL_PREFIX=../install-soa/ ..
make
make install
