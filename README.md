# raychip-8
Chip-8 interpreter using C with raylib.

![Screenshot](https://github.com/MattDrivenDev/raychip-8/blob/main/screenshot.png)

## Docs/Specification
[http://devernay.free.fr/hacks/chip8/C8TECH10.HTM](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)

## Test Cases
[https://github.com/Timendus/chip8-test-suite?tab=readme-ov-file#chip-8-test-suite](https://github.com/Timendus/chip8-test-suite?tab=readme-ov-file#chip-8-test-suite)

## TIL: Bytes
The initial commit was using the `int` data type all over the place for the various registers and memory etc. I initially chose this because of some of the reading I did with .NET in my [F# Chip-8 Interpreter](https://github.com/MattDrivenDev/Chip-8/blob/master/C8.fsx)... because, a `byte` data type there was going to be converted to either a 32 or 64 bit `int` behind the scenes because the processors are heavily optimized for those operations anyway. So, I figured? Just do that!

But, that gets ropey when I hit some of the C8 instructions like `ADD`... when adding two values together it has to roll over when the value is too high, right?

That's when I learned a little more about bytes: [Is there a 'byte' data type in C++?](https://stackoverflow.com/questions/20024690/is-there-byte-data-type-in-c): as it turns out I should probably be using an `unsigned char` value. Which, is what [raylib](https://www.raylib.com/) is returning from the `LoadFileData()` function (makes sense now!).

## Shorts
Just a note - I'm using `unsigned short` in places... But, really, it should be a `uint16_t`.
