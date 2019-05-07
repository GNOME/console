if get_option('gtk_doc')

subdir('xml')

private_headers = [
    'kgx-dbus.h'
]

images = [
]

content_files = [
]

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')
docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

gnome.gtkdoc('kgx',
           main_xml: 'kgx-docs.xml',
            src_dir: [
                       join_paths(meson.source_root(), 'src'),
                       join_paths(meson.build_root(), 'src'),
                     ],
       dependencies: kgx_dep,
  gobject_typesfile: 'kgx.types',
          scan_args: [
                       '--rebuild-types',
                       '--ignore-headers=' + ' '.join(private_headers),
                     ],
       fixxref_args: [
                       '--html-dir=@0@'.format(docpath),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'glib')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gobject')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gio')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'libgtop')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'gtk3')),
                       '--extra-dir=@0@'.format(join_paths(glib_docpath, 'vte-2.91')),
                     ],
        install_dir: 'kgx',
      content_files: content_files,
        html_assets: images,
            install: true)

endif
