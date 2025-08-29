project('mintd', 'cpp')
lt_dep = dependency('libtorrent-rasterbar')
tomlpp_dep = dependency('tomlplusplus')
mintd_depends = [tomlpp_dep, lt_dep]
ldflags = ['-rdynamic']
cxxflags = ['-Wall', '-g', '-O2']
sources = ['./cmd.cpp', './main.cpp', './util.cpp']
executable(
  'mintd',
  sources,
  include_directories: './include/',
  dependencies: mintd_depends,
  link_args: ldflags,
  cpp_args: cxxflags
)
