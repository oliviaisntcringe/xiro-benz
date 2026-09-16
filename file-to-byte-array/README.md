# File to Byte Array

`file-to-byte-array` converts a binary file into a C++ header that can be
included directly in another Visual Studio project.

```text
file-to-byte-array.exe <input-file> [output-header] [array-name]
```

When the optional output path is omitted, the header is written as
`<input-stem>.h` in the current directory. The generated header contains the
byte array and a `<array-name>_size` constant.
