# Nana C++ Library [![BiiCode build status](https://webapi.biicode.com/v1/badges/qiangwu/qiangwu/nana/master)](https://www.biicode.com/qiangwu/nana) [![TravisCI build status](https://travis-ci.org/cnjinhao/nana.svg)](https://travis-ci.org/cnjinhao/nana) [![Licence](https://img.shields.io/badge/license-BSL-blue.svg?style=flat)](LICENSE_1_0.txt)


Nana is a C++ library designed to allow developers to easily create cross-platform GUI applications with modern C++11 style, currently it can work on Linux(X11) and Windows. The nana repository contains the entire source of library, you can browse the source code and submit your pull request for contributing.

## License

Nana is licensed under the [Boost Software License](http://www.boost.org/LICENSE_1_0.txt)

## Biicode
Nana is available in biicode, download biicode and try the nana example:

```
> mkdir try-nana
> cd try-nana
> bii init
> bii open qiangwu/nana-example
> bii find
> bii build
> cd bin
```

Run it! All dependencies will be resovled automatically by biicode! Amazing, isn't it?

## Support

The best way to get help with Nana library is by visiting http://nanapro.org/help.htm

## Sending a Pull Request ?

This project is encourage you to contribute it through sending a pull request! There is a simple rule, please **don't** directly commit your contributions to the **master** branch. According to your commits, please choose the **hotfixes** branch or the **develop** branch. Thank you!

## Introduction to the Repository

There are two main branches with an infinite lifetime:
* **master** is the main branch and it is marked as every version release.
* **develop** is also another main branch where the source code reflects a state with the lastest delivered developement changes for the next release.

Other branches:
* **features** are used to develop new features for the upcoming or a distant future release. Feature branches are named as 'feature-FEATURENAME'.
* **hotfix** is meant to prepare for a new release, and fixes some bugs from the corresponding tag on the master branch.
