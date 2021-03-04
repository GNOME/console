sassc = find_program('sassc', required: false)
if not sassc.found() and not get_option('sassc').disabled()
  subproject('sassc')
  sassc = find_program('sassc', required: get_option('sassc').enabled())
endif

if sassc.found()
  sassc_opts = [ '-a', '-M', '-t', 'compact' ]

  style_deps = [
    custom_target('Stylesheet',
      input: 'styles.scss',
      output: 'styles.css',
      command: [
        sassc, sassc_opts, '@INPUT@', '@OUTPUT@',
      ],
      depend_files: files([
       '_definitions.scss',
       'styles.scss'
      ]),
    ),
  ]
endif
