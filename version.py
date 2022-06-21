versionDefines = [
	"#define SIGNALSMITH_DSP_VERSION_MAJOR ",
	"#define SIGNALSMITH_DSP_VERSION_MINOR ",
	"#define SIGNALSMITH_DSP_VERSION_PATCH "
]
versionStringDefine = "#define SIGNALSMITH_DSP_VERSION_STRING \"%s\""
version = [0, 0, 0]

with open("dsp/common.h") as commonH:
	for line in commonH:
		for i in range(3):
			if line.startswith(versionDefines[i]):
				version[i] = int(line[len(versionDefines[i]):])

startVersion = list(version)

import sys
if len(sys.argv) > 1:
	action = sys.argv[1]
	if action == "bump-patch":
		version[2] += 1
	elif action == "bump-minor":
		version[1] += 1
		version[2] = 0
	elif action == "bump-major":
		version[0] += 1
		version[1] = 0
		version[2] = 0
	else:
		print("Unrecognised action: " + action)
		exit(1)
		
oldVersion = ".".join([str(x) for x in startVersion])
newVersion = ".".join([str(x) for x in version])

code = None
with open("dsp/common.h") as commonH:
	code = commonH.read()
if code != None:
	for i in range(3):
		code = code.replace(versionDefines[i] + str(startVersion[i]), versionDefines[i] + str(version[i]))
	code = code.replace(versionStringDefine%(oldVersion,), versionStringDefine%(newVersion,))
	with open("dsp/common.h", 'w') as commonH:
		commonH.write(code)
		
text = ""
with open("Doxyfile") as doxyfile:
	text = doxyfile.read()
text = text.replace("PROJECT_NUMBER = " + oldVersion, "PROJECT_NUMBER = " + newVersion)
with open("Doxyfile", 'w') as doxyfile:
	doxyfile.write(text)

text = ""
with open("dsp/README.md") as readme:
	text = readme.read()
text = text.replace("SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(startVersion), "SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(version))
with open("dsp/README.md", 'w') as readme:
	readme.write(text)

code = ""
with open("tests/common/version.cpp") as testFile:
	code = testFile.read()
code = code.replace("SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(startVersion), "SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(version))
with open("tests/common/version.cpp", 'w') as testFile:
	testFile.write(code)

print(newVersion)
