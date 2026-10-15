// MARGIN CALL - Desktop-App (Windows, macOS, Linux)
// Startet den eingebauten Spiel-Server und zeigt das Spiel in einem eigenen Fenster.
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

mod server;

use tauri::{Manager, WebviewUrl, WebviewWindowBuilder};

fn main() {
    let port = server::start();
    tauri::Builder::default()
        .setup(move |app| {
            let handle = app.handle().clone();
            let url = format!("http://127.0.0.1:{port}/?app=1&home={port}");
            WebviewWindowBuilder::new(app, "main", WebviewUrl::External(url.parse().expect("gueltige URL")))
                .title("MARGIN CALL")
                .inner_size(1280.0, 780.0)
                .min_inner_size(640.0, 400.0)
                // Das Spiel steuert die App ueber besondere Adressen:
                // margincall.quit = Beenden, margincall.fullscreen = Vollbild an/aus (F4).
                .on_navigation(move |url| match url.host_str() {
                    Some("margincall.quit") => std::process::exit(0),
                    Some("margincall.fullscreen") => {
                        let h = handle.clone();
                        std::thread::spawn(move || {
                            if let Some(w) = h.get_webview_window("main") {
                                let full = w.is_fullscreen().unwrap_or(false);
                                let _ = w.set_fullscreen(!full);
                            }
                        });
                        false
                    }
                    _ => true,
                })
                .build()?;
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("MARGIN CALL konnte nicht starten");
}
