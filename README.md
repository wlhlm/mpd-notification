mpd-notification
================

This little program creates a notification everytime `mpd` plays a
track. A lot of inspiration has been taken from [mpd-notify][notify]

Building/Installation
---------------------

Before bulding feel free to tweak some default settings in
`config.def.h` and copy the file afterwards to `config.h`. Simply build
`mpd-notification` with:
```
make
```
and for installation, just run:
```
make install
```
One might want to adjust the `PREFIX` directory:
```
make install PREFIX=/usr
```

Dependencies
------------

- libnotify
- libmpdclient
- glib2

License
-------

`mpd-notification` is licensed under a MIT license. For more details
please see the `LICENSE` file.


[notify]: https://github.com/Unia/mpd-notify
