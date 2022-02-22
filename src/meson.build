subdir('styles')

kgx_sources = [
  'kgx-application.c',
  'kgx-application.h',
  'fp-vte-util.c',
  'fp-vte-util.h',
  'kgx-terminal.c',
  'kgx-terminal.h',
  'kgx-tab.c',
  'kgx-tab.h',
  'kgx-tab-button.c',
  'kgx-tab-button.h',
  'kgx-tab-switcher.c',
  'kgx-tab-switcher.h',
  'kgx-tab-switcher-row.c',
  'kgx-tab-switcher-row.h',
  'kgx-theme-switcher.c',
  'kgx-theme-switcher.h',
  'kgx-simple-tab.c',
  'kgx-simple-tab.h',
  'kgx-pages.c',
  'kgx-pages.h',
  'kgx-proxy-info.c',
  'kgx-proxy-info.h',
  'kgx-close-dialog.c',
  'kgx-close-dialog.h',
  'kgx-window.c',
  'kgx-window.h',
  'kgx-process.c',
  'kgx-process.h',
]

kgx_deps = [
  dependency('gio-2.0', version: '>= 2.66'),
  dependency('gtk+-3.0', version: '>= 3.24'),
  dependency('libhandy-1', version: '>= 1.5', fallback: ['handy', 'libhandy_dep']),
  dependency('vte-2.91', version: '>= 0.67', fallback: ['vte', 'libvte_gtk3_dep']),
  dependency('libgtop-2.0'),
  dependency('gsettings-desktop-schemas'),
  cc.find_library('m', required: false),
]

kgx_sources += config_h

resources_file = configure_file(
                       input: 'kgx.gresource.xml.in',
                      output: 'kgx.gresource.xml',
               configuration: conf
)

kgx_sources += gnome.compile_resources('kgx-resources',
  resources_file,
  c_name: 'kgx',
  dependencies: style_deps
)

kgx_enums = gnome.mkenums_simple('kgx-enums',
                                    sources: [
                                      'kgx-close-dialog.h',
                                      'kgx-terminal.h',
                                      'kgx-window.h',
                                      'kgx-tab.h'
                                    ])

kgx_lib = static_library (bin_name,
                   kgx_sources + kgx_enums,
     dependencies: kgx_deps,
)
kgx_inc = include_directories('.')

kgx_dep = declare_dependency (sources: [config_h, kgx_enums],
                              include_directories: kgx_inc,
                              link_with: kgx_lib,
                              dependencies: kgx_deps)

executable(        bin_name,
                   'main.c',
     dependencies: kgx_dep,
          install: true,
)
