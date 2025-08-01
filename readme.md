# mintd
a minimal torrent daemon written in C++

## ipc header protocol 
* 16 bit `magic`: `"MT"`
* 16 bit `cmd`
* the rest depends on the `cmd` 

To see an example **client** implementation, see [client.py](client.py).
