import fnmatch
import sys
import os

source_files = []
for root, dirnames, filenames in os.walk('src'):
    for filename in filenames:
        if fnmatch.fnmatch(filename, '*.cpp'):
            source_files.append(str(os.path.join(root, filename)))

build_dir = 'build'

# Windows
if sys.platform[:3] == 'win':
    build_dir = 'build_win'
    env = Environment(tools = ['mingw'])

# Linux etc.
else:
    env = Environment()

VariantDir(build_dir, 'src', duplicate=0)
source_files = [s.replace('src', build_dir, 1) for s in source_files]
env.Append(LIBS=['opencv_core', 'opencv_imgproc',
                 'opencv_highgui', 'opencv_imgcodecs'])
env.Append(CXXFLAGS='-std=c++11 -O3 -Wall -Wextra -pedantic -Werror')
env.Program(target='release/AllColors', source=source_files)