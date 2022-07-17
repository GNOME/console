sassc = find_program('sassc', required: false)
if not sassc.found() and not get_option('sassc').disabled()
  subproject('sassc')
  sassc = find_program('sassc', required: get_option('sassc').enabled())
endif

if sassc.found()
  styles = [
    'styles-light',
    'styles-dark',
  ]

  style_deps = []

  sassc_opts = [ '-a', '-M', '-t', 'compact' ]

  foreach style: styles
    style_deps += [
      custom_target(style,
        input: '@0@.scss'.format(style),
        output: '@0@.css'.format(style),
        command: [
          sassc, sassc_opts, '@INPUT@', '@OUTPUT@',
        ],
        depend_files: files([
         '_styles.scss',
        ]),
      ),
    ]
  endforeach
endif
