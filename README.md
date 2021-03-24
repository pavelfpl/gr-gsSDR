
# gr-gsSDR
GNURadio blocks for distributed pico **satellites Ground Station**, written in **C++** with Boost.

>For **Debian and Ubuntu** install:

`sudo apt install libboost-system-dev`  
`sudo apt install libboost-beast`  
`sudo apt install libboost`  
`sudo apt install libboost-asio`  
`sudo apt install libssl-dev`

## Building
>This module requires **Gnuradio 3.8.x** and `Boost`.
>Build is pretty standard:
```
mkdir build
cd build
cmake ..
make
sudo make install
sudo ldconfig
```
## Building local

>**Gnuradio 3.8.x** is installed to `$HOME/gr3.8` using `PyBombs`:

```
cd gr3.8
source setup_env.sh 
cd ..
cd gr-gsSDR
mkdir build 
cd build
cmake ../ -Wno-dev -DCMAKE_INSTALL_PREFIX=~/gr3.8
make install
sudo ldconfig
```
## Screenshots

>`websocket_transfer_sink` with `gr-satellites` blocks: https://github.com/daniestevez/gr-satellites/

![Example Source](https://github.com/pavelfpl/gr-gsSDR/blob/master/examples/websocket_transfer_sink_example.png)
