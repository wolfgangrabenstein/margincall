fn main() {
    println!("cargo:rerun-if-changed=../../pc/game.html");
    tauri_build::build()
}
