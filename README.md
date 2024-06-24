# lpac

lpac is a cross-platform local profile agent program, compatible with [SGP.22 version 2.2.2](https://www.gsma.com/solutions-and-impact/technologies/esim/wp-content/uploads/2020/06/SGP.22-v2.2.2.pdf).

Features:

- Support Activation Code and Confirmation Code
- Support Custom IMEI sent to server
- Support Profile Discovery (SM-DS)
- Profile management: list, enable, disable, delete and nickname
- Notification management: list, send and delete
- Lookup eUICC chip info
- etc

## Usage

You can download lpac from [GitHub Release][latest], and read [USAGE](docs/USAGE.md) to use it.
If you can't run it you need to compile by yourself, see also [DEVELOPERS](docs/DEVELOPERS.md).
If you want to known which Linux distributions include lpac, see also [LINUX-DIST](docs/LINUX-DIST.md).
If you have any issue, please read [FAQ](docs/FAQ.md) first.

[latest]: https://github.com/estkme-group/lpac/releases/latest

## Software Ecosystem

- [EasyLPAC] (Windows, Linux and macOS)
- [{Open,Easy}EUICC][openeuicc] ([Mirror][openeuicc-mirror], Android)
- [eSIM Manager (lpa-gtk)](https://codeberg.org/lucaweiss/lpa-gtk) (Linux Mobile)
- [rlpa-server](https://github.com/estkme-group/rlpa-server) for eSTK.me Cloud Enhance function

[easylpac]: https://github.com/creamlike1024/EasyLPAC/releases/latest
[openeuicc]: https://gitea.angry.im/PeterCxy/OpenEUICC
[openeuicc-mirror]: https://github.com/estkme-group/openeuicc

## Thanks

[![Contributors][contrib]][contributors]

[contrib]: https://contrib.rocks/image?repo=estkme-group/lpac
[contributors]: https://github.com/estkme-group/lpac/graphs/contributors

---

## License

- lpac ([/src](src), [/driver](driver)): AGPL-3.0
- libeuicc ([/euicc](euicc)): LGPL-v2

Copyright &copy; 2023-2024 eSTKme Group
