{
  description = "UPPAAL DBM Library";

  inputs.nixpkgs.url = "nixpkgs/nixos-21.11";

  outputs = { self, nixpkgs }:
  let
      # System types to support.
      supportedSystems = [ "x86_64-linux" "x86_64-darwin" "aarch64-linux" "aarch64-darwin" ];
      # Helper function to generate an attrset '{ x86_64-linux = f "x86_64-linux"; ... }'.
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      # Nixpkgs instantiated for supported system types.
      nixpkgsFor = forAllSystems (system: import nixpkgs { inherit system; });
  in
  {
    packages = forAllSystems (system:
    let
      pkgs = nixpkgsFor.${system};
    in
    {
      UDBM = pkgs.stdenv.mkDerivation {
        pname = "UDBM";
        version = "0.0.0";
        src = ./.;
      };
    });
    devShell = forAllSystems (system:
    let pkgs = nixpkgsFor.${system};
    in pkgs.mkShell {
      buildInputs = with pkgs; [ cmake doctest boost ninja clang-tools ];
    });
  };
}
