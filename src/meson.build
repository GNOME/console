kgx_sources = [
  'kgx-application.c',
  'fp-vte-util.c',
  'kgx-terminal.c',
  'kgx-search-box.c',
  'kgx-window.c',
]

kgx_deps = [
  dependency('gio-2.0', version: '>= 2.58'),
  dependency('gtk+-3.0', version: '>= 3.24'),
  dependency('vte-2.91', version: '>= 0.50'),
]

if get_option('gtop')
  kgx_deps += dependency('libgtop-2.0')
  kgx_sources += 'kgx-process.c'
else
  kgx_sources += 'kgx-process-dummy.c'
endif

gnome = import('gnome')

kgx_sources += gnome.compile_resources('kgx-resources',
  'kgx.gresource.xml',
  c_name: 'kgx'
)

kgx_sources += gnome.gdbus_codegen('kgx-dbus',
  sources: 'org.gnome.Terminal.xml',
  interface_prefix : 'org.gnome.',
  object_manager: false,
  namespace : 'KgxDBus'
)

kgx_sources += gnome.mkenums_simple('kgx-enums', sources: 'kgx-terminal.h')

kgx_lib = library ('kgx', kgx_sources, dependencies: kgx_deps)
kgx_inc = include_directories('.')

kgx_dep = declare_dependency (sources: kgx_sources,
                              include_directories: kgx_inc,
                              link_with: kgx_lib,
                              dependencies: kgx_deps)

executable('kgx', 'main.c',
  dependencies: kgx_dep,
  install: true,
)
