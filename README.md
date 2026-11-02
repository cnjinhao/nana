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

> **The documentation is being rebuilt.** The old website (nanapro.org, later nana.acemind.cn) is no longer online.
> Meanwhile, please use:
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

## Sending a Pull Request ?

This project encourage you to contribute through sending a pull request! There is a simple rule: please **don't** directly commit your contributions to the **master** branch. According to your commits, please choose the **hotfixes** branch or the **develop** branch. Thank you!

## Introduction to the Repository

There are two main branches with an infinite lifetime:
* **master** is the main branch and it is marked as every version release.
* **develop** is also another main branch where the source code reflects a state with the lastest delivered developement changes for the next release.

Other branches:
* **features** are used to develop new features for the upcoming or a distant future release. Feature branches are named as 'feature-FEATURENAME'.
* **hotfix** is meant to prepare for a new release, and fixes some bugs from the corresponding tag on the master branch.
