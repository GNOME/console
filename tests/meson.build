tests = [
  'colour-utils',
  'livery',
  'palette',
  'shared-closures',
  'tab',
  'utils',
]

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())
test_env.set('G_DEBUG', 'gc-friendly,fatal-warnings')
test_env.set('GSETTINGS_BACKEND', 'memory')
test_env.set('GSETTINGS_SCHEMA_DIR', meson.project_build_root() / 'data')
test_env.set('PYTHONDONTWRITEBYTECODE', 'yes')
test_env.set('MALLOC_CHECK_', '2')
test_env.set('NO_AT_BRIDGE', '1')

foreach test : tests
  t = executable('test-@0@'.format(test),
                 'test-@0@.c'.format(test),
                 c_args: kgx_cargs,
                 dependencies: kgx_dep)
  test(test, t, env: test_env)
endforeach
