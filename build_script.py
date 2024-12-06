Import("env")

# Build öncesi temizlik
def before_build(source, target, env):
    env.Execute("pio run -t clean")

env.AddPreAction("buildprog", before_build) 