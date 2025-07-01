with import <nixpkgs> {};
pkgs.mkShell {
  buildInputs = [
    gnumake
    wayland
  ];
}
