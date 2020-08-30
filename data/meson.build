if get_option('generic')
  desktop_file = i18n.merge_file(
    input: 'org.gnome.zbrown.KingsCross-generic.desktop.in',
    output: 'org.gnome.zbrown.KingsCross.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications')
  )
else
  desktop_file = i18n.merge_file(
    input: 'org.gnome.zbrown.KingsCross.desktop.in',
    output: 'org.gnome.zbrown.KingsCross.desktop',
    type: 'desktop',
    po_dir: '../po',
    install: true,
    install_dir: join_paths(get_option('datadir'), 'applications')
  )
endif

desktop_utils = find_program('desktop-file-validate', required: false)
if desktop_utils.found() and get_option('tests')
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif

appstream_file = i18n.merge_file(
  input: 'org.gnome.zbrown.KingsCross.appdata.xml.in',
  output: 'org.gnome.zbrown.KingsCross.appdata.xml',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'metainfo')
)

appstream_util = find_program('appstream-util', required: false)
if appstream_util.found() and get_option('tests')
  test('Validate appstream file', appstream_util,
    args: ['validate', '--nonet', appstream_file]
  )
endif

install_data('org.gnome.zbrown.KingsCross.gschema.xml',
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas')
)

compile_schemas = find_program('glib-compile-schemas', required: false)
if compile_schemas.found() and get_option('tests')
  test('Validate schema file', compile_schemas,
    args: ['--strict', '--dry-run', meson.current_source_dir()]
  )
endif

configure_file(input: 'org.gnome.zbrown.KingsCross.service.in',
               output: 'org.gnome.zbrown.KingsCross.service',
               configuration: conf,
               install_dir: join_paths([get_option('datadir'),'dbus-1/services']))

install_data('org.gnome.zbrown.KingsCross.svg',
             install_dir: join_paths([get_option('datadir'),'icons/hicolor/scalable/apps']))

install_data('org.gnome.zbrown.KingsCross-symbolic.svg',
             install_dir: join_paths([get_option('datadir'),'icons/hicolor/symbolic/apps']))

install_data('org.gnome.zbrown.KingsCross.Generic.svg',
             install_dir: join_paths([get_option('datadir'),'icons/hicolor/scalable/apps']))

install_data('org.gnome.zbrown.KingsCross.Generic-symbolic.svg',
             install_dir: join_paths([get_option('datadir'),'icons/hicolor/symbolic/apps']))

