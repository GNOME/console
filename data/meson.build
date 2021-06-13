src_datadir = meson.current_source_dir()

desktop_utils = find_program('desktop-file-validate', required: false)
appstream_util = find_program('appstream-util', required: false)
compile_schemas = find_program('glib-compile-schemas', required: false)

if get_option('generic')
  desktop_filename = 'org.gnome.zbrown.KingsCross-generic.desktop.in.in'
else
  desktop_filename = 'org.gnome.zbrown.KingsCross.desktop.in.in'
endif

desktop_file = i18n.merge_file(
            input: configure_file(
                           input: desktop_filename,
                           output: '@BASENAME@',
                           configuration: conf
                   ),
           output: app_id + '.desktop',
             type: 'desktop',
           po_dir: '../po',
          install: true,
      install_dir: datadir / 'applications'
)

if desktop_utils.found() and get_option('tests')
  test('Validate desktop file', desktop_utils,
    args: [desktop_file]
  )
endif

appstream_file = i18n.merge_file(
            input: configure_file(
                           input: 'org.gnome.zbrown.KingsCross.appdata.xml.in.in',
                           output: '@BASENAME@',
                           configuration: conf
                   ),
           output: app_id + '.appdata.xml',
           po_dir: '../po',
          install: true,
      install_dir: datadir / 'metainfo'
)

if appstream_util.found() and get_option('tests')
  test('Validate appstream file', appstream_util,
    args: ['validate', '--nonet', appstream_file]
  )
endif

configure_file(
           input: 'org.gnome.zbrown.KingsCross.gschema.xml.in',
          output: app_id + '.gschema.xml',
   configuration: conf,
     install_dir: datadir / 'glib-2.0/schemas'
)

if compile_schemas.found() and get_option('tests')
  test('Validate schema file', compile_schemas,
    args: ['--strict', '--dry-run', src_datadir]
  )
endif

configure_file(
           input: 'org.gnome.zbrown.KingsCross.service.in',
          output: app_id + '.service',
   configuration: conf,
     install_dir: datadir / 'dbus-1/services'
)

install_data('org.gnome.zbrown.KingsCross.svg',
     install_dir: datadir / 'icons/hicolor/scalable/apps',
          rename: app_id + '.svg',
)

install_data('org.gnome.zbrown.KingsCross-symbolic.svg',
     install_dir: datadir / 'icons/hicolor/symbolic/apps',
          rename: app_id + '-symbolic.svg',
)
