#curl http://mirrordirector.raspbian.org/raspbian.public.key  | apt-key add -
#curl http://archive.raspbian.org/raspbian.public.key  | apt-key add -
sudo apt -q update

sudo apt install devscripts equivs wget git lsb-release
sudo mk-build-deps -ir /ci_source/build-deps/control
sudo apt-get -q --allow-unauthenticated install -f

# Temporary fix until 3.19 is available as a pypi package
# 3.19 is needed: https://gitlab.kitware.com/cmake/cmake/-/issues/20568
url='https://dl.cloudsmith.io/public/alec-leamas/opencpn-plugins-stable/deb/debian'
wget $url/pool/bullseye/main/c/cm/cmake-data_3.20.5-0.1/cmake-data_3.20.5-0.1_all.deb
wget $url/pool/bullseye/main/c/cm/cmake_3.20.5-0.1/cmake_3.20.5-0.1_armhf.deb
sudo apt install ./cmake_3.*-0.1_armhf.deb ./cmake-data_3.*-0.1_all.deb

cd /ci_source
rm -rf build-bionic-armhf; mkdir build-bionic-armhf; cd build-bionic-armhf
cmake -DCMAKE_BUILD_TYPE=debug ..
make -j $(nproc) VERBOSE=1 tarball
ldd  app/*/lib/opencpn/*.so
#sudo chown --reference=.. .
#EOF

