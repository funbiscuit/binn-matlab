# Binn

Matlab MEX functions to decode and encode with Binn format

Original C Binn Library: https://github.com/liteserver/binn

Binn Specification: https://github.com/liteserver/binn/blob/master/spec.md

# Compilation

Visit Matlab help for mex introduction and for information on how to setup compiler.

When mex compiler is ready, just run script `compileBinn.m`

It will create two mex functions: `binnDecode` and `binnEncode`

# Limitations

Currently only a limited set of Binn and Matlab types is supported.
For example, when encoding there is no support for cells, matrices and column vectors.
When decoding there is no support for maps and lists with non numeric types.

## Encoding

* Input must be a structure
* Structures are encoded as Binn objects
* Row vectors are encoded into list of numbers (int compression is used when possible)
* Scalar values are encoded into appropriate Binn number (also with int compression)

## Decoding

* Input must be a vector of uint8 values
* Objects are decoded into Matlab structures
* Only lists of numeric types are supported. When lists contains different types, largest
one is used for created row vector
* Strings are decoded into char arrays

# Examples

See `testBinn.m` for some encoding/decoding example.

# Contributing

If you find bugs or want support for some other types, feel free to create an issue.
Pull requests are welcome too.
