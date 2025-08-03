# mintd
a minimal torrent daemon written in C++

> [!CAUTION]
> This protocol is subject to change whenever i feel like it, however the [example client](client.py) will always be up to date.

## ipc header protocol 
* 16 bit `magic`: `"MT"`
* 16 bit `cmd`
* the rest depends on the `cmd` 

To see an example **client** implementation, see [client.py](client.py).
