# ctimer
simple console timer

## install
```
git clone https://github.com/nagatoxxx/ctimer.git
cd ctimer/
make && make install
```
this will install `ctimer` to `~/.local/bin` \
add this directory to `PATH` or manually move it to the desired path 

## dependencies
```
libnotify
```
## example usage
* countdown 30s:
``` 
$ ctimer -c 30
```
* send notification at 7:15 (process becomes background):
```
$ ctimer -t 7:15 -n "time is over!"
```
* combine with other programs:
```
$ ctimer -t 7:15 && notify-send "time is over!"
```
```
$ ctimer -c 30 && swaylock
```
