#find_package(LibBladeRF REQUIRED)

# Add executable called "helloDemo" that is built from the source files
# "demo.cxx" and "demo_b.cxx". The extensions are automatically found.
add_executable (blade_recorder.out record_iq_blade.cpp)

# Link the executable to the Hello library. Since the Hello library has
# public include directories we will use those link directories when building
# helloDemo
target_link_libraries (blade_recorder.out LINK_PUBLIC bladeRF)
