import _SoirV8 as sv
iso = sv.JSIsolate()
iso.enter()
ctx = sv.JSContext()
print(dir(ctx))
