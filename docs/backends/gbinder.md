# GBinder APDU Backend

GBinder-based backends for `libhybris` (Halium) distributions

## HIDL

> HAL interface definition language or HIDL is an interface description language (IDL) to specify the interface between
> a HAL and its users. HIDL allows specifying types and method calls, collected into interfaces and packages. More
> broadly, HIDL is a system for communicating between codebases that may be compiled independently.

This backend uses the [`android.hardware.radio@1.0::IRadio`] HIDL interface to send and receive APDUs via the RIL.

[`android.hardware.radio@1.0::IRadio`]: https://android.googlesource.com/platform/hardware/interfaces/+/3c927eee/radio/1.0/IRadio.hal

## Environment Variables

- `LPAC_APDU_GBINDER_DEBUG`: enable debug output for GBinder APDU backend. \
  ([boolean](types.md#boolean-type), default: `false`)

## References

- <https://source.android.com/docs/core/architecture/hidl>
