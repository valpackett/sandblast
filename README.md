# Sandblast [![ISC License](https://img.shields.io/badge/license-ISC-red.svg?style=flat)](https://tldrlegal.com/license/-isc-license)

Sandblast is the missing simple container tool for [FreeBSD].

- **Minimalist containerization** -- conceptually similar to [Docker], but [doesn't try to do too much](http://suckless.org/philosophy)
- **No [jail(8)], only [jail(3)]** -- made for ephemeral jails, avoids the infrastructure that was made for persistent jails, does not touch any config files
- **Plugin system** -- most of the jail setup process is done by plugins which are simply shell scripts, you can easily customize anything

It's a core building block suitable for any future container system, such as one that supports the [App Container spec].

[FreeBSD]: https://www.FreeBSD.org
[Docker]: http://docker.io
[jail(3)]: https://www.FreeBSD.org/cgi/man.cgi?query=jail&apropos=0&sektion=3}&arch=default&format=html
[jail(8)]: https://www.FreeBSD.org/cgi/man.cgi?query=jail&apropos=0&sektion=8&arch=default&format=html
[App Container spec]: https://github.com/appc/spec/blob/master/SPEC.md

## Dependencies

- FreeBSD, obviously -- currently tested on 10.1
- *For CPU and memory limiting*: the kernel rebuilt with [RCTL/RACCT](https://wiki.freebsd.org/Hierarchical_Resource_Limits)
- *For virtualized networking*: the kernel rebuilt with VIMAGE (and IPFIREWALL/DUMMYNET for traffic limiting -- pf's ALTQ crashes the system when starting vnet jails!)
- (the best kernel configuration is mentioned later in the readme)
- [Jansson](http://www.digip.org/jansson/) -- `pkg install jansson`

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

Sandblast reads the jail configuration from a JSON file like this one:

```javascript
{
	"ipv4": "192.168.1.99",
	"hostname": "myjail",
	"process": "#!/bin/sh\npkg install -y nginx",
	"plugins": [
		{
			"name": "mount_dir",
			"options": {
				"source": "/var/worlds/10.1-RELEASE",
				"mode": "ro"
			}
		},
		{
			"name": "mount_dir",
			"options": {
				"source": "/var/containers/nginx",
				"mode": "rw"
			}
		}
	]
}
```

When you run

```bash
$ sudo sandblast this_file.json
```

The following will happen:

- `/var/worlds/10.1-RELEASE` will be mounted using `nullfs` in `ro` mode at a temporary mountpoint (`/tmp/sandblast.*`)
- `/var/containers/nginx` will be mounted using `unionfs` in `rw` mode at the same mountpoint
- a jail will be created, with that mountpoint as the filesystem root, `192.168.1.99` as the IPv4 address, hostname and jailname `myjail`
- the `sh` script that calls `pkg install -y nginx` will be saved to a temporary file
- it will be executed inside the jail
- after it's done, the jail will be destroyed (just in case there are orphan processes left in the jail somehow)
- `/var/containers/nginx` will be unmounted from the mountpoint
- `/var/worlds/10.1-RELEASE` will be unmounted from the mountpoint

So, this run will create an nginx installation at `/var/containers/nginx`, which then could be mounted in read-only mode in a jail that runs it...

### Plugins

All system configuration for container jails is done using plugins.

Plugins are executables (typically `/bin/sh` scripts) that accept one command line argument (either `start` or `stop`) and the following environment variables:

- `SANDBLAST_PATH`
- `SANDBLAST_IPV4`
- `SANDBLAST_HOSTNAME`
- `SANDBLAST_JAILNAME`
- `SANDBLAST_JID`

Options from the JSON file are passed as environment variables too (lowercase letters are converted to uppercase letters; everything other than letters is replaced with underscores.)

They are executed in the same order as in the JSON file when starting, and in the reverse order when stopping.

## Security

*This code has not been audited yet!*
You might want to wait some time before you start building your Heroku killer on top of it.
Please do test it on your desktops though :-)

### Trust

- The jail configuration JSON files **are trusted**.
  When building PaaS/CI/hosting/etc. services, you need to make sure *your software* generates valid configuration that doesn't eat your system.
- The plugins **are trusted** (obviously).
  Plugins are executed on the host system with root privileges, because they need to set up the sandbox (using things like `mount` and `ifconfig`.)
- The `process` field of the JSON **is untrusted** -- it's executed inside of the jail.

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
options IPFIREWALL
options DUMMYNET
options HZ=1000
```

See [Building and Installing a Custom Kernel](https://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/kernelconfig-building.html) for further instructions.
tl;dr:

```shell
$ cd /usr/src
$ sudo make buildkernel KERNCONF=SANDBLAST
$ sudo make installkernel KERNCONF=SANDBLAST
$ sudo shutdown -r now
```

## Copyright

Copyright (c) 2014-2015 Greg V <greg@unrelenting.technology> 
Available under the ISC license, see the `COPYING` file
