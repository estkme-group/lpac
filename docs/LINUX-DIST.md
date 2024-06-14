# Linux distributions

> [!CAUTION]
>
> **All Linux distribution packages are unofficially maintained.**

## OpenWrt

> Minimum available release: Snapshot
> (Added on 2024-05-15)

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

## Arch Linux

> Need to enable [archlinuxcn repo](https://github.com/archlinuxcn/repo#readme) first

```console
# pacman -S lpac
# # or lpac-git
# pacman -S lpac-git
```

see <https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/lpac/PKGBUILD> \
see <https://github.com/archlinuxcn/repo/blob/master/archlinuxcn/lpac-git/PKGBUILD>

## Nix OS

> Need to enable [NUR](https://github.com/nix-community/NUR#readme "Nix User Repository") first

```console
# nix-env -i lpac
```

see <https://github.com/nix-community/nur-combined/blob/master/repos/linyinfeng/pkgs/lpac/default.nix>
