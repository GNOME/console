kgx_pids_sources = files('kgx-gtop.c', 'kgx-gtop.h', 'kgx-pids.c', 'kgx-pids.h')

kgx_pids_enum_sources = files('kgx-pids.h')

kgx_pids_enums = gnome.mkenums_simple(
  'kgx-pids-enums',
  sources: kgx_pids_enum_sources,
)

kgx_pids_lib = static_library(
  '@0@-pids'.format(bin_name),
  kgx_pids_sources + kgx_pids_enums,
  dependencies: [kgx_config_dep, gtop_dep, gio_unix_dep],
  c_args: kgx_cargs,
  install: false,
  gnu_symbol_visibility: 'internal',
)
kgx_pids_inc = include_directories('.')

kgx_pids_dep = declare_dependency(
  sources: [kgx_pids_enums],
  include_directories: kgx_pids_inc,
  link_with: kgx_pids_lib,
  dependencies: [kgx_config_dep, gtop_dep, gio_unix_dep],
)
