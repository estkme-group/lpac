# Linux distributions

## OpenWrt

> Minimum available release: Snapshot
> (Before **OpenWrt 23.x** Release)

```console
# opkg install lpac
```

see <https://github.com/openwrt/packages/blob/master/utils/lpac/Makefile>

## Alpine

> Minimum available release: [v3.20.0](https://pkgs.alpinelinux.org/packages?name=lpac&branch=v3.20)
> (Release date: 2024-05-22).

```console
# pkg install lpac
```

see <https://github.com/alpinelinux/aports/blob/master/community/lpac/APKBUILD>

## Archlinux

```console
# pacman -S lpac
# # or lpac-git
# pacman -S lpac-git
```

see <https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/lpac/PKGBUILD> \
see <https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/lpac-git/PKGBUILD>

## Nix OS

```console
# nix-env -i lpac
```

see <https://github.com/nix-community/nur-combined/blob/master/repos/linyinfeng/pkgs/lpac/default.nix>
