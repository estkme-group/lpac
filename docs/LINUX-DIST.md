# Linux distributions

> [!CAUTION]
>
> **All Linux distribution packages are unofficially maintained.**

## OpenWrt

> Minimum available release: Snapshot
> (Added on 2024-05-15)

```shell
opkg install lpac
```

see <https://github.com/openwrt/packages/blob/master/utils/lpac/Makefile>

## Alpine

> Minimum available release: [v3.20.0](https://pkgs.alpinelinux.org/packages?name=lpac&branch=v3.20)
> (Release date: 2024-05-22).

```shell
pkg install lpac
```

see <https://github.com/alpinelinux/aports/blob/master/community/lpac/APKBUILD>

## Arch Linux

> Need to enable [archlinuxcn repo](https://github.com/archlinuxcn/repo#readme) first
>
> If you want to use AUR, the package name is [lpac-git](https://aur.archlinux.org/packages/lpac-git)

```shell
pacman -S lpac
# or
pacman -S lpac-git
```

see <https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/lpac/PKGBUILD> \
see <https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/lpac-git/PKGBUILD>

## Nix OS

> Need to enable [NUR](https://github.com/nix-community/NUR#readme "Nix User Repository") first

```shell
nix-env -i lpac
```

see <https://github.com/nix-community/nur-combined/blob/master/repos/linyinfeng/pkgs/lpac/default.nix>

## Ubuntu and Debian/Devuan

> Minimum available release: Ubuntu [14.04 Trusty Tahr](https://releases.ubuntu.com/14.04/)
> (Published on 2024-09-01)

see <https://launchpad.net/~daniel-gimpelevich/+archive/ubuntu/ssl/+sourcepub/16426799/+listing-archive-extra>
