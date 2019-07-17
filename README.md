# King's Cross

![](logo.png)

A minimal terminal for GNOME

KGX is supposed to be a simple terminal emulator for the average user to carry out simple cli tasks and aims to be a 'core' app for GNOME/Phosh

We are not however trying to replace GNOME Terminal/Tilix, these advanced tools are great for developers and administrators, rather kgx aims to serve the casual linux user who rarely needs a terminal

## Why the name?

It's not a out there as some of those listed in [debian's WhyTheName](https://wiki.debian.org/WhyTheName) but likely falls into the "developers imagine it's obvious" trap

[National Rail](http://www.nationalrail.co.uk/) assigns a code to all stations in [Great Britain](https://en.wikipedia.org/wiki/Great_Britain) (note other parts of the UK are managed by different organisations)

KGX is the station code for [King's Cross](https://www.nationalrail.co.uk/stations_destinations/kgx.aspx) (of Harry Potter fame), the London *terminus* of the East Coast Main Line

So there you go, it's a terminal emulator named after a real work terminal

### Railway Nerds

Yes the icon is a [TfL Roundel](https://tfl.gov.uk/corporate/about-tfl/culture-and-heritage/art-and-design/the-roundel) which would mean King's Cross St Pancras, the Underground station, rather than King's Cross but it's awfully hard to draw a simple icon for King's Cross (and without copyright concerns).

We also considered a center aligned ~ to be a proper station roundel but we felt left aligned >_ better represented a terminal

## Roadmap

- [ ] 'API' compatible with GNOME Terminal
    - [ ] Command line flags *Partial, supports -e/--command and --working-directory*
- [ ] Command done notifications
- [X] 'root mode' turns red when sudo/su/pkexec is active in the terminal
- [X] 'remote mode' turns (purple?) when ssh is in use
- [ ] Other bash/shell integrations

Due to the complexities of sandboxing a terminal emulator flatpaks are only for development purposes (for now)
