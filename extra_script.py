Import("env")

env.Append(
    CPPDEFINES=["BOOTLOADER_BUILD=0"],
)