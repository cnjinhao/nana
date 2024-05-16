# Nana C++ Library 

[![Licence](https://img.shields.io/badge/license-BSL-blue.svg?style=flat)](LICENSE)


Nana is a C++ standard-like GUI library designed to allow developers to easily create cross-platform GUI applications with modern C++ style. Currently it is regularly tested on Linux(X11) and Windows, and experimentally on macOS and FreeBSD. The [nana repository](https://github.com/cnjinhao/nana) contains the entire source of the library. You can browse the source code and submit your pull request for contributing.

## License

Nana is licensed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt)

## Members

Jinhao, [Ariel Viña Rodríguez].

[Ariel Viña Rodríguez]: http://qpcr4vir.github.io/

## Documentation

The best way to get help with Nana library is by visiting https://nana.acemind.cn/documentation

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


## Sending a Pull Request ?

This project encourage you to contribute through sending a pull request! There is a simple rule: please **don't** directly commit your contributions to the **master** branch. According to your commits, please choose the **hotfixes** branch or the **develop** branch. Thank you!

## Introduction to the Repository

There are two main branches with an infinite lifetime:
* **master** is the main branch and it is marked as every version release.
* **develop** is also another main branch where the source code reflects a state with the lastest delivered developement changes for the next release.

Other branches:
* **features** are used to develop new features for the upcoming or a distant future release. Feature branches are named as 'feature-FEATURENAME'.
* **hotfix** is meant to prepare for a new release, and fixes some bugs from the corresponding tag on the master branch.
