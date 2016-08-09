## gimp-webp

[ Now part of GIMP should make show up for 2.9.5 or 2.9.6, currently in the gimp git repository (master)]
This plugin provides [Gimp](https://developers.google.com/speed/webp/) with the ability to load and export WebP images. During export, a dialog is presented that provides access to image quality settings.

![Export Dialog](images/gimp-webp.jpg)

The plugin is designed to run on all platforms currently supported by the Gimp.

### Building

In order to build this plugin, you will need the following app, tools, and libraries installed:

 - CMake 2.8.12+
 - Gimp 2.8+ development files 
 - Webp 0.5+ development files 
 - Gegl 0.3.8+ development files 
   (required only for use with Gimp 2.9+)

The build process consists of:

    mkdir build
    cd build
    cmake ..
    make

(Note: exif and xmp support requires patching exiv2 as it doesn't support webp yet.)

### Installation

On most *nix platforms, installation is as simple as:

    sudo make install

If you don't have root privileges, you can copy the `src/file-webp` binary to the appropriate directory:

- **Gimp 2.8.x:** &mdash; `~/.gimp-2.8/plug-ins/`
- **Gimp 2.9.x:** &mdash; `~/.config/GIMP/2.9/plug-ins/`
