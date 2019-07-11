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
]

ideal_vte = dependency('vte-2.91', version: '>= 0.57',
                       fallback: ['vte', 'libvte_gtk3_dep'],
                       required: false)

if not ideal_vte.found()
  message('Using old vte, this is only supported for PureOS!')
  add_project_arguments(['-DVTE_PTY_NO_CTTY=0'], language: 'c')
  kgx_deps += [dependency('vte-2.91', version: '>= 0.54.2')]
else
  kgx_deps += [ideal_vte]
endif

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
