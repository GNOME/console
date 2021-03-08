nautilus_sources = [
    'kgx-nautilus.c',
    'kgx-nautilus.h',
    'kgx-nautilus-menu-item.c',
    'kgx-nautilus-menu-item.h',
    config_h
]

nautilus_extension = shared_library('kgx-nautilus', nautilus_sources,
                                     dependencies: [nautilus_dep],
                              include_directories: kgx_inc,
                                          install: true,
                                      install_dir: libdir / 'nautilus' / 'extensions-3.0')
