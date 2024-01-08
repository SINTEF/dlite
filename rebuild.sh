mkdir build

cd build

cmake -DCMAKE_INSTALL_PREFIX=$VIRTUAL_ENV -DPython3_FIND_VIRTUALENV=ONLY -DPYTHON_VERSION=3.8 ..

cmake --build .


if [[ "$1" == "-y" ]]; then
	
	answer="y"

else

  echo -n "Continue? (y/n): "
  read answer

fi


if [[ "$answer" == "y" || "$answer" == "Y" ]]; then


	cmake --install .

	ctest
else
	echo "Aborting..."

fi



#cd ..

#rm -r build
