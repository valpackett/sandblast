# Sandblast [![ISC License](https://img.shields.io/badge/license-ISC-red.svg?style=flat)](https://tldrlegal.com/license/-isc-license)

Sandblast is the missing simple container tool for [FreeBSD].

[FreeBSD]: https://www.FreeBSD.org

## Dependencies

- FreeBSD, obviously -- currently tested on 10.2
- `pkg install libucl`
- *For CPU and memory limiting*
  - *10.2 and newer*: add `kern.racct.enable=1` to `/boot/loader.conf` and reboot
  - *10.1 and older*: [rebuild the kernel](https://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/kernelconfig-building.html) with [RCTL/RACCT](https://wiki.freebsd.org/Hierarchical_Resource_Limits) (and reboot, obviously)

## Installation

For now, `git clone`, `make` and `sudo make install`.

## Usage

TODO

## Security

*This code has not been audited yet!*
You might want to wait some time before you start building your Heroku killer on top of it.
Please do test it on your desktops though :-)

### Trust

- The jail configuration files **are trusted**.
  When building PaaS/CI/hosting/etc. services, you need to make sure *your software* generates valid configuration that doesn't eat your system.
- The `process` field of the configuration **is untrusted** -- it's executed inside of the jail.

### setuid

You *can* run sandblast as a non-root user if you set the setuid bit on the `sandblast` binary, like so:

```shell
$ sudo chmod 4755 /usr/local/bin/sandblast
```

Think carefully before you make any binaries setuid though.

## Copyright

Copyright (c) 2014-2015 Greg V <greg@unrelenting.technology>  
Available under the ISC license, see the `COPYING` file
