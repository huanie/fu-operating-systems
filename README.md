# Configure Toolchain
# Build
```sh
cargo build
```
# Run
```sh
qemu-system-arm -nographic -M portux920t -m 64M -kernel target/armv4t-none-eabi/debug/kernel
```
