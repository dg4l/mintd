# mintd
a minimal torrent daemon written in C++

> [!CAUTION]
> EXPERIMENTAL SOFTWARE !!!!!! THERE IS SOME HACKY STUFF, WILL BE FIXED LATER.


## ipc header protocol
> [!CAUTION]
> This protocol is subject to change whenever i feel like it, however the [example client](client.py) will always be up to date.

* 16 bit `magic`: `"MT"`
* 16 bit `cmd`
* the rest depends on the `cmd` 

To see an example **client** implementation, see [client.py](client.py).

## Building

```bash
cd src
meson setup build
cd build
meson compile
```

## Rough TODO
### these mainly have to do with decreasing hackiness.
* Improve packet parsing
* Improve error handling


