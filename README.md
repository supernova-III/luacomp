# luacomp
This project aims to implement a compiler for the Lua programming language according to the official lua spec: [[link]](https://www.lua.org/manual/5.4/contents.html#contents)

## My feelings
Tokenizer is almost ready but I feel that I have no control over it. It must be reviewed and analyzed.

- I don't like how I implemented scanning of numbers, it seems overcomplicated
- I feel bad about the string table, the entire algorithm must be specified
- Scanning of all kinds of string literals annoys me. The entire algorithm must be specified

General note: the entire scanner operation must be specified, how do we iterate over the input, and so on.
