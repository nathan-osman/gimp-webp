## gimp-webp

This plugin provides [Gimp](https://developers.google.com/speed/webp/) with the ability to load and export WebP images. During export, a dialog is presented that provides access to image quality settings.

![Export Dialog](http://i.stack.imgur.com/0TXi2.png)

The plugin is designed to run on all platforms currently supported by the Gimp.

### Building

In order to build this plugin, you will need the following tools and libraries installed:

 - [CMake 2.8.12+](http://www.cmake.org/)
 - Gimp 2.8+ development files (`libgimp2.0-dev` on Ubuntu)
 - Webp 0.4+ development files (`libwebp-dev` on Ubuntu)

The build process consists of:

    mkdir build
    cd build
    cmake ..
    make

### Installation

On most *nix platforms, installation is as simple as:

    sudo make install

If you don't have root privileges, you can copy the `src/file-webp` binary to `~/.gimp-2.8/plug-ins/`.
