
<p align="center">
    <img src="https://github.com/lzungri/laritos/wiki/resources/logo.png">
</p>


***

# laritOS  [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

laritOS is a highly-configurable, modular, test-driven developed educational operating system created entirely from scratch, from bootloader to kernel, kernel to drivers, toolchain (including a minimal libc) to userspace applications.

The primary aim of laritOS is to provide a set of free resources (codebase, documentation, tutorials) for learning about operating system internals. Therefore, its focus is put mainly on simplicity, maintainability, modularity, debuggability and testability.
Aspects such as performance, security, compatibility with existing OSes are not the priority for now.

The operating system is composed of the following 3 components, each of them with its corresponding repository:
| Component  | Description |
| ---------  | ----------- |
| [laritos](https://github.com/lzungri/laritos)  | Kernel + drivers + board-specific resources + tests + misc tools  |
| [laritos-toolchain](https://github.com/lzungri/laritos-toolchain)  | Minimal `libc` + userspace apps building tools  |
| [laritos-apps](https://github.com/lzungri/laritos-apps) | Userland applications such as `shell`, `ps` unix-like tool for listing processes, testing apps, etc


<p align="center">
    <img src="https://github.com/lzungri/laritos/wiki/resources/home.gif" width="700">
</p>

## Wiki
For the complete OS documentation, please refer to the [wiki page](https://github.com/lzungri/laritos/wiki)

## Features
### Kernel features
* Multitasking with kernel preemption
* Userspace/kernelspace and system call support
* Testing infrastructure for Test Driven Development (TDD) with more than 250 tests already implemented
* Interrupt management infrastructure
* ARM 32bits support (`armv7a`)
* Spinlock support
* Semaphores, reentrant mutex and atomic32/64 support
* Priority-based scheduler (round robin by default)
* Support for multiple architectures by separating the kernel generic code from the architecture-specific code. So far, only the `armv7a` (arm 32 bits) arch has been implemented
* Monolitic but modular kernel (not a microkernel :cry:)
* Configurable kernel parameters via the `Kbuild` infrastructure, this is the same builder used by Linux ported to laritOS
* Configurable target hardware specs via `board/<boardname/*.bi` (board information) files. This could be seen as a very simplified version of the Linux device tree 
* Hierarchical Virtual File System (`VFS`) supporting `ext2` and `pseudofs` (aka in-memory filesystems, e.g. `/proc`, `stats` fs)
* Kernel heap and stack protector
* Dynamic system configuration via properties (`/property` pseudo filesystem)
* Monouser
* Userspace shell
* ELF support for userspace applications
* Health monitor service configurable via system properties

#### Future features
* SMP support
* UI
* Multiuser support
* Virtual memory
* Memory protection
* and many more...

### Toolchain features

* Easy to use build tools
* Minimal libc support for functions such as sleep, chdir, listdir, open, close, read, write, spawn_process, waitpid, time, printf, yield, exit, and more
* ARM 32bits support (`armv7a`)
* Debug utilities

See [laritos-toolchain](https://github.com/lzungri/laritos-toolchain) repo for more info

### Userspace features

* Minimal shell supporting commands such as cat, xxd, run, run in the background, cd, mkdir, ls, getprop, and others
* `ps` program for listing active processes

See [laritos-app](https://github.com/lzungri/laritos-app) repo for more info

## Building, installing and running

See [wiki page](https://github.com/lzungri/laritos/wiki)

## Status
laritOS is currently in alpha version, hence some features may not work as expected and stability is not guaranteed. Use at your own risk :boom:

## License 
The code is licensed under the [MIT](https://github.com/lzungri/laritos/blob/master/LICENSE.md) license.
