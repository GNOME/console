kgx_sources = [
  'main.c',
  'kgx-window.c',
]

kgx_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('vte-2.91')
]

gnome = import('gnome')

kgx_sources += gnome.compile_resources('kgx-resources',
  'kgx.gresource.xml',
  c_name: 'kgx'
)

executable('kgx', kgx_sources,
  dependencies: kgx_deps,
  install: true,
)
