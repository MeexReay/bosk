with import <nixpkgs> {};
pkgs.mkShell {
  buildInputs = [
    gnumake
    wayland
    wayland-scanner
    wayland-protocols
    pkg-config
  ];
}
