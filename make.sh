echo "Building DEBUG version."
echo "-----------------------"
make --no-print-directory -C build/debug -j 4

echo
echo "Building RELEASE version."
echo "-----------------------"
make --no-print-directory -C build/release -j 4

mkdir -p bin
echo
echo "Copying executables to bin directory."
echo "-------------------------------------"
echo "-> bin/myosotis_debug"
cp build/debug/src/myosotis bin/myosotis_debug
echo "-> bin/myosotis"
cp build/release/src/myosotis bin/myosotis
echo
echo "Build completed."
