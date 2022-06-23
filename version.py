versionDefines = [
	"#define SIGNALSMITH_DSP_VERSION_MAJOR ",
	"#define SIGNALSMITH_DSP_VERSION_MINOR ",
	"#define SIGNALSMITH_DSP_VERSION_PATCH "
]
versionStringDefine = "#define SIGNALSMITH_DSP_VERSION_STRING \"%s\""
version = [0, 0, 0]

# Read existing version from `#define`s
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
		
def fileReplace(filename, fromText, toText):
	text = ""
	with open(filename) as textfile:
		text = textfile.read()
	if len(text) == 0:
		return
	text = text.replace(fromText, toText)
	with open(filename, 'w') as textfile:
		textfile.write(text)

for i in range(3):
	fileReplace("dsp/common.h", versionDefines[i] + str(startVersion[i]), versionDefines[i] + str(version[i]))
fileReplace("dsp/common.h", versionStringDefine%oldVersion, versionStringDefine%newVersion)
fileReplace("Doxyfile", "PROJECT_NUMBER = " + oldVersion, "PROJECT_NUMBER = " + newVersion)
fileReplace("dsp/README.md",
	"SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(startVersion),
	"SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(version))
fileReplace("tests/common/version.cpp",
	"SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(startVersion),
	"SIGNALSMITH_DSP_VERSION_CHECK(%i, %i, %i)"%tuple(version))

print(newVersion)
