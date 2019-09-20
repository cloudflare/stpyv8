import _SoirV8 as sv
plat = sv.JSPlatform()
plat.init()
iso = sv.JSIsolate()
iso.enter()
ctx = sv.JSContext()
print(dir(ctx))
