
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

## Blocks overview

- `http_transfer_sink` - block provides HTTP (POST) transmission capabilities to the destination server
- `http_transfer_source` - block gets data using HTTP (GET) from server
- `gs_doppler_correction` - automatic Doppler correction using GPREDICT or GSJD 
- `websocket_transfer_sink` - websocket transfer capabilities (obsolete)

![Source_sink](https://github.com/pavelfpl/gr-gsSDR/blob/master/http_sink_source.png)
![Doppler](https://github.com/pavelfpl/gr-gsSDR/blob/master/doppler_correction.png)

## Block parameters

- `ServerName` - server name - use IP address or domain name
- `ServerPort` - server port e.g. 8080
- `ServerTarget` - server target e.g. /gs/tm
- `ServerId` - identification of ground station
- `SpacecraftId` - identification of tracked satellite
- `Source Type` - Doppler correction source - GPREDICT or GSJD (internal automatic system) - only for `gs_doppler_correction`
- `Base Frequency` - initial center frequency for Doppler correction (PMT output = base_freq - predict_freq) - only for `gs_doppler_correction` 
- `UserName` - user name (for future use now)
- `UserPass` - user password (for future use now)

## Screenshots

>`http_transfer_sink` with `gr-satellites` blocks: https://github.com/daniestevez/gr-satellites/

![Example Source](https://github.com/pavelfpl/gr-gsSDR/blob/master/examples/http_transfer_sink_example.png)
