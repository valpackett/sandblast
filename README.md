# Sandblast [![ISC License](https://img.shields.io/badge/license-ISC-brightgreen.svg?style=flat)](https://tldrlegal.com/license/-isc-license)

Sandblast is the missing simple container tool for FreeBSD.

- **Minimalist containerization** -- conceptually similar to [Docker], but [doesn't try to do too much](http://suckless.org/philosophy)
- **No [jail(8)], only [jail(2)]** -- made for ephemeral jails, avoids the infrastructure that was made for persistent jails, does not touch any config files
- **Plugin system** -- most of the jail setup process is done by plugins which are simply shell scripts, you can easily customize anything

[Docker]: http://docker.io
[jail(2)]: http://www.freebsd.org/cgi/man.cgi?query=jail&apropos=0&sektion=2&arch=default&format=html
[jail(8)]: http://www.freebsd.org/cgi/man.cgi?query=jail&apropos=0&sektion=8&arch=default&format=html

## Dependencies

- [Jansson](https://www.tldrlegal.com/l/-isc-license) -- `pkg install jansson`

## Usage

Sandblast reads the jail configuration from a JSON file like this one:

```javascript
{
	"ipv4": "127.0.0.2",
	"hostname": "myjail",
	"process": "#!/bin/sh\npkg install -y nginx",
	"plugins": [
		{
			"name": "mount_dir",
			"options": {
				"source": "/usr/local/worlds/10.1-RELEASE",
				"mode": "ro"
			}
		},
    {
      "name": "mount_dir",
      "options": {
        "source": "/usr/local/containers/nginx",
        "mode": "rw"
      }
    }
	]
}
```

When you run

```bash
# sandblast this_file.json
```

The following will happen:

- `/usr/local/worlds/10.1-RELEASE` will be mounted using `nullfs` in `ro` mode at a temporary mountpoint (`/tmp/sandblast.*`)
- `/usr/local/containers/nginx` will be mounted using `unionfs` in `rw` mode at the same mountpoint
- a jail will be created, with that mountpoint as the filesystem root, `127.0.0.2` as the IPv4 address, hostname and jailname `myjail`
- the `sh` script that calls `pkg install -y nginx` will be saved to a temporary file
- it will be executed inside the jail
- the jail will be destroyed (just in case there are orphan processes left in the jail somehow)
- `/usr/local/containers/nginx` will be unmounted from the root
- `/usr/local/worlds/10.1-RELEASE` will be unmounted from the root

So, this run will create an nginx installation at `/usr/local/containers/nginx`, which then could be mounted in read-only mode in a jail that runs it...

### Plugins

You might have noticed that the JSON for mounting a directory is under the `plugins` array.
Yes, that's right -- even mounting directories is a plugin.

Plugins are executables that accept one command line argument (either `start` or `stop`) and the following environment variables:

- `SANDBLAST_PATH`
- `SANDBLAST_IPV4`
- `SANDBLAST_HOSTNAME`
- `SANDBLAST_JAILNAME`
- `SANDBLAST_JID`

Options from the JSON file are passed as environment variables too (lowercase letters are converted to uppercase letters; everything other than letters is replaced with underscores.)

They are executed in the same order as in the JSON file when starting, and in the reverse order when stopping.
