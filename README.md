# Nana C++ Library 

[![Ubuntu](https://github.com/cnjinhao/nana/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/cnjinhao/nana/actions?query=workflow%3AUbuntu)

[![Licence](https://img.shields.io/badge/license-BSL-blue.svg?style=flat)](LICENSE)

> **nana 1.8 beta.** `master` now contains nana 1.8, which requires C++17. It is a pre-release while the tests, the
> documentation and this README are being updated. nana 1.7, the last version before C++17, continues in the branch
> [`nana-1.7`](https://github.com/cnjinhao/nana/tree/nana-1.7).

Nana is a C++ standard-like GUI library designed to allow developers to easily create cross-platform GUI applications with modern C++ style. Currently it is regularly tested on Linux(X11) and Windows, and experimentally on macOS and FreeBSD. The [nana repository](https://github.com/cnjinhao/nana) contains the entire source of the library. You can browse the source code and submit your pull request for contributing.

## License

Nana is licensed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt)

## Members

Jinhao, [Ariel Viña Rodríguez].

[Ariel Viña Rodríguez]: http://qpcr4vir.github.io/

## Documentation

> **The documentation is being rebuilt.** The websites nanapro.org and nana.acemind.cn are now offline.
> Meanwhile, please use:
> - [INSTALL.md](INSTALL.md), to build nana and use it in your project;
> - the comments in the source code, mainly in the headers (`include/nana`);
> - the [reference generated with Doxygen](https://qpcr4vir.github.io/nana-doxy/html/), for nana 1.7.2 (2019): mostly still
>   valid, but partly outdated;
> - the [wiki](https://github.com/cnjinhao/nana/wiki);
> - the examples of [nana-demo](https://github.com/qPCR4vir/nana-demo).

## Examples

Here are some examples to give you an idea how to use the Nana C++ Library.

### Create a window

The `form` class provides methods to manipulating a window.
```C++
#include <nana/gui.hpp>

int main()
{
    nana::form fm;
    fm.show();
    nana::exec();
}
```

## Tests

The examples of [nana-demo](https://github.com/qPCR4vir/nana-demo) are also the tests: with
`-DNANA_CMAKE_AUTOMATIC_GUI_TESTING=ON` each one is registered in CTest, and the CI builds and runs them on every push
and pull request. nana-demo also has interaction tests (`InteractionTests/`), which drive the widgets with the real mouse
and keyboard (X11 and Windows).

## Sending a Pull Request

Contributions are welcome! Please send your pull request to:
* **master**, for nana 1.8;
* **nana-1.7**, only for fixes of nana 1.7 (the last version before C++17).

Please keep each pull request small, about one fix or feature. Thank you!

## Branches

* **master**: the current version, nana 1.8. Releases are tags on this branch.
* **nana-1.7**: maintenance of nana 1.7.
* **[dev_dpi_system](https://github.com/cnjinhao/nana/tree/dev_dpi_system)**: experimental support of DPI scaling (work in
  progress, kept up to date with master).
* Other branches are for work in progress on new features.
