// MARGIN CALL - Desktop-App (Windows, macOS, Linux)
// Startet den eingebauten Spiel-Server und zeigt das Spiel in einem eigenen Fenster.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod server;

use tauri::{WebviewUrl, WebviewWindowBuilder};

fn main() {
    let port = server::start();
    tauri::Builder::default()
        .setup(move |app| {
            let url = format!("http://127.0.0.1:{port}/?app=1&home={port}");
            WebviewWindowBuilder::new(app, "main", WebviewUrl::External(url.parse().expect("gueltige URL")))
                .title("MARGIN CALL")
                .inner_size(1280.0, 780.0)
                .min_inner_size(640.0, 400.0)
                // Das Spiel beendet sich, indem es diese Adresse aufruft ("Beenden" im Menue).
                .on_navigation(|url| {
                    if url.host_str() == Some("margincall.quit") {
                        std::process::exit(0);
                    }
                    true
                })
                .build()?;
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("MARGIN CALL konnte nicht starten");
}
