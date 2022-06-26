sassc = find_program('sassc', required: get_option('sassc'))

if sassc.found()
  styles = [
    'styles-light',
    'styles-dark',
    'styles-hc',
    'styles-hc-dark',
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
         '_definitions.scss',
         '_drawing.scss',
         '_recoloring.scss',
         '_styles.scss',
        ]),
      ),
    ]
  endforeach
endif
