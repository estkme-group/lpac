# Linux distributions

> [!CAUTION]
>
> **All Linux distribution packages are unofficially maintained.**

[![Packaging status][packaging-badge]][packaging-versions]

[packaging-badge]: https://repology.org/badge/vertical-allrepos/lpac.svg
[packaging-versions]: https://repology.org/project/lpac/versions

## OpenWrt

> Minimum available release: [24.10.2](https://downloads.openwrt.org/releases/24.10.2/targets/)
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

## NixOS

> Available in nixpkgs.
> (Added on 2025-01-30)

```shell
nix-shell -p lpac
```

see <https://github.com/NixOS/nixpkgs/blob/master/pkgs/by-name/lp/lpac/package.nix>

## Ubuntu and Debian/Devuan

> Minimum available release: Ubuntu [14.04 Trusty Tahr](https://releases.ubuntu.com/14.04/)
> (Published on 2024-09-01)

see <https://launchpad.net/~daniel-gimpelevich/+archive/ubuntu/ssl/+sourcepub/17063242/+listing-archive-extra>

## SailfishOS

> (Published on 2025-12-21)
> You need to enable [chum repo](https://chumrpm.netlify.app/) first

GUI
- Install [Chum GUI Installer](https://openrepos.net/content/olf/sailfishoschum-gui-installer) from [Storeman](https://openrepos.net/content/olf/storeman-installer) to enable chum repo
- Open Chum GUI annd refresh chum repo
- Install [lpac](https://openrepos.net/content/juanro49/lpac) from [Storeman](https://openrepos.net/content/olf/storeman-installer)

CLI
```shell
devel-su
#add chum repo
ssu ar sailfishos-chum "https://repo.sailfishos.org/obs/sailfishos:/chum/$(grep '^VERSION_ID=' /etc/os-release | cut -d= -f2 | cut -d. -f1-2)_%(arch)/"
#add Juanro49 openrepos repo
ssu ar openrepos-Juanro49 https://sailfish.openrepos.net/Juanro49/personal/main
# refresh repositories
zypper refresh
# or
pkcon refresh
#install lpac
zypper install lpac
#or
pkcon install lpac
```

see <https://openrepos.net/content/juanro49/lpac>
