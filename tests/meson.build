tests = [
  'colour-utils',
  'empty',
  'file-closures',
  'icon-closures',
  'livery',
  'locale',
  'palette',
  'proxy-info',
  'settings',
  'shared-closures',
  'spad-source',
  'spad',
  'tab',
  'theme-switcher',
  'utils',
]

test_env = {
  'G_DEBUG': 'gc-friendly,fatal-warnings',
  'G_ENABLE_DIAGNOSTIC': '1',
  'G_TEST_BUILDDIR': meson.current_build_dir(),
  'G_TEST_SRCDIR': meson.current_source_dir(),
  'GDK_DEBUG': 'no-portals',
  'GDK_DISABLE': 'vulkan',
  'GIO_USE_VFS': 'local',
  'GSETTINGS_BACKEND': 'memory',
  'GSETTINGS_SCHEMA_DIR': meson.project_build_root() / 'data',
  'GTK_A11Y': 'test',
  # Setting this to an empty string prevents gtk from looking in the system
  # dirs where an older gtk install may have placed a print backend that
  # conflicts with those now built into gtk
  'GTK_EXE_PREFIX': '',
  'MALLOC_CHECK_': '2',
  'NO_AT_BRIDGE': '1',
  'PYTHONDEVMODE': '1',
  'PYTHONDONTWRITEBYTECODE': 'yes',
}

foreach test : tests
  test(
    test,
    executable(
      'test-@0@'.format(test),
      'test-@0@.c'.format(test),
      c_args: kgx_cargs,
      dependencies: kgx_dep,
    ),
    env: test_env,
    protocol: 'tap',
  )
endforeach
