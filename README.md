# Binn

Matlab MEX functions to decode and encode with Binn format

Original C Binn Library: https://github.com/liteserver/binn

Binn Specification: https://github.com/liteserver/binn/blob/master/spec.md

# Compilation

Visit Matlab help for mex introduction and for information on how to setup compiler.

When mex compiler is ready, just run script `compileBinn.m`

It will create two mex functions: `binnDecode` and `binnEncode`

# Limitations

Currently only a limited set of Binn and Matlab types is supported. For example, when encoding there is no support for
matrices (only row-vectors and column-vectors are supported). When decoding there is no support for maps.

## Encoding

* Input must be a structure, numeric array (1xN or Nx1) or cell array (1xN or Nx1)
* Structures are encoded as Binn objects
* Numeric and cell arrays are encoded into lists. Int compression is used when possible
* Scalar values are encoded into appropriate Binn number (also with int compression)
* If scalar value was used as input, it is encoded into list of one element

## Decoding

* Input must be a vector of uint8 values
* Objects are decoded into Matlab structures
* Lists are decoded to numeric array (if it contatins only numbers)
* Lists with non-numeric data are decoded to cell arrays
* Strings are decoded into char arrays
* Lists are always decoded into rows (1xN) even if when encoded, it was a column (Nx1)
* Cell arrays, with only scalar numbers, after encoding, will be a list of numbers. So they're decoded into simple
  numeric arrays

# Examples

See `testBinn.m` for some encoding/decoding example.

# Contributing

If you find bugs or want support for some other types, feel free to create an issue.
Pull requests are welcome too.
