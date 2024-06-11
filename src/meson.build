kgx_sources = [
  'fp-vte-util.c',
  'fp-vte-util.h',
  'kgx-about.c',
  'kgx-about.h',
  'kgx-application.c',
  'kgx-application.h',
  'kgx-close-dialog.c',
  'kgx-close-dialog.h',
  'kgx-despatcher.c',
  'kgx-despatcher.h',
  'kgx-drop-target.c',
  'kgx-drop-target.h',
  'kgx-pages.c',
  'kgx-pages.h',
  'kgx-paste-dialog.c',
  'kgx-paste-dialog.h',
  'kgx-process.c',
  'kgx-process.h',
  'kgx-proxy-info.c',
  'kgx-proxy-info.h',
  'kgx-settings.c',
  'kgx-settings.h',
  'kgx-simple-tab.c',
  'kgx-simple-tab.h',
  'kgx-system-info.c',
  'kgx-system-info.h',
  'kgx-tab.c',
  'kgx-tab.h',
  'kgx-terminal.c',
  'kgx-terminal.h',
  'kgx-theme-switcher.c',
  'kgx-theme-switcher.h',
  'kgx-utils.h',
  'kgx-watcher.c',
  'kgx-watcher.h',
  'kgx-window.c',
  'kgx-window.h',
  'preferences/kgx-font-picker.c',
  'preferences/kgx-font-picker.h',
  'preferences/kgx-preferences-window.c',
  'preferences/kgx-preferences-window.h',
]

kgx_deps = [
  gio_dep,
  gio_unix_dep,
  pango_dep,
  gtk_dep,
  adw_dep,
  vte_dep,
  gtop_dep,
  pcre_dep,
  schemas_dep,
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
  c_name: 'kgx'
)

kgx_sources += gnome.genmarshal('kgx-marshals',
                  sources: 'kgx-marshals.list',
                 internal: true,
                   prefix: 'kgx_marshals',
       valist_marshallers: true,
)

kgx_sources += gnome.gdbus_codegen('xdg-fm1',
           sources: 'org.freedesktop.FileManager1.xml',
  interface_prefix: 'org.freedesktop.',
         namespace: 'Xdg',
)

kgx_enums = gnome.mkenums_simple('kgx-enums',
                                    sources: [
                                      'kgx-close-dialog.h',
                                      'kgx-paste-dialog.h',
                                      'kgx-settings.h',
                                      'kgx-tab.h',
                                      'kgx-terminal.h',
                                    ])

kgx_lib = static_library (bin_name,
                   kgx_sources + kgx_enums,
           c_args: kgx_cargs,
     dependencies: kgx_deps,
)
kgx_inc = include_directories('.')

kgx_dep = declare_dependency (sources: [config_h, kgx_enums, kgx_schemas],
                              include_directories: kgx_inc,
                              link_with: kgx_lib,
                              dependencies: kgx_deps)

executable(        bin_name,
                   'main.c',
     dependencies: kgx_dep,
           c_args: kgx_cargs,
          install: true,
)
