echo "CURDIR"
echo $(pwd)

echo "TELAF_ROOT"
echo $TELAF_ROOT

cp "$TELAF_ROOT/apps/ivss/common/CMakeLists.txt" ./

rm -rf build
if [ ! -d build ];then
    mkdir build
else
    echo build exist
fi

cd build
cmake -DPROJECT_NAME=$1 ..
make
