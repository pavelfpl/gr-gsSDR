
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

- `ServerName` - server name - use IP address or domain name (e.g. localhost)
- `ServerPort` - server port e.g. 8080
- `ServerTarget` - server target e.g. /gs/tm
- `ServerId` - identification of Ground Station
- `SpacecraftId` - identification of Tracked Satellite
- `Source Type` - Doppler correction source - GPREDICT or GSJD (internal automatic correction system) - parameter only for `gs_doppler_correction`
- `Base Frequency` - initial center frequency for Doppler correction (PMT output = base_freq - predict_freq), GSJD supports both RX and TX direction - parameter only for `gs_doppler_correction` 
- `UserName` - user name (for future use now)
- `UserPass` - user password (for future use now)

## Notes
> Add arguments to GnuRadio python generated script
```
self.port = port = '8080'
        
if len(sys.argv) >= 2:
   print('Argv[1] provided ...')
   port = sys.argv[1] 
```
> Running GnuRadio from C program (using fork)

```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

int main(int argc, char const *argv[]){

      char arg1[20];
      snprintf(arg1, sizeof(arg1), "%d", 8080);
      
      pid_t pid = fork();
      if(!pid) { // child
         execlp("python3", "python3", "lucky7_test.py",arg1, (const char*)0);
         // Doesn't return (check error maybe)
      }

      printf("Parent - waiting for given period...\n");
      sleep(30);
      printf("Cleaning resources - START \n");
      kill(pid, SIGTERM); // SIGINT
      sleep(1);
      printf("Cleaning resources - STOP \n");
      return 0;
}
```
> Add GnuRadio runtime variables to path
```
Add this line (with modified path) to: .bashrc
source /path/to/setup_env.sh 
e.g source $HOME/gr-3.8/setup_env.sh 
```

> Design development cycle
![Dev cycle](https://github.com/pavelfpl/gr-gsSDR/blob/master/doppler_correction.png)

## Screenshots

>`http_transfer_sink` with `gr-satellites` blocks: https://github.com/daniestevez/gr-satellites/

![Example Source](https://github.com/pavelfpl/gr-gsSDR/blob/master/examples/http_transfer_sink_example.png)
