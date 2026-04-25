{ pkgs ? import <nixpkgs> { } }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    nasm
    qemu
    mtools
    dosfstools
    gnumake
    pkgsCross.i686-embedded.buildPackages.gcc
    pkgsCross.i686-embedded.buildPackages.binutils
  ];
}
