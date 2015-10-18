# Sandblast [![ISC License](https://img.shields.io/badge/license-ISC-red.svg?style=flat)](https://tldrlegal.com/license/-isc-license)

Sandblast is the missing simple container tool for [FreeBSD].

- **No [jail(8)], only [jail(3)]** -- made for ephemeral jails, avoids the infrastructure that was made for persistent jails, does not touch any config files

[FreeBSD]: https://www.FreeBSD.org
[jail(3)]: https://www.FreeBSD.org/cgi/man.cgi?query=jail&apropos=0&sektion=3}&arch=default&format=html
[jail(8)]: https://www.FreeBSD.org/cgi/man.cgi?query=jail&apropos=0&sektion=8&arch=default&format=html

## Dependencies

- FreeBSD, obviously -- currently tested on 10.2
- *For CPU and memory limiting*: the kernel rebuilt with [RCTL/RACCT](https://wiki.freebsd.org/Hierarchical_Resource_Limits)
- *For bandwidth limiting*: the kernel rebuilt with ALTQ
- (the best kernel configuration is mentioned later in the readme)

## Installation

Sandblast will be in the ports tree when it's ready.
For now, `git clone`, `make` and `sudo make install`.

## Usage

First, you need a base jail, which is just a directory with a fresh FreeBSD installation, usually with ports and pkg ready to use.
*Something like this* sets it up (ZFS is not required, obviously, and `examples/bootstrap.json` references `/var/worlds/10.1-RELEASE` and the `vtnet0` interface, so you probably want to customize it):

```shell
$ sudo zfs create zroot/var/worlds/10.1-RELEASE
$ sudo bsdinstall jail /var/worlds/10.1-RELEASE
  # select: empty root password, no services, no users
$ sudo sandblast ./examples/bootstrap.json
```

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

## Building a custom FreeBSD kernel

You must have the FreeBSD source tree at /usr/src -- if you didn't choose it when installing, there are several ways to get it.
One of them is cloning [freebsd/freebsd](https://github.com/freebsd/freebsd) from GitHub and checking out the appropriate branch (don't forget!).
Once you have the source, you need a configuration file.

Put the following in `/usr/src/sys/amd64/conf/SANDBLAST` (where `amd64` is your CPU architecture):

```
include GENERIC
ident SANDBLAST

options RCTL
options RACCT
options VIMAGE
options NULLFS
options UNIONFS
options TMPFS

options ALTQ
options ALTQ_RED
options ALTQ_RIO
options ALTQ_HFSC
options ALTQ_PRIQ
options ALTQ_NOPCC

options SC_KERNEL_CONS_ATTR=(FG_GREEN|BG_BLACK)
```

See [Building and Installing a Custom Kernel](https://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/kernelconfig-building.html) for further instructions.
tl;dr:

```shell
$ cd /usr/src
$ sudo make buildkernel KERNCONF=SANDBLAST
$ sudo make installkernel KERNCONF=SANDBLAST
$ sudo reboot
```

## Copyright

Copyright (c) 2014-2015 Greg V <greg@unrelenting.technology>  
Available under the ISC license, see the `COPYING` file
