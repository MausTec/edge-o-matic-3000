Import("env")

# General options that are passed to the C and C++ compilers
# env.Append(CCFLAGS=["flag1", "flag2"])

# General options that are passed to the C compiler (C only; not C++).
env.Append(CFLAGS=["-Werror=incompatible-pointer-type", "-Werror=incompatible-pointer-types", "-Wno-override-init"])

# General options that are passed to the C++ compiler
# env.Append(CXXFLAGS=["flag1", "flag2"]) 