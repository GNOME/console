project('kgx', 'c', version: '0.1.0',
  meson_version: '>= 0.49.0',
  default_options: ['werror=true', 'c_std=gnu11', 'cpp_std=gnu++17'],
)

i18n = import('i18n')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'kgx')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config_h.set('BINDIR', join_paths(get_option('prefix'), get_option('bindir')))
if get_option('gtop')
  config_h.set('HAS_GTOP', 1)
else
  config_h.set('HAS_GTOP', 0)
endif
configure_file(
  output: 'kgx-config.h',
  configuration: config_h,
)

gtk_ver = 'GDK_VERSION_3_24'
glib_ver = 'GLIB_VERSION_2_58'
add_project_arguments([
  '-I' + meson.build_root(),
  '-DGDK_VERSION_MIN_REQUIRED=@0@'.format(gtk_ver),
  '-DGDK_VERSION_MAX_ALLOWED=@0@'.format(gtk_ver),
  '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_ver),
  '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_ver),
], language: 'c')

subdir('data')
subdir('src')
subdir('po')
subdir('doc')

meson.add_install_script('build-aux/meson/postinstall.py')
