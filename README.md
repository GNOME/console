# King's Cross

![](logo.png)

A minimal terminal for GNOME

KGX is supposed to be a simple terminal emulator for the average user to carry out simple cli tasks and aims to be a 'core' app for GNOME/Phosh

We are not however trying to replace GNOME Terminal/Tilix, these advanced tools are great for developers and administrators, rather kgx aims to serve the casual linux user who rarely needs a terminal

## Roadmap

- [ ] 'API' compatible with GNOME Terminal
    - [ ] Command line flags
    - [ ] DBus interface
- [ ] Command done notifications
- [ ] 'root mode' turns red when sudo/su/pkexec is active in the terminal
- [ ] 'remote mode' turns (purple?) when ssh is in use
- [ ] Other bash/shell integrations

Due to the complexities of sandboxing a terminal emulator flatpaks are only for development purposes (for now)
