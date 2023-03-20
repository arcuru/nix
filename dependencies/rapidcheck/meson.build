# rapidcheck Dependency File
#========================================================================

# Find rapidcheck using the provided option
rapidcheck_inc = include_directories(
    get_option('rapidcheck_dir') + '/extras/gtest/include')

rapidcheck_dep = declare_dependency(
    dependencies : cpp.find_library('rapidcheck',
        dirs : [ get_option('rapidcheck_dir') + '/lib']),
    include_directories : rapidcheck_inc)
