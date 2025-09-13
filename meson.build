project(
  'gnome-console',
  'c',
  version: '50.alpha',
  meson_version: '>= 1.5.0',
  license: 'GPL-3.0-or-later',
  license_files: ['COPYING'],
  default_options: [
    'buildtype=debugoptimized',
    'c_std=c17',
    'adwaita:examples=false',
    'adwaita:introspection=disabled',
    'adwaita:tests=false',
    'adwaita:vapi=false',
    'gtk:build-examples=false',
    'gtk:build-tests=false',
    'gtk:build-testsuite=false',
    'gtk:build-demos=false',
    'gtk:introspection=disabled',
    'libsass:werror=false',
    'vte:c_std=gnu11',
    'vte:cpp_std=gnu++17',
    'vte:gir=false',
    'vte:gtk3=false',
    'vte:gtk4=true',
    'vte:vapi=false',
    'vte:werror=false',
  ],
)

i18n = import('i18n')
gnome = import('gnome')


if get_option('devel')
  app_id = 'org.gnome.Console.Devel'
  app_path = '/org/gnome/Console/Devel/'
  bin_name = 'kgx-devel'
  log_domain = 'KgxDevel'
else
  app_id = 'org.gnome.Console'
  app_path = '/org/gnome/Console/'
  bin_name = 'kgx'
  log_domain = 'Kgx'
endif

kgx_homepage_url = 'https://apps.gnome.org/Console/'
kgx_issue_url = 'https://gitlab.gnome.org/GNOME/console/-/issues/'
copyright_range = '2019-2025'
vcs_tag_fallback = 'no-tag'

prefix = get_option('prefix')
bindir = prefix / get_option('bindir')
libdir = prefix / get_option('libdir')
datadir = prefix / get_option('datadir')
pkglibdir = prefix / libdir / bin_name
pkgdatadir = datadir / bin_name

conf = configuration_data()
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('GETTEXT_PACKAGE', bin_name)
conf.set_quoted('KGX_APPLICATION_ID', app_id)
conf.set('KGX_APPLICATION_ID_RAW', app_id)
conf.set_quoted('KGX_APPLICATION_PATH', app_path)
conf.set('KGX_APPLICATION_PATH_RAW', app_path)
conf.set_quoted('KGX_LIVERY_UUID_KGX', '9f1374fd-f199-429f-bae6-9cf1260f6e3e')
conf.set_quoted('KGX_LIVERY_UUID_LINUX', '131b4aac-399b-4ee4-a8e1-f22e5c3c7bdd')
conf.set_quoted('KGX_LIVERY_UUID_XTERM', '54156855-4a0d-454a-9d5a-7d3e2c9f26f5')
conf.set_quoted('LOCALEDIR', prefix / get_option('localedir'))
conf.set('BIN_DIR', prefix / bindir)
conf.set('BIN_NAME', bin_name)
conf.set_quoted('KGX_BIN_NAME', bin_name)
conf.set_quoted('KGX_ISSUE_URL', kgx_issue_url)
conf.set('KGX_ISSUE_URL_RAW', kgx_issue_url)
conf.set_quoted('KGX_HOMEPAGE_URL', kgx_homepage_url)
conf.set('KGX_HOMEPAGE_URL_RAW', kgx_homepage_url)
conf.set_quoted('KGX_COPYRIGHT_RANGE', copyright_range)
conf.set('KGX_COPYRIGHT_RANGE_RAW', copyright_range)
conf.set_quoted('KGX_GIT_TAG_FALLBACK', vcs_tag_fallback)
conf.set('IS_DEVEL', get_option('devel'))

if get_option('tests')
  conf.set_quoted('TRUE_BIN_PATH', find_program('true').full_path())
endif

# Gtk Requirement
gtk_min_ver = '4.19'
gtk_min_ver_parts = gtk_min_ver.split('.')
if gtk_min_ver_parts[1].to_int().is_odd()
  gtk_sym_ver = 'GDK_VERSION_@0@_@1@'.format(
    gtk_min_ver_parts[0],
    gtk_min_ver_parts[1].to_int() + 1,
  )
else
  gtk_sym_ver = 'GDK_VERSION_@0@@_@_@1@'.format(
    gtk_min_ver_parts[0],
    gtk_min_ver_parts[1],
  )
endif

# GLib Requirement
glib_min_ver = '2.80'
glib_sym_ver = 'GLIB_VERSION_@0@'.format(glib_min_ver.replace('.', '_'))

# Adwaita Requirement
adw_min_ver = '1.8'
adw_sym_ver = 'ADW_VERSION_@0@'.format(adw_min_ver.replace('.', '_'))

kgx_cargs = [
  '-DG_LOG_DOMAIN="@0@"'.format(log_domain),
  '-DG_LOG_USE_STRUCTURED',
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_sym_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_sym_ver),
  '-DGDK_VERSION_MIN_REQUIRED=@0@'.format(gtk_sym_ver),
  '-DGDK_VERSION_MAX_ALLOWED=@0@'.format(gtk_sym_ver),
  '-DADW_VERSION_MIN_REQUIRED=@0@'.format(adw_sym_ver),
  '-DADW_VERSION_MAX_ALLOWED=@0@'.format(adw_sym_ver),
]

cc = meson.get_compiler('c')

foreach arg : [
  '-Wdeclaration-after-statement',
  ['-Werror=format-security', '-Werror=format=2'],
  '-Werror=missing-declarations',
  '-Wformat-nonliteral',
  '-Wformat-security',
  '-Werror=format',
  '-Winit-self',
  '-Wmaybe-uninitialized',
  '-Wno-strict-aliasing',
  '-Wno-unused-parameter',
  '-Wold-style-definition',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wswitch-default',
  '-Wswitch-enum',
  '-Wunused-function',
]
  if cc.has_multi_arguments(arg)
    kgx_cargs += arg
  endif
endforeach

global_cargs = []

if get_option('buildtype') != 'plain'
  foreach arg : [
    '-fstack-protector-strong',
    '-fno-omit-frame-pointer',
    '-mno-omit-leaf-frame-pointer',
    '-fasynchronous-unwind-tables',
  ]
    if cc.has_multi_arguments(arg)
      global_cargs += arg
    endif
  endforeach
endif

add_global_arguments(global_cargs, language: 'c')

gio_dep = dependency('gio-2.0', version: '>= @0@'.format(glib_min_ver))
gio_unix_dep = dependency('gio-unix-2.0', version: '>= @0@'.format(glib_min_ver))
vte_dep = dependency('vte-2.91-gtk4', version: '>= 0.77.0')
gtk_dep = dependency('gtk4', version: '>= @0@'.format(gtk_min_ver))
adw_dep = dependency('libadwaita-1', version: '>= @0@'.format(adw_min_ver))
pango_dep = dependency('pango', version: '>= 1.51.2')
gtop_dep = dependency('libgtop-2.0')
pcre_dep = dependency('libpcre2-8', version: '>= 10.32')
schemas_dep = dependency('gsettings-desktop-schemas')

subdir('data')
subdir('src')
subdir('po')
if get_option('tests')
  subdir('tests')
endif

gnome.post_install(
  glib_compile_schemas: true,
  gtk_update_icon_cache: true,
  update_desktop_database: true,
)
