project('mintd', 'cpp',
  default_options : ['cpp_std=c++20'])
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
  cpp_args: cxxflags,
  override_options : ['cpp_std=c++20']
)
