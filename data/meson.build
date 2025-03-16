desktop_utils = find_program(
 'desktop-file-validate',
 required: get_option('tests'),
)
appstreamcli = find_program('appstreamcli', required: get_option('tests'))
compile_schemas = find_program('glib-compile-schemas')

desktop_filename = 'org.gnome.Console.desktop.in.in'
if get_option('devel')
  icon_filename = 'org.gnome.Console.Devel.svg'
else
  icon_filename = 'org.gnome.Console.svg'
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

if get_option('tests')
 test('Validate desktop file', desktop_utils, args: [desktop_file])
endif

appstream_file = i18n.merge_file(
            input: configure_file(
                           input: 'org.gnome.Console.metainfo.xml.in.in',
                           output: '@BASENAME@',
                           configuration: conf
                   ),
           output: app_id + '.metainfo.xml',
           po_dir: '../po',
          install: true,
      install_dir: datadir / 'metainfo'
)

if get_option('tests')
 test(
  'Validate appstream file',
  appstreamcli,
  args: ['validate', '--no-net', '--explain', appstream_file],
 )
endif

schema_file = configure_file(
           input: 'org.gnome.Console.gschema.xml.in',
          output: app_id + '.gschema.xml',
   configuration: conf,
     install_dir: datadir / 'glib-2.0/schemas'
)

# gnome.compile_schemas only looks at the source tree
kgx_schemas = custom_target('kgx-schemas',
           input: schema_file,
          output: 'gschemas.compiled',
         command: [compile_schemas, meson.current_build_dir()],
)
meson.add_devenv({'GSETTINGS_SCHEMA_DIR': meson.current_build_dir()})

if get_option('tests')
 test(
  'Validate schema file',
  compile_schemas,
  args: ['--strict', '--dry-run', meson.current_build_dir()],
 )
endif

configure_file(
           input: 'org.gnome.Console.service.in',
          output: app_id + '.service',
   configuration: conf,
     install_dir: datadir / 'dbus-1/services'
)

install_data(icon_filename,
     install_dir: datadir / 'icons/hicolor/scalable/apps',
          rename: app_id + '.svg',
)

install_data('org.gnome.Console-symbolic.svg',
     install_dir: datadir / 'icons/hicolor/symbolic/apps',
          rename: app_id + '-symbolic.svg',
)
