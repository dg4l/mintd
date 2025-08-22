project('mintd', 'cpp')
lt_dep = dependency('libtorrent-rasterbar')
ldflags = ['-rdynamic']
cxxflags = ['-Wall', '-g', '-O2']
sources = ['./cmd.cpp', './main.cpp', './util.cpp']
executable(
  'mintd',
  sources,
  include_directories: './include/',
  dependencies: lt_dep,
  link_args: ldflags,
  cpp_args: cxxflags
)
