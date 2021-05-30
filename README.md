#Binn

Matlab MEX functions to decode and encode with Binn format

Original C Binn Library: https://github.com/liteserver/binn

Binn Specification: https://github.com/liteserver/binn/blob/master/spec.md

# Compilation

Visit Matlab help for mex introduction and for information on how to setup compiler.

When mex compiler is ready, just run script `compileBinn.m`

It will create two mex functions: `binnDecode` and `binnEncode`

# Limitations

Currently only a limited set of Binn types is supported.
