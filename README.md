*This was a good learning experiment, but `jail(8)` is actually fine.*

# Sandblast [![ISC License](https://img.shields.io/badge/license-ISC-red.svg?style=flat)](https://tldrlegal.com/license/-isc-license)

Sandblast is a sandbox/container tool for [FreeBSD].

Like `jail(8)`, but:

- no persistent/system-wide config files like `jail.conf`
- just pass JSON (or nginx-style) configuration for ONE jail as a file or to stdin
- it will manage nullfs/unionfs mounts and RCTL resource limits
- (manage, as in, both set and clean up)

[FreeBSD]: https://www.FreeBSD.org

## Dependencies

- FreeBSD, obviously -- at least 10.x
- `pkg install pkgconf libucl`
- *For CPU and memory limiting*
  - *10.2 and newer*: add `kern.racct.enable=1` to `/boot/loader.conf` and reboot
  - *10.1 and older*: [rebuild the kernel](https://www.freebsd.org/doc/en_US.ISO8859-1/books/handbook/kernelconfig-building.html) with [RCTL/RACCT](https://wiki.freebsd.org/Hierarchical_Resource_Limits) (and reboot)

## Installation

For now, `git clone`, `make` and `sudo make install`.

## Usage

Sandblast runs configuration files parsed by libucl, which allows both a human-friendly (nginx-style) configuration language and JSON.

Here's an example:

```conf
ipv4 = ["192.168.122.67", "128.4.4.4"]; # one address or array of addresses
ipv6 = 2001:dbca::2; # one address or array of addresses
net_iface = vtnet0;
hostname = myjail;
# jailname = myjail; # default: same as hostname
# securelevel = 3; # default: 3
# devfs_ruleset = 4; # default: 4
resources { # rctl deny
	pcpu = 50;
}
mount = [ # nullfs/unionfs (automatically uses unionfs when "to" is the same)
	{ from = /usr/jails/base/10.2-RELEASE, to = /, readonly = true },
	{ from = /tmp/myjail-storage, to = / },
	{ from = /usr/local, to = /usr/local, readonly = true },
	{ from = /home/user, to = /home/user }
]
script = "#!/bin/sh\nTERM=screen-256color exec sh";
```

## Security

*This code has not been audited yet!*

Configuration parsing (libucl) is sandboxed using Capsicum.

### Trust

- The jail configuration files **are trusted**.
  When building PaaS/CI/hosting/etc. services, you need to make sure *your software* generates valid configuration that doesn't eat your system.
- Except for the `script` field, which **is untrusted** -- it's executed inside of the jail.

### setuid

You *can* run sandblast as a non-root user if you set the setuid bit on the `sandblast` binary, like so:

```shell
$ sudo chmod 4755 /usr/local/bin/sandblast
```

## Copyright

Copyright (c) 2014-2016 Greg V <greg@unrelenting.technology>  
Available under the ISC license, see the `COPYING` file
