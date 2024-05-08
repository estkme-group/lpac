## FAQ

### Any subcommand of lpac will get error message like "SCardListReaders() failed:" and 8-digits error code, such as 80100069, 80100066, 8010002E, 8010000F and so on.

- [80100069] means your UICC is not plugged correctly
- [80100066] means your card has no response, please clean the pin and plug in again
- [8010002E] means communication error
- [8010000F] means the card is not a eUICC, or detect wrong card reader like Yubikey. For latter one, you can use `lpac driver apdu list` to list all reader and use `$DRIVER_IFID` to specify correct card reader
- for others, Google is your friend.

[80100069]: https://pcsclite.apdu.fr/api/group__ErrorCodes.html#gaa2efd953946973972b1afc5d0343820c
[80100066]: https://pcsclite.apdu.fr/api/group__ErrorCodes.html#ga359a9e85e3b7c83c76507a096452b74f
[8010002E]: https://pcsclite.apdu.fr/api/group__ErrorCodes.html#ga81b59e9319d3fcd0d957d98781b3ebd2
[8010000F]: https://pcsclite.apdu.fr/api/group__ErrorCodes.html#ga36d821a0458f935ddbe345f10408a988

### I can't download eSIM profile of xxx.

The verification of SM-DP+ servers of telecom operators is diverse. Please check whether the parameters you enter are consistent with those provided to you by the telecom operators. Some telecom operators issue profiles in the form of push, which may require the use of lpac's discovery and custom IMEI function.
