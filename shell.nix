{ pkgs ? import <nixpkgs> {} }:

let
  cross = pkgs.pkgsCross.x86_64-embedded;
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    # Cross toolchain
    cross.buildPackages.gcc
    cross.buildPackages.binutils  # inclut objcopy, ld

    # Assembleur
    nasm

    # Bootloader + outils image
    limine
    mtools

    # Emulation
    qemu
    OVMF

    # Utilitaires
    b2sum
  ];

  shellHook = ''
    export LIMINE=${pkgs.limine}/share/limine
    export OVMF_CODE=${pkgs.OVMF.fd}/FV/OVMF_CODE.fd
    export OVMF_VARS=$(pwd)/OVMF_VARS.fd

    if [ ! -f OVMF_VARS.fd ]; then
      cp ${pkgs.OVMF.fd}/FV/OVMF_VARS.fd OVMF_VARS.fd
      chmod 644 OVMF_VARS.fd
    fi
  
    echo "Limine: $LIMINE"
    echo "OVMF_CODE: $OVMF_CODE"
'';
}
